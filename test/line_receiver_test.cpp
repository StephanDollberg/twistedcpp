#include "catch/single_include/catch.hpp"

#include "../include/twisted/line_receiver.hpp"
#include "../include/twisted/reactor.hpp"
#include "../include/twisted/default_factory.hpp"

#include <boost/range/algorithm.hpp>
#include <boost/asio/read.hpp>
#include <boost/scope_exit.hpp>

#include <vector>
#include <string>
#include <future>

using boost::asio::ip::tcp;

struct line_receiver_test : twisted::line_receiver<line_receiver_test> {
    template <typename Iter>
    void line_received(Iter begin, Iter end) {
        send_line(begin, end);
    }
};

struct line_receiver_test_alt_delimiter
    : twisted::line_receiver<line_receiver_test_alt_delimiter> {

    line_receiver_test_alt_delimiter() : line_receiver("XXX") {}

    template <typename Iter>
    void line_received(Iter begin, Iter end) {
        send_line(begin, end);
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

TEST_CASE("line_receiver behavior tests",
          "[line_receiver][protocols][behavior]") {
    SECTION("perfect match 2 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\nBBB\r\n");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");
        test_results.push_back("BBB\r\n");

        message_tester<line_receiver_test>(test_data, test_results);
    }

    SECTION("perfect match 3 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("\r");
        test_data.push_back("\n");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");

        message_tester<line_receiver_test>(test_data, test_results);
    }

    SECTION("perfect match 2 on 2") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\n");
        test_data.push_back("BBB\r\n");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");
        test_results.push_back("BBB\r\n");

        message_tester<line_receiver_test>(test_data, test_results);
    }

    SECTION("open match 2 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\nBBB\r\nCCC");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");
        test_results.push_back("BBB\r\n");

        message_tester<line_receiver_test>(test_data, test_results);
    }

    SECTION("open match 2 on 2") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\n");
        test_data.push_back("BBB\r\n");
        test_data.push_back("CCC");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");
        test_results.push_back("BBB\r\n");

        message_tester<line_receiver_test>(test_data, test_results);
    }

    SECTION("end match 3 on 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");
        test_data.push_back("CCC\r\n");

        std::vector<std::string> test_results;
        test_results.push_back("AAABBBCCC\r\n");

        message_tester<line_receiver_test>(test_data, test_results);
    }

    SECTION("no match 2 on 0") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");

        std::vector<std::string> test_results;
        test_results.push_back("");

        message_tester<line_receiver_test>(test_data, test_results);
    }

    SECTION("alternative delimiter") {
        std::vector<std::string> test_data;
        test_data.push_back("AAAXXX");

        std::vector<std::string> test_results;
        test_results.push_back("AAAXXX");

        message_tester<line_receiver_test_alt_delimiter>(test_data,
                                                         test_results);
    }
}
