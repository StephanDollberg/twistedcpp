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
#include <chrono>

using boost::asio::ip::tcp;

struct echo_protocol : twisted::basic_protocol<echo_protocol> {
    void on_message(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);
    }
};

TEST_CASE("basic tcp send & recv test", "[tcp][reactor]") {
    test::single_send_and_recv<echo_protocol>(
        "TEST123", "TEST123", twisted::default_factory<echo_protocol>());
}

struct error_test_protocol : twisted::basic_protocol<error_test_protocol> {
    void on_message(const_buffer_iterator begin, const_buffer_iterator end) {
        test_string.assign(begin, end);
        throw std::exception();
    }

    void on_error(std::exception_ptr /* eptr */) {
        send_message(test_string.begin(), test_string.end());
    }

    std::string test_string;
};

TEST_CASE("on_error test", "[tcp][reactor]") {
    test::single_send_and_recv<error_test_protocol>(
        "TEST123", "TEST123", twisted::default_factory<error_test_protocol>());
}

TEST_CASE("on_error test - protocol is fully functioning after error",
          "[error][protocol][protocol_core]") {
    std::vector<std::string> test_strings;
    test_strings.push_back("AAA");
    test_strings.push_back("BBB");

    test::multi_send_and_recv<error_test_protocol>(test_strings, test_strings);
}

struct disconnect_test_protocol
    : twisted::basic_protocol<disconnect_test_protocol> {

    disconnect_test_protocol(bool& disconnected)
        : _disconnected(disconnected) {}

    void on_message(const_buffer_iterator /* begin */,
                    const_buffer_iterator /* end */) {}

    void on_disconnect() { _disconnected = true; }

    bool& _disconnected;
};

TEST_CASE("on_disconnect test - disconnect from peer",
          "[tcp][protocol_core][on_disconnect]") {
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

struct local_disconnect_test_protocol
    : twisted::basic_protocol<local_disconnect_test_protocol> {

    local_disconnect_test_protocol(bool& disconnected)
        : _disconnected(disconnected) {}

    void on_message(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);
        lose_connection();
    }

    void on_disconnect() { _disconnected = true; }

    bool& _disconnected;
};

TEST_CASE("on_disconnect test - local disconnect",
          "[tcp][protocol_core][on_disconnect]") {
    bool disconnected = false;
    test::single_send_and_recv<local_disconnect_test_protocol>(
        "AAA", "AAA",
        [&] { return local_disconnect_test_protocol(disconnected); });
    CHECK(disconnected == true);
}

struct call_test_protocol : twisted::basic_protocol<call_test_protocol> {
    void on_message(const_buffer_iterator begin, const_buffer_iterator end) {
        call([=]() { send_message(begin, end); });
    }
};

TEST_CASE("call test", "[protocol_core][call]") {
    test::single_send_and_recv<call_test_protocol>(
        "TEST123", "TEST123", twisted::default_factory<call_test_protocol>());
}

struct call_from_thread_test_protocol
    : twisted::basic_protocol<call_from_thread_test_protocol> {
    call_from_thread_test_protocol(bool& flag) : _flag(flag) {}

    void on_message(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);
        auto self = shared_from_this();
        call_from_thread([&, self]() {
            // Don't ever do this in user code - it's not the way
            // call_from_thread is supposed to be used
            _flag = true;
        });
    }

    bool& _flag;
};

TEST_CASE("call from thread test", "[protocol_core][call_from_thread]") {
    bool flag = false;
    test::single_send_and_recv<call_from_thread_test_protocol>(
        "AAA", "AAA", [&] { return call_from_thread_test_protocol(flag); });
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    CHECK(flag == true);
}

struct forward_test_protocol : twisted::basic_protocol<forward_test_protocol> {
    std::vector<forward_test_protocol*>& _clients;
    forward_test_protocol(std::vector<forward_test_protocol*>& clients)
        : _clients(clients) {}

    forward_test_protocol(forward_test_protocol&& other)
        : _clients(other._clients) {
        _clients.push_back(this);
    }

    void on_message(const_buffer_iterator begin, const_buffer_iterator end) {
        for (auto&& client : _clients) {
            if (client != this) {
                forward(*client, begin, end);
            }
        }
    };
};

TEST_CASE("forward", "[protocol_core][forward]") {
    twisted::reactor reac;
    std::vector<forward_test_protocol*> clients;
    auto fut = std::async(std::launch::async, [&]() {
        reac.listen_tcp(50000,
                        [&]() { return forward_test_protocol(clients); });
        reac.run();
    });

    boost::asio::io_service io_service;
    tcp::socket socket_sender(io_service);
    tcp::socket socket_receiver(io_service);

    BOOST_SCOPE_EXIT_TPL((&reac)(&socket_sender)(&socket_receiver)) {
        reac.stop();
        if (socket_sender.is_open()) {
            socket_sender.close();
        }

        if (socket_receiver.is_open()) {
            socket_receiver.close();
        }
    }
    BOOST_SCOPE_EXIT_END

    std::this_thread::sleep_for(std::chrono::milliseconds(3));

    socket_receiver.connect(tcp::endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), 50000));
    socket_sender.connect(tcp::endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), 50000));

    std::this_thread::sleep_for(std::chrono::milliseconds(3));

    std::string send_buffer("XXX");
    boost::asio::write(socket_sender, boost::asio::buffer(send_buffer));

    std::string recv_buffer(send_buffer.size(), '\0');
    boost::asio::read(socket_receiver,
                      boost::asio::buffer(&recv_buffer[0], recv_buffer.size()));
    CHECK(send_buffer == recv_buffer);
}
