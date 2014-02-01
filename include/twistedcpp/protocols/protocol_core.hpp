#ifndef TWISTEDCPP_PROTOCOL_CORE
#define TWISTEDCPP_PROTOCOL_CORE

#include "../sockets.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/optional.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>

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

                for (;_socket->is_open();) {
                    auto bytes_read =
                        _socket->async_read_some(asio_buffer(), yield);
                    checked_on_message(buffer_begin(),
                                       std::next(buffer_begin(), bytes_read));
                }
            }
            catch (boost::system::system_error& connection_error) { // network
                                                                    // errors
                print_connection_error(connection_error);
                this_protocol().on_disconnect();
            }
            catch (const std::exception& excep) { // errors from user protocols
                print_exception_what(excep);
            }
        });
    }

    template <typename Iter>
    void send_message(Iter begin, Iter end) {
        _socket->async_write(boost::asio::buffer(&*begin, std::distance(begin, end)),
            *_yield);
    }

    void send_buffers(const std::array<boost::asio::const_buffer, 2>& buffers) {
        _socket->async_write(buffers, *_yield);
    }

    void on_disconnect() {}

    void on_error(std::exception_ptr eptr) { std::rethrow_exception(eptr); }

    void lose_connection() {
       _socket->close();
        std::cout << "Closing connection to client" << std::endl;
    }

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
    void print_connection_error(
        const boost::system::system_error& connection_error) const {
        std::cerr << "Client disconnected with code " << connection_error.what()
                  << std::endl;
    }

    void print_exception_what(const std::exception& excep) {
        std::cerr << "Killing connection, exception in client handler: "
                  << excep.what() << std::endl;
    }

    void checked_on_message(const_buffer_iterator begin,
                            const_buffer_iterator end) {
        try {
            this_protocol().on_message(begin, end);
        }
        catch (...) {
            this_protocol().on_error(std::current_exception());
        }
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
    std::unique_ptr<socket_type> _socket; // unique_ptr as boost::optional has
                                          // no move support
    boost::optional<strand_type> _strand;
};

} // namespace twisted

#endif
