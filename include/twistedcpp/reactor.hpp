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

class reactor {
public:
    void run() { _io_service.run(); }

    void stop() { _io_service.stop(); }

    template <typename ProtocolFactory>
    void listen_tcp(int port, ProtocolFactory factory) {
        run_impl_tcp(port, std::move(factory));
    }

private:
    template <typename ProtocolFactory>
    void run_impl_tcp(int port, ProtocolFactory factory) {
        using boost::asio::ip::tcp;

        boost::asio::spawn(_io_service, [=](boost::asio::yield_context yield) {
            tcp::acceptor acceptor(_io_service, tcp::endpoint(tcp::v4(), port));

            for (;;) {
                boost::system::error_code ec;
                tcp::socket socket(_io_service);
                acceptor.async_accept(socket, yield[ec]);

                if (ec) {
                    throw boost::system::system_error(ec);
                }

                auto new_client =
                    std::make_shared<decltype(factory())>(factory());
                // lazy init to avoid clutter in protocol constructors
                new_client->set_socket(std::move(socket));
                new_client->run_protocol();
            }
        });
    }

    boost::asio::io_service _io_service;
};

} // namespace twisted

#endif
