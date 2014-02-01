#ifndef TWISTEDCPP_REACTOR_HPP
#define TWISTEDCPP_REACTOR_HPP

#include "exception.hpp"
#include "sockets.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/system/system_error.hpp>

#include <iostream>
#include <memory>
#include <iterator>
#include <functional>

namespace twisted {

class reactor {
public:
    void run() { _io_service.run(); }

    void stop() { _io_service.stop(); }

    template <typename ProtocolFactory>
    void listen_tcp(int port, ProtocolFactory factory) {
        using namespace std::placeholders;
        run_impl(port, std::move(factory),
                 std::bind(&reactor::run_impl_tcp<ProtocolFactory>, this, _1,
                           _2, _3));
    }

    template <typename ProtocolFactory>
    void listen_ssl(int port, ProtocolFactory factory) {
        using namespace std::placeholders;
        run_impl(port, std::move(factory),
                 std::bind(&reactor::run_impl_ssl<ProtocolFactory>, this, _1,
                           _2, _3));
    }

private:
    template <typename ProtocolFactory, typename Handler>
    void run_impl(int port, ProtocolFactory factory, Handler handler) {
        boost::asio::spawn(_io_service, [=](boost::asio::yield_context yield) {
            handler(port, factory, yield);
        });
    }

    template <typename ProtocolFactory>
    void run_impl_tcp(int port, ProtocolFactory factory,
                      boost::asio::yield_context yield) {
        using boost::asio::ip::tcp;
        tcp::acceptor acceptor(_io_service, tcp::endpoint(tcp::v4(), port));

        for (;;) {
            boost::system::error_code ec;
            std::unique_ptr<detail::tcp_socket> socket(
                new detail::tcp_socket(_io_service));
            acceptor.async_accept(socket->lowest_layer(), yield[ec]);

            if (ec) {
                throw boost::system::system_error(ec);
            }

            auto new_client = std::make_shared<decltype(factory())>(factory());
            // lazy init to avoid clutter in protocol constructors
            new_client->set_socket(std::move(socket));
            new_client->run_protocol();
        }
    }

    template <typename ProtocolFactory>
    void run_impl_ssl(int port, ProtocolFactory factory,
                      boost::asio::yield_context yield) {
        using boost::asio::ip::tcp;

        tcp::acceptor acceptor(_io_service, tcp::endpoint(tcp::v4(), port));
        boost::asio::ssl::context context(
            boost::asio::ssl::context::sslv3_server);
        context.set_options(boost::asio::ssl::context::default_workarounds |
                            boost::asio::ssl::context::no_sslv2 |
                            boost::asio::ssl::context::single_dh_use);
        context.set_password_callback([](
            std::size_t /* max_length */,
            boost::asio::ssl::context::password_purpose /* purpose */) {
            return "ikn1";
        });
        context.use_certificate_chain_file("server.crt");
        context.use_private_key_file("server.key",
                                     boost::asio::ssl::context::pem);

        for (;;) {
            boost::system::error_code ec;

            std::unique_ptr<detail::ssl_socket> socket(
                new detail::ssl_socket(_io_service, context));

            acceptor.async_accept(socket->lowest_layer(), yield[ec]);

            if (ec) {
                throw boost::system::system_error(ec);
            }

            auto new_client = std::make_shared<decltype(factory())>(factory());
            // lazy init to avoid clutter in protocol constructors
            new_client->set_socket(std::move(socket));
            new_client->run_protocol();
        }
    }

    boost::asio::io_service _io_service;
};

} // namespace twisted

#endif
