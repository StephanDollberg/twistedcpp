#include "catch/single_include/catch.hpp"

#include "../include/twisted/mixed_receiver.hpp"
#include "../include/twisted/reactor.hpp"
#include "../include/twisted/default_factory.hpp"

#include <boost/range/algorithm.hpp>
#include <boost/asio/read.hpp>
#include <boost/scope_exit.hpp>

#include <vector>
#include <string>
#include <future>

using boost::asio::ip::tcp;

struct mixed_receiver_test : twisted::mixed_receiver<mixed_receiver_test> {
    mixed_receiver_test() : mixed_receiver() {
        set_package_size(5);
    }

    void line_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);
        set_byte_mode();
    }

    void bytes_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);
        set_line_mode();
    }
};

struct double_mixed_receiver_test : twisted::mixed_receiver<double_mixed_receiver_test> {
    double_mixed_receiver_test() : mixed_receiver(), switch_mode(0) {
        set_package_size(5);
    }

    void line_received(const_buffer_iterator begin, const_buffer_iterator end) {
        std::cout << "line" << std::endl;
        send_message(begin, end);

        if(switch_mode == 1) {
            set_byte_mode();
            switch_mode = 0;
        }
        else {
            switch_mode += 1;
        }
    }

    void bytes_received(const_buffer_iterator begin, const_buffer_iterator end) {
        std::cout << "byte" << std::endl;
        send_message(begin, end);

        if(switch_mode == 1) {
            set_line_mode();
            switch_mode = 0;
        }
        else {
            switch_mode += 1;
        }
    }

    int switch_mode;
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

TEST_CASE("mixed_receiver behavior tests",
          "[mixed_receiver][protocols][behavior]") {
    SECTION("perfect march 4 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\nBBBBBCCC\r\nDDDDD");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBBBB");
        test_results.push_back("CCC");
        test_results.push_back("DDDDD");

        message_tester<mixed_receiver_test>(test_data, test_results);
    }

    SECTION("perfect match 4 in 4") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\n");
        test_data.push_back("BBBBB");
        test_data.push_back("CCC\r\n");
        test_data.push_back("DDDDD");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBBBB");
        test_results.push_back("CCC");
        test_results.push_back("DDDDD");

        message_tester<mixed_receiver_test>(test_data, test_results);
    }

    SECTION("double perfect march 8 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\n111\r\nBBBBB22222CCC\r\n333\r\nDDDDD44444");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("111");
        test_results.push_back("BBBBB");
        test_results.push_back("22222");
        test_results.push_back("CCC");
        test_results.push_back("333");
        test_results.push_back("DDDDD");
        test_results.push_back("44444");

        message_tester<double_mixed_receiver_test>(test_data, test_results);
    }

    SECTION("double perfect match 8 in 4") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\n111\r\n");
        test_data.push_back("BBBBB22222");
        test_data.push_back("CCC\r\n333\r\n");
        test_data.push_back("DDDDD44444");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("111");
        test_results.push_back("BBBBB");
        test_results.push_back("22222");
        test_results.push_back("CCC");
        test_results.push_back("333");
        test_results.push_back("DDDDD");
        test_results.push_back("44444");

        message_tester<double_mixed_receiver_test>(test_data, test_results);
    }
}
