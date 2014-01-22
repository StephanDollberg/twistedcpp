#ifndef TWISTEDCPP_BASIC_PROTOCOL
#define TWISTEDCPP_BASIC_PROTOCOL

namespace twisted {

template<typename ChildProtocol>
class basic_protocol : public std::enable_shared_from_this<ChildProtocol> {
public:
    typedef boost::asio::ip::tcp::socket socket_type;
    typedef boost::asio::steady_timer timer_type;
    typedef boost::asio::io_service::strand strand_type;
    typedef std::vector<char> buffer_type;
    typedef buffer_type::iterator buffer_iterator;
    typedef buffer_type::const_iterator const_buffer_iterator;

    basic_protocol()
        : _read_buffer(1024),
            _read_index(_read_buffer.begin()) {}

    void set_socket(socket_type socket) {
        _socket.reset(new socket_type(std::move(socket)));
        _strand = boost::in_place(boost::ref(_socket->get_io_service()));
        _timer =  boost::in_place(boost::ref(_socket->get_io_service()));
    }

    void run_protocol() {
        auto self = this_protocol().shared_from_this();
        boost::asio::spawn(*_strand, [this, self] (boost::asio::yield_context yield) {
            _yield = boost::in_place(yield);

            try {
                for(;_socket->is_open();) {
                    expand_buffer();
                    auto bytes_read = _socket->async_read_some(read_buffer(), yield);
                    checked_on_message(_read_index, std::next(_read_index, bytes_read));
                }
            } catch(boost::system::system_error& connection_error) { // network errors
                print_connection_error(connection_error);
                this_protocol().on_disconnect();
            } catch(const std::exception& excep) { // errors from user protocols
                print_exception_what(excep);
            }
        });
    }

    boost::asio::mutable_buffers_1 read_buffer() {
        return boost::asio::buffer(&*_read_index, std::distance(_read_index, _read_buffer.end()));
    }

    template<typename Iter>
    void send_message(Iter begin, Iter end) {
        boost::asio::async_write(
            *_socket,
            boost::asio::buffer(&*begin, std::distance(begin, end)),
            *_yield);
    }

    void on_disconnect() {}

    void on_error(std::exception_ptr eptr) {
        std::rethrow_exception(eptr);
    }

    void lose_connection() {
        _socket->close();
        std::cout << "Closing connection to client" << std::endl;
    }

private:
    void print_connection_error(const boost::system::system_error& connection_error) const {
        std::cerr << "Client disconnected with code "
                  << connection_error.what()
                  << std::endl;
    }

    void print_exception_what(const std::exception& excep) {
        std::cerr << "Killing connection, exception in client handler: "
                  << excep.what()
                  << std::endl;
    }

    void expand_buffer() {
        auto range = std::distance(_read_buffer.begin(), _read_index);
        if(range < 1024) {
            _read_buffer.resize(_read_buffer.size() * 2);
            _read_index = std::next(_read_buffer.begin(), range);
        }
    }

    template<typename Iter>
    void checked_on_message(Iter begin, Iter end) {
        try {
            this_protocol().on_message(begin, end);
        } catch (...) {
            this_protocol().on_error(std::current_exception());
        }
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

protected:
    buffer_type _read_buffer;
    buffer_iterator _read_index;
private:
    boost::optional<boost::asio::yield_context> _yield;
    std::unique_ptr<socket_type> _socket; // unique_ptr as boost::optional has no move support
    boost::optional<timer_type> _timer;
    boost::optional<strand_type> _strand;
};

}


#endif
