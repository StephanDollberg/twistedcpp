#ifndef TWISTEDCPP_PROTOCOL_CORE
#define TWISTEDCPP_PROTOCOL_CORE

#include "detail/sockets.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/optional.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/ref.hpp>

#ifndef NDEBUG
#include <iostream>
#endif

namespace twisted {

template <typename ChildProtocol, typename BufferType>
class protocol_core : public std::enable_shared_from_this<ChildProtocol> {
public:
    typedef detail::socket_base socket_type;
    typedef boost::asio::steady_timer timer_type;
    typedef boost::asio::io_service::strand strand_type;
    typedef BufferType buffer_type;
    typedef typename buffer_type::iterator buffer_iterator;
    typedef typename buffer_type::const_iterator const_buffer_iterator;

    protocol_core() = default;
    protocol_core(protocol_core&&) = default;
    protocol_core& operator=(protocol_core&&) = default;

    void set_socket(std::unique_ptr<socket_type> socket) {
        _socket = std::move(socket);
        _strand = boost::in_place(boost::ref(_socket->get_io_service()));
    }

    void run_protocol() {
        auto self = this_protocol().shared_from_this();
        boost::asio::spawn(*_strand,
                           [this, self](boost::asio::yield_context yield) {
            _yield = boost::in_place(yield);

            try {
                _socket->do_handshake(*_yield);

                while (_socket->is_open()) {
                    auto bytes_read =
                        _socket->async_read_some(asio_buffer(), yield);
                    this_protocol().on_message(
                        buffer_begin(), std::next(buffer_begin(), bytes_read));
                }
            }
            catch (boost::system::system_error& connection_error) {
                handle_network_error(connection_error);
            }
            catch (...) {
                handle_user_error(std::current_exception());
            }
        });
    }

    template <typename Callable, typename... Args>
    void call_from_thread(Callable&& callable, Args&&... args) {
        _socket->get_io_service().post(std::bind(
            std::forward<Callable>(callable), std::forward<Args>(args)...));
    }

    template <typename Duration>
    void wait_for(const Duration& duration) const {
        boost::asio::high_resolution_timer timer(_socket->get_io_service(),
                                                 duration);
        timer.async_wait(*_yield);
    }

    template <typename Iter>
    void send_message(Iter begin, Iter end) {
        _socket->async_write(
            boost::asio::buffer(&*begin, std::distance(begin, end)), *_yield);
    }

    void send_buffers(const std::array<boost::asio::const_buffer, 2>& buffers) {
        _socket->async_write(buffers, *_yield);
    }

    /*
     * @brief default on_disconnect implementation; does nothing
     */
    void on_disconnect() {}

    /*
     * @brief default on_error implementation; swallows everything
     */
    void on_error(std::exception_ptr eptr) {
        try {
            std::rethrow_exception(eptr);
        }
        catch (const std::exception& excep) {
            print_exception_what(excep);
        }
    }

    void lose_connection() { _socket->close(); }

    /*
     * @brief CRTP wrapper for derived class access
     */
    ChildProtocol& this_protocol() {
        return *static_cast<ChildProtocol*>(this);
    }

    /*
     * @brief CRTP wrapper for derived class access
     */
    const ChildProtocol& this_protocol() const {
        return *static_cast<ChildProtocol*>(this);
    }

private:
#ifdef NDEBUG
    void handle_network_error(boost::system::system_error&) {
#else
    void handle_network_error(boost::system::system_error& connection_error) {
        print_connection_error(connection_error);
#endif
        this_protocol().on_disconnect();
    }

    void handle_user_error(const std::exception_ptr& excep) {
        this_protocol().on_error(excep);
    }

    void print_connection_error(
        const boost::system::system_error& connection_error) const {
        std::cerr << "Client disconnected with code " << connection_error.what()
                  << std::endl;
    }

    void print_exception_what(const std::exception& excep) {
        std::cerr << "Killing connection, exception in client handler: "
                  << excep.what() << std::endl;
    }

    buffer_iterator buffer_begin() {
        return this_protocol().read_buffer().begin();
    }

    buffer_iterator buffer_begin() const {
        return this_protocol().read_buffer().begin();
    }

    buffer_iterator buffer_end() { return this_protocol().read_buffer().end(); }

    buffer_iterator buffer_end() const {
        return this_protocol().read_buffer().end();
    }

    boost::asio::mutable_buffers_1 asio_buffer() {
        return boost::asio::buffer(&*buffer_begin(),
                                   std::distance(buffer_begin(), buffer_end()));
    }

private:
    boost::optional<boost::asio::yield_context> _yield;
    std::unique_ptr<socket_type> _socket;
    boost::optional<strand_type> _strand;
};

} // namespace twisted

#endif
