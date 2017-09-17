#include "catch/single_include/catch.hpp"

#include "test_utils.hpp"

#include "../include/twisted/reactor.hpp"
#include "../include/twisted/basic_protocol.hpp"

#include <boost/asio/read.hpp>
#include <boost/scope_exit.hpp>

#include <vector>
#include <string>
#include <future>

using boost::asio::ip::tcp;

struct echo_protocol : twisted::basic_protocol<echo_protocol> {
    void on_message(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);
    }
};

TEST_CASE("reactor threads test", "[tcp][reactor]") {
    twisted::reactor reac;
    auto fut = std::async(std::launch::async, [&]() {
        reac.listen_tcp<echo_protocol>(50000);
        reac.run(4);
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

    std::string send("hello threads");
    boost::asio::write(socket, boost::asio::buffer(send));

    std::string buffer(send.size(), '\0');
    boost::asio::read(socket, boost::asio::buffer(&buffer[0], buffer.size()));
    CHECK(buffer == send);
}
