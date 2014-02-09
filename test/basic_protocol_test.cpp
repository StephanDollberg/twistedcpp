#include "catch/single_include/catch.hpp"

#include "../include/twisted/reactor.hpp"
#include "../include/twisted/basic_protocol.hpp"
#include "../include/twisted/default_factory.hpp"

#include <boost/asio/read.hpp>
#include <boost/scope_exit.hpp>

#include <vector>
#include <string>
#include <future>

using boost::asio::ip::tcp;

struct echo_protocol : twisted::basic_protocol<echo_protocol> {
    template <typename Iter>
    void on_message(Iter begin, Iter end) {
        send_message(begin, end);
    }
};

TEST_CASE("basic tcp send & recv test", "[tcp][reactor]") {
    twisted::reactor reac;
    auto fut = std::async(std::launch::async, [&]() {
        reac.listen_tcp(50000, twisted::default_factory<echo_protocol>());
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

    std::string input("TEST123");

    boost::asio::write(socket, boost::asio::buffer(input));

    std::string buffer(input.size(), '\0');
    boost::asio::read(socket, boost::asio::buffer(&buffer[0], buffer.size()));
    CHECK(buffer == input);
}

struct error_test_protocol : twisted::basic_protocol<error_test_protocol> {
    template <typename Iter>
    void on_message(Iter begin, Iter end) {
        test_string.assign(begin, end);
        throw std::exception();
    }

    void on_error(std::exception_ptr /* eptr */) {
        send_message(test_string.begin(), test_string.end());
    }

    std::string test_string;
};

TEST_CASE("on_error test", "[tcp][reactor]") {
    twisted::reactor reac;
    auto fut = std::async(std::launch::async, [&]() {
        reac.listen_tcp(50000, twisted::default_factory<error_test_protocol>());
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

    std::string input("TEST123");

    boost::asio::write(socket, boost::asio::buffer(input));

    std::string buffer(input.size(), '\0');
    boost::asio::read(socket, boost::asio::buffer(&buffer[0], buffer.size()));
    CHECK(buffer == input);
}

TEST_CASE("on_disconnect test", "[tcp][reactor]") {}
