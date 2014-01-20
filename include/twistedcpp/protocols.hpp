#ifndef TWISTED_PROTOCOLS
#define TWISTED_PROTOCOLS

namespace twisted {

template<typename ChildProtocol>
class basic_protocol : public std::enable_shared_from_this<ChildProtocol> {
public:
    typedef boost::asio::ip::tcp::socket socket_type;
    typedef boost::asio::steady_timer timer_type;
    typedef boost::asio::io_service::strand strand_type;

    basic_protocol() {}

    void set_socket(socket_type socket) {
        _socket.reset(new socket_type(std::move(socket)));
        _strand = boost::in_place(boost::ref(_socket->get_io_service()));
        _timer =  boost::in_place(boost::ref(_socket->get_io_service()));
    }

    void run_protocol() {
        auto self = static_cast<ChildProtocol*>(this)->shared_from_this();
        boost::asio::spawn(*_strand, [this, self] (boost::asio::yield_context yield) {
            _yield = boost::in_place(yield);

            std::vector<char> buffer(1024, '\0');
            try {
                for(;;) {
                    auto bytes_read = _socket->async_read_some(
                        boost::asio::buffer(buffer), yield);
                    static_cast<ChildProtocol*>(this)->on_message(
                        buffer.begin(), std::next(buffer.begin(), bytes_read));
                }
            } catch (std::exception& e) {
                std::cout << "Exception in basic_protocol" << std::endl;
            }
        });
    }

    template<typename Iter>
    void send_message(Iter begin, Iter end) {
        boost::asio::async_write(
            *_socket,
            boost::asio::buffer(&*begin, std::distance(begin, end)),
            *_yield);
    }

private:
    boost::optional<boost::asio::yield_context> _yield;
    std::unique_ptr<socket_type> _socket; // unique_ptr as boost::optional has no move support
    boost::optional<timer_type> _timer;
    boost::optional<strand_type> _strand;
};

}


#endif
