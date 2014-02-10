#ifndef TWISTEDCPP_TEST_UTILS_HPP
#define TWISTEDCPP_TEST_UTILS_HPP

#include "../include/twisted/reactor.hpp"
#include "../include/twisted/basic_protocol.hpp"
#include "../include/twisted/default_factory.hpp"

#include <boost/asio/read.hpp>
#include <boost/scope_exit.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <future>
#include <vector>
#include <string>

using boost::asio::ip::tcp;

namespace test {

template<typename ProtocolType>
void single_send_and_recv(std::string send, std::string recv) {
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

    boost::asio::write(socket, boost::asio::buffer(send));

    std::string buffer(recv.size(), '\0');
    boost::asio::read(socket, boost::asio::buffer(&buffer[0], buffer.size()));
    CHECK(buffer == recv);
}

} // namespace test


#endif // TWISTEDCPP_TEST_UTILS_HPP
