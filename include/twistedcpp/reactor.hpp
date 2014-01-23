#ifndef TWISTEDCPP_REACTOR_HPP
#define TWISTEDCPP_REACTOR_HPP

#include "exception.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/system/system_error.hpp>
#include <iostream>
#include <memory>
#include <iterator>


namespace twisted {

typedef boost::asio::io_service reactor;

template<typename Protocol, typename ...ProtocolArgs>
void run_impl(reactor& reac, int port, ProtocolArgs&&... protocol_args) {
    using boost::asio::ip::tcp;

    boost::asio::spawn(reac,
        [&](boost::asio::yield_context yield)
        {
            tcp::acceptor acceptor(reac,
                tcp::endpoint(tcp::v4(), port));

            for (;;) {
                boost::system::error_code ec;
                tcp::socket socket(reac);
                acceptor.async_accept(socket, yield[ec]);
                if (!ec) {
                    auto new_client =
                        std::make_shared<Protocol>(std::forward<ProtocolArgs>(protocol_args)...);
                    // lazy init to avoid clutter in protocol constructors
                    new_client->set_socket(std::move(socket));
                    new_client->run_protocol();
                }
                else {
                    throw boost::system::system_error(ec);
                }
            }
        });
}

template<typename Protocol, typename ...ProtocolArgs>
void run(reactor& reac, int port, ProtocolArgs&&... protocol_args) {
    run_impl<Protocol>(reac, port, std::forward<ProtocolArgs>(protocol_args)...);
}

template<typename Protocol, typename ...ProtocolArgs>
void run(int port, ProtocolArgs&&... protocol_args) {
    reactor reac;
    run_impl<Protocol>(reac, port, std::forward<ProtocolArgs>(protocol_args)...);
    reac.run();
}

}

#endif
