#ifndef TWISTEDCPP_SOCKETS_HPP
#define TWISTEDCPP_SOCKETS_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>

#include <iostream>
namespace twisted {

namespace detail {
class socket_base {
public:
    virtual void do_handshake(boost::asio::yield_context) = 0;
    virtual std::size_t async_read_some(boost::asio::mutable_buffers_1,
                                        boost::asio::yield_context) = 0;
    virtual bool is_open() const = 0;
    virtual void close() = 0;
    virtual void async_write(boost::asio::const_buffers_1,
                             boost::asio::yield_context) = 0;
    virtual void async_write(std::array<boost::asio::const_buffer, 2>,
                             boost::asio::yield_context) = 0;
    virtual void async_write(boost::asio::mutable_buffers_1,
                             boost::asio::yield_context) = 0;
    virtual void async_write(std::array<boost::asio::mutable_buffer, 2>,
                             boost::asio::yield_context) = 0;
    virtual boost::asio::io_service& get_io_service() = 0;
    virtual ~socket_base() {}
};

class tcp_socket : public socket_base {
public:
    tcp_socket(boost::asio::io_service& io_service) : _socket(io_service) {}

    virtual void do_handshake(boost::asio::yield_context /* context */) {
        // noop
    }

    virtual std::size_t async_read_some(boost::asio::mutable_buffers_1 buffers,
                                        boost::asio::yield_context yield) {
        return _socket.async_read_some(buffers, yield);
    }

    virtual void async_write(boost::asio::const_buffers_1 buffers,
                             boost::asio::yield_context yield) {
        boost::asio::async_write(_socket, buffers, yield);
    }

    virtual void async_write(std::array<boost::asio::const_buffer, 2> buffers,
                             boost::asio::yield_context yield) {

        boost::asio::async_write(_socket, buffers, yield);
    }

    virtual void async_write(boost::asio::mutable_buffers_1 buffers,
                             boost::asio::yield_context yield) {
        boost::asio::async_write(_socket, buffers, yield);
    }
    virtual void async_write(std::array<boost::asio::mutable_buffer, 2> buffers,
                             boost::asio::yield_context yield) {
        boost::asio::async_write(_socket, buffers, yield);
    }

    virtual bool is_open() const { return _socket.is_open(); }

    virtual void close() { _socket.close(); }

    virtual boost::asio::io_service& get_io_service() {
        return _socket.get_io_service();
    }

    boost::asio::ip::tcp::socket& lowest_layer() { return _socket; }

    virtual ~tcp_socket() {}

private:
    boost::asio::ip::tcp::socket _socket;
};

class ssl_socket : public socket_base {
public:
    ssl_socket(boost::asio::io_service& io_service,
               boost::asio::ssl::context& context)
        : _socket(io_service, context) {}

    virtual void do_handshake(boost::asio::yield_context yield) {
        _socket.async_handshake(boost::asio::ssl::stream_base::server, yield);
    }

    virtual std::size_t async_read_some(boost::asio::mutable_buffers_1 buffers,
                                        boost::asio::yield_context yield) {
        return _socket.async_read_some(buffers, yield);
    }

    virtual void async_write(boost::asio::const_buffers_1 buffers,
                             boost::asio::yield_context yield) {
        boost::asio::async_write(_socket, buffers, yield);
    }

    virtual void async_write(std::array<boost::asio::const_buffer, 2> buffers,
                             boost::asio::yield_context yield) {

        boost::asio::async_write(_socket, buffers, yield);
    }

    virtual void async_write(boost::asio::mutable_buffers_1 buffers,
                             boost::asio::yield_context yield) {
        boost::asio::async_write(_socket, buffers, yield);
    }

    virtual void async_write(std::array<boost::asio::mutable_buffer, 2> buffers,
                             boost::asio::yield_context yield) {
        boost::asio::async_write(_socket, buffers, yield);
    }

    virtual bool is_open() const { return _socket.lowest_layer().is_open(); }

    virtual void close() { _socket.lowest_layer().close(); }

    virtual boost::asio::io_service& get_io_service() {
        return _socket.get_io_service();
    }

    boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::lowest_layer_type&
    lowest_layer() {
        return _socket.lowest_layer();
    }

    virtual ~ssl_socket() {}

private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> _socket;
};
} // namespace detail

} // namespace twisted

#endif
