#ifndef TWISTEDCPP_REACTOR_HPP
#define TWISTEDCPP_REACTOR_HPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <boost/container/deque.hpp>
#include <iostream>
#include <memory>
#include <iterator>

namespace twisted {

template<typename Protocol, typename ...ProtocolArgs>
void run(int port, ProtocolArgs&&... protocol_args) {
    using boost::asio::ip::tcp;

    boost::asio::io_service io_service;

    boost::asio::spawn(io_service,
        [&](boost::asio::yield_context yield)
        {
            tcp::acceptor acceptor(io_service,
                tcp::endpoint(tcp::v4(), port));

            for (;;) {
                boost::system::error_code ec;
                tcp::socket socket(io_service);
                acceptor.async_accept(socket, yield[ec]);
                if (!ec) {
                    auto new_client = 
                        std::make_shared<Protocol>(std::forward<ProtocolArgs>(protocol_args)...);
                    // lazy init to avoid clutter in protocol constructors
                    new_client->set_socket(std::move(socket));
                    new_client->run_protocol();
                }
            }
        });

    io_service.run();
}

}

#endif 
