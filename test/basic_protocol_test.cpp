#include "catch/single_include/catch.hpp"

#include "test_utils.hpp"

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
    test::single_send_and_recv<echo_protocol>("TEST123", "TEST123");
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
    test::single_send_and_recv<error_test_protocol>("TEST123", "TEST123");
}

struct disconnect_test_protocol
    : twisted::basic_protocol<disconnect_test_protocol> {

    disconnect_test_protocol(bool& disconnected)
        : _disconnected(disconnected) {}

    template <typename Iter>
    void on_message(Iter /* begin */, Iter /* end */) {}

    void on_disconnect() { _disconnected = true; }

    bool& _disconnected;
};

TEST_CASE("on_disconnect test", "[tcp][reactor]") {
    twisted::reactor reac;
    bool disconnected = false;
    auto fut = std::async(std::launch::async, [&]() {
        reac.listen_tcp(
            50000, [&]() { return disconnect_test_protocol(disconnected); });
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

    socket.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    CHECK(disconnected == true);
}
