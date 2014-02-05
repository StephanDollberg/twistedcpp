#ifndef TWISTEDCPP_REACTOR_HPP
#define TWISTEDCPP_REACTOR_HPP

#include "exception.hpp"
#include "sockets.hpp"
#include "ssl_options.hpp"

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
        run_impl_tcp(port, std::move(factory));
    }

    template <typename ProtocolFactory>
    void listen_ssl(int port, ProtocolFactory factory, ssl_options ssl_ops) {
        using namespace std::placeholders;
        run_impl_ssl(port, std::move(factory), std::move(ssl_ops));
    }

private:
    template <typename ProtocolFactory>
    void run_impl_tcp(int port, ProtocolFactory factory) {
        boost::asio::spawn(_io_service, [=](boost::asio::yield_context yield) {
            using boost::asio::ip::tcp;
            tcp::acceptor acceptor(_io_service, tcp::endpoint(tcp::v4(), port));
            run_impl_tcp_core(acceptor, factory, yield);
        });
    }

    template <typename ProtocolFactory>
    void run_impl_ssl(int port, ProtocolFactory factory, ssl_options ssl_ops) {
        boost::asio::spawn(_io_service, [=](boost::asio::yield_context yield) {
            using boost::asio::ip::tcp;
            tcp::acceptor acceptor(_io_service, tcp::endpoint(tcp::v4(), port));
            run_impl_ssl_core(acceptor, factory, yield, ssl_ops);
        });
    }

    template <typename ProtocolFactory>
    void run_impl_tcp_core(boost::asio::ip::tcp::acceptor& acceptor,
                           ProtocolFactory factory,
                           boost::asio::yield_context yield) {

        auto socket_factory = [=]() {
            return std::unique_ptr<detail::tcp_socket>(
                new detail::tcp_socket(_io_service));
        };

        run_loop(acceptor, std::move(factory), yield, socket_factory);
    }

    template <typename ProtocolFactory>
    void run_impl_ssl_core(boost::asio::ip::tcp::acceptor& acceptor,
                           ProtocolFactory factory,
                           boost::asio::yield_context yield,
                           ssl_options ssl_ops) {
        using boost::asio::ip::tcp;

        auto socket_factory = [=] {
            boost::asio::ssl::context _context(boost::asio::ssl::context::sslv23_server);
            _context.set_options(
                boost::asio::ssl::context::default_workarounds |
                boost::asio::ssl::context::no_sslv2 |
                boost::asio::ssl::context::single_dh_use);
            _context.set_password_callback([=](
                std::size_t /* max_length */,
                boost::asio::ssl::context::password_purpose /* purpose */) {
                return ssl_ops.password();
            });
            _context.use_certificate_chain_file(ssl_ops.certificate());
            _context.use_private_key_file(ssl_ops.key(),
                                          boost::asio::ssl::context::pem);

            return std::unique_ptr<detail::ssl_socket>(
                new detail::ssl_socket(_io_service, _context));
        };

        run_loop(acceptor, std::move(factory), yield, socket_factory);
    }

    template <typename ProtocolFactory, typename SocketFactory>
    void run_loop(boost::asio::ip::tcp::acceptor& acceptor,
                  ProtocolFactory factory, boost::asio::yield_context yield,
                  SocketFactory socket_factory) {

        for (;;) {
            boost::system::error_code ec;

            auto socket = socket_factory();

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
