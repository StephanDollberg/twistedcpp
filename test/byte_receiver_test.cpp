#include "catch/single_include/catch.hpp"

#include "../include/twisted/byte_receiver.hpp"
#include "../include/twisted/reactor.hpp"
#include "../include/twisted/default_factory.hpp"

#include <boost/range/algorithm.hpp>
#include <boost/asio/read.hpp>
#include <boost/scope_exit.hpp>

#include <vector>
#include <string>
#include <future>

using boost::asio::ip::tcp;

struct byte_receiver_test : twisted::byte_receiver<byte_receiver_test> {
    byte_receiver_test() : byte_receiver(3) {}

    void bytes_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);
    }
};

struct byte_receiver_update_test : twisted::byte_receiver<byte_receiver_update_test> {
    byte_receiver_update_test() : byte_receiver(2) {}

    void bytes_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);

        set_package_size(20);
    }
};

struct byte_receiver_decrease_update_test : twisted::byte_receiver<byte_receiver_decrease_update_test> {
    byte_receiver_decrease_update_test() : byte_receiver(4) {}

    void bytes_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);

        set_package_size(2);
    }
};

template <typename ProtocolType>
void message_tester(const std::vector<std::string>& send_input,
                    const std::vector<std::string>& results) {
    twisted::reactor reac;
    auto fut = std::async(std::launch::async, [&]() {
        reac.listen_tcp(50000, twisted::default_factory<ProtocolType>());
        reac.run();
    });

    boost::asio::io_service io_service;
    tcp::socket socket(io_service);

    BOOST_SCOPE_EXIT_TPL((&reac)(&socket)) {
        reac.stop();
        if (socket.is_open()) {
            socket.close();
        }
    }
    BOOST_SCOPE_EXIT_END

    std::this_thread::sleep_for(std::chrono::milliseconds(3));

    socket.connect(tcp::endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), 50000));

    boost::for_each(send_input, [&](const std::string& input) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        boost::asio::write(socket, boost::asio::buffer(input));
    });

    boost::for_each(results, [&](const std::string& input) {
        std::string buffer(input.size(), '\0');
        boost::asio::read(socket,
                          boost::asio::buffer(&buffer[0], buffer.size()));
        CHECK(buffer == input);
    });
}

TEST_CASE("byte_receiver behavior tests",
          "[byte_receiver][protocols][behavior]") {
    SECTION("perfect match 2 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAABBB");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");

        message_tester<byte_receiver_test>(test_data, test_results);
    }

    SECTION("perfect match 3 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("A");
        test_data.push_back("A");
        test_data.push_back("A");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");

        message_tester<byte_receiver_test>(test_data, test_results);
    }

    SECTION("perfect match 2 on 2") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");

        message_tester<byte_receiver_test>(test_data, test_results);
    }

    SECTION("open match 2 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAABBBCC");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");

        message_tester<byte_receiver_test>(test_data, test_results);
    }

    SECTION("open match 2 on 2") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");
        test_data.push_back("CC");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");

        message_tester<byte_receiver_test>(test_data, test_results);
    }

    SECTION("end match 3 on 1") {
        std::vector<std::string> test_data;
        test_data.push_back("A");
        test_data.push_back("B");
        test_data.push_back("C");

        std::vector<std::string> test_results;
        test_results.push_back("ABC");

        message_tester<byte_receiver_test>(test_data, test_results);
    }

    SECTION("no match 2 on 0") {
        std::vector<std::string> test_data;
        test_data.push_back("A");
        test_data.push_back("B");

        std::vector<std::string> test_results;
        test_results.push_back("");

        message_tester<byte_receiver_test>(test_data, test_results);
    }

    SECTION("even wrap around") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");
        test_data.push_back("CCC");
        test_data.push_back("DDD");
        test_data.push_back("EEE");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");
        test_results.push_back("CCC");
        test_results.push_back("DDD");
        test_results.push_back("EEE");

        message_tester<byte_receiver_test>(test_data, test_results);
    }

    SECTION("uneven wrap around") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");
        test_data.push_back("C");
        test_data.push_back("CCD");
        test_data.push_back("DDE");
        test_data.push_back("EE");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");
        test_results.push_back("CCC");
        test_results.push_back("DDD");
        test_results.push_back("EEE");

        message_tester<byte_receiver_test>(test_data, test_results);
    }

    SECTION("change package size - simple upgrade") {
        std::vector<std::string> test_data;
        test_data.push_back("AA");
        test_data.push_back(std::string(20, 'X'));

        std::vector<std::string> test_results;
        test_results.push_back("AA");
        test_results.push_back(std::string(20, 'X'));

        message_tester<byte_receiver_update_test>(test_data, test_results);
    }

    SECTION("change package size - simple upgrade one shot") {
        std::vector<std::string> test_data;
        std::string test_string = std::string(20, 'X');
        test_string.insert(0, 2, 'A');
        test_data.push_back(test_string);

        std::vector<std::string> test_results;
        test_results.push_back("AA");
        test_results.push_back(std::string(20, 'X'));

        message_tester<byte_receiver_update_test>(test_data, test_results);
    }

    SECTION("decrease package size - simple upgrade") {
        std::vector<std::string> test_data;
        test_data.push_back("AAAABBCCDD");

        std::vector<std::string> test_results;
        test_results.push_back("AAAA");
        test_results.push_back("BB");
        test_results.push_back("CC");
        test_results.push_back("DD");

        message_tester<byte_receiver_decrease_update_test>(test_data, test_results);
    }
}
