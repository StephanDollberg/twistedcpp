#ifndef TWISTEDCPP_REACTOR_HPP
#define TWISTEDCPP_REACTOR_HPP

#include "detail/sockets.hpp"
#include "ssl_options.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/system/system_error.hpp>

#include <memory>
#include <thread>

namespace twisted {

/**
 * @brief Reactor class to listen on certain protocols
 */
class reactor {
public:
    /**
     * @brief Starts the reactor
     * @param thread_count Number of threads the reactor shall use
     */
    void run(int thread_count = 1) {
        for (int i = 0; i != thread_count - 1; ++i) {
            worker_threads.emplace_back([&] { _io_service.run(); });
        }

        _io_service.run();
    }

    /**
     * @brief Stops the reactor
     */
    void stop() {
        _io_service.stop();

        for (auto&& t : worker_threads) {
            t.join();
        }
    }

    /**
     * @brief Registers a tcp listener
     * @param port Port to listen on
     * @param args Arguments that will be passed to the protocol constructor
     */
    template <typename ProtocolType, typename... Args>
    void listen_tcp(int port, Args&&... args) {
        boost::asio::spawn(_io_service, [&, port](boost::asio::yield_context yield) {
            using boost::asio::ip::tcp;
            tcp::acceptor acceptor(_io_service, tcp::endpoint(tcp::v4(), port));
            tcp_core<ProtocolType>(acceptor, yield, std::forward<Args>(args)...);
        });
    }

    /**
     * @brief Registers a ssl/tls listener
     * @param port Port to listen on
     * @param args Arguments that will be passed to the protocol constructor
     * @param ssl_opts <a
     * href="http://www.boost.org/doc/libs/1_57_0/doc/html/boost_asio/reference/ssl__context.html">boost::asio
     * ssl context</a> - sets the ssl/tls options
     */
    template <typename ProtocolType, typename... Args>
    void listen_ssl(int port, ssl_options&& ssl_opts, Args&&... args) {
        std::shared_ptr<ssl_options> ssl_opts_ptr // move capture
            = std::make_shared<ssl_options>(std::move(ssl_opts));
        boost::asio::spawn(_io_service, [&, ssl_opts_ptr, port](boost::asio::yield_context yield) {
            using boost::asio::ip::tcp;
            tcp::acceptor acceptor(_io_service, tcp::endpoint(tcp::v4(), port));
            ssl_impl<ProtocolType>(acceptor, yield, ssl_opts_ptr, std::forward<Args>(args)...);
        });
    }

private:
    template <typename ProtocolType, typename... Args>
    void tcp_core(boost::asio::ip::tcp::acceptor& acceptor,
                  boost::asio::yield_context yield, Args&&... args) {
        auto socket_factory = [=]() {
            return std::unique_ptr<detail::tcp_socket>(
                new detail::tcp_socket(_io_service));
        };

        run_loop<ProtocolType>(acceptor, yield, socket_factory, std::forward<Args>(args)...);
    }

    template <typename ProtocolType, typename... Args>
    void ssl_impl(boost::asio::ip::tcp::acceptor& acceptor,
                  boost::asio::yield_context yield,
                  std::shared_ptr<ssl_options> ssl_context, Args&&... args) {
        auto socket_factory = [&] {
            return std::unique_ptr<detail::ssl_socket>(
                new detail::ssl_socket(_io_service, *ssl_context));
        };

        run_loop<ProtocolType>(acceptor, yield, socket_factory, std::forward<Args>(args)...);
    }

    template <typename ProtocolType, typename SocketFactory, typename... Args>
    void run_loop(boost::asio::ip::tcp::acceptor& acceptor,
                  boost::asio::yield_context yield,
                  SocketFactory socket_factory, Args&&... args) {
        while (true) {
            auto socket = socket_factory();

            acceptor.async_accept(socket->lowest_layer(), yield);

            auto new_client = std::make_shared<ProtocolType>(std::forward<Args>(args)...);
            // lazy init to avoid clutter in protocol constructors
            new_client->set_socket(std::move(socket));
            new_client->run_protocol();
        }
    }

    boost::asio::io_service _io_service;
    std::vector<std::thread> worker_threads;
};

} // namespace twisted

#endif
