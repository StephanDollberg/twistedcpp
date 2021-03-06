#ifndef TWISTEDCPP_PROTOCOL_CORE
#define TWISTEDCPP_PROTOCOL_CORE

#include "detail/sockets.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/optional.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/ref.hpp>
#include <boost/utility/in_place_factory.hpp>

#ifndef NDEBUG
#include <iostream>
#endif

namespace twisted {

/**
 * @brief Core buffer interface of all protocols. Provides all the basic
 * functions like deferreds or send_message. You will only inherit from this if
 * you use the buffer interface. Otherwise you probably want the basic_protocol
 * or other more specialized protocols.
 * @tparam ChildProtocol CRTP child protocol parameter
 * @tparam BufferType Buffer interface buffer type
 */
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
    }

    void run_protocol() {
        auto self = this_protocol().shared_from_this();
        boost::asio::spawn(_socket->get_io_service(),
                           [this, self](boost::asio::yield_context yield) {
            _yield = boost::in_place(yield);

            try {
                _socket->do_handshake(*_yield);

                while (_socket->is_open()) {
                    auto bytes_read =
                        _socket->async_read_some(asio_buffer(), yield);
                    checked_on_message(buffer_begin(),
                                       std::next(buffer_begin(), bytes_read));
                }
            }
            catch (boost::system::system_error& connection_error) {
                handle_network_error(connection_error);
            }

            this_protocol().on_disconnect();
        });
    }

    /**
     * @brief Runs the callback after the given duration in the context of the
     * current protocol instance.
     * @param duration std::chrono duration
     * @param callable Callback
     */
    template <typename Duration, typename Callable>
    void call_later(Duration duration, Callable callable) {
        auto self = this_protocol().shared_from_this();
        // we pass our existing yield_context and not the io_service such that
        // both contexts run under the same strand
        boost::asio::spawn(*_yield, [this, duration, callable, self](
                                        boost::asio::yield_context yield) {
            boost::asio::high_resolution_timer timer(_socket->get_io_service(),
                                                     duration);
            timer.async_wait(yield);

            // we need to exchange the yield context so that that the user can
            // use the send_message function in the right context
            boost::optional<boost::asio::yield_context> old_yield(
                boost::in_place(*_yield));
            _yield = boost::in_place(yield);
            callable();
            _yield = boost::in_place(*old_yield);
        });
    }

    /**
     * @brief Equal to call_later(std::chrono::seconds(0));
     */
    template <typename Callable>
    void call(Callable callable) {
        call_later(std::chrono::seconds(0), std::move(callable));
    }

    /**
     * @brief Executes the given callback with the passed args out of the
     * context of the current protocol instance in a different thread.
     * @param callable Callback to execute
     * @param args Arguments to pass the callback
     */
    template <typename Callable, typename... Args>
    void call_from_thread(Callable&& callable, Args&&... args) {
        _socket->get_io_service().post(std::bind(
            std::forward<Callable>(callable), std::forward<Args>(args)...));
    }

    /**
     * @brief Waits/Sleeps for the given duration
     * @param duration std::chrono duration
     */
    template <typename Duration>
    void wait_for(const Duration& duration) const {
        boost::asio::high_resolution_timer timer(_socket->get_io_service(),
                                                 duration);
        timer.async_wait(*_yield);
    }

    /**
     * @brief Send data to the connected client
     * @param begin Data range begin
     * @param end Data range end
     */
    template <typename Iter>
    void send_message(Iter begin, Iter end) {
        _socket->async_write(
            boost::asio::buffer(&*begin, std::distance(begin, end)), *_yield);
    }

    /**
     * @brief Sends data on the connection of the passed protocol instance
     * @param other Protocol instance to which the data shall be sent
     * @param begin Data range begin
     * @param end Data range end
     */
    template <typename Iter>
    void forward(const protocol_core& other, Iter begin, Iter end) {
        other._socket->async_write(
            boost::asio::buffer(&*begin, std::distance(begin, end)), *_yield);
    }

    /**
     * @brief Scatter/gather sending.
     * @param buffers Valid <a
     * href="https://www.boost.org/doc/libs/1_57_0/doc/html/boost_asio/overview/core/buffers.html">boost::asio
     * buffers</a>
     */
    template <typename ConstBufferSequence>
    void send_buffers(const ConstBufferSequence& buffers) {
        _socket->async_write(buffers, *_yield);
    }

    /**
     * @brief Actively request more data without returning to the callback.
     * Reads end - begin bytes of data
     * @param begin Begin of the buffer
     * @param end End of the buffer
     */
    template <typename Iter>
    void read_more(Iter begin, Iter end) {
        _socket->async_read(
            boost::asio::buffer(&*begin, std::distance(begin, end)), *_yield);
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

    /**
     * @brief returns whether the connection is still open
     */
    bool is_connected() const { return _socket->is_open(); }

    /**
     * @brief closes the connection
     */
    void lose_connection() { _socket->close(); }

    /**
     * @brief CRTP wrapper for derived class access
     */
    ChildProtocol& this_protocol() {
        return *static_cast<ChildProtocol*>(this);
    }

    /**
     * @brief CRTP wrapper for derived class access
     */
    const ChildProtocol& this_protocol() const {
        return *static_cast<ChildProtocol*>(this);
    }

private:
    void checked_on_message(const_buffer_iterator begin,
                            const_buffer_iterator end) {
        try {
            this_protocol().on_message(begin, end);
        }
        catch (...) {
            this_protocol().on_error(std::current_exception());
        }
    }

#ifdef NDEBUG
    void handle_network_error(boost::system::system_error&) {
#else
    void handle_network_error(boost::system::system_error& connection_error) {
        print_connection_error(connection_error);
#endif
    }

    void handle_user_error(const std::exception_ptr& excep) {
        this_protocol().on_error(excep);
    }

    void print_connection_error(
        const boost::system::system_error& connection_error) const {
        std::cerr << "Client disconnected with code " << connection_error.what()
                  << std::endl;
    }

    void print_exception_what(const std::exception& excep) const {
        std::cerr << "Killing connection, exception in client handler: "
                  << excep.what() << std::endl;
    }

    buffer_iterator buffer_begin() {
        using std::begin;
        return begin(this_protocol().read_buffer());
    }

    buffer_iterator buffer_begin() const {
        using std::begin;
        return begin(this_protocol().read_buffer());
    }

    buffer_iterator buffer_end() {
        using std::end;
        return end(this_protocol().read_buffer());
    }

    buffer_iterator buffer_end() const {
        using std::end;
        return end(this_protocol().read_buffer());
    }

    boost::asio::mutable_buffers_1 asio_buffer() {
        return boost::asio::buffer(&*buffer_begin(),
                                   std::distance(buffer_begin(), buffer_end()));
    }

private:
    boost::optional<boost::asio::yield_context> _yield;
    std::unique_ptr<socket_type> _socket;
};

} // namespace twisted

#endif
