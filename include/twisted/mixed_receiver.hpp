#ifndef TWISTEDCPP_MIXED_RECEIVER
#define TWISTEDCPP_MIXED_RECEIVER

#include "protocol_core.hpp"
#include "detail/line_receiver_parser.hpp"
#include "detail/byte_receiver_parser.hpp"

#include <boost/container/vector.hpp>

namespace twisted {


/**
 * @brief Protocol that combines the line_receiver and the byte_receiver
 *
 * You can switch between the two modes by calling
 *
 * void set_byte_mode() and void set_line_mode();
 *
 * Provided callbacks:
 *
 * void bytes_received(const_buffer_iterator begin, const_buffer_iterator end);
 *
 * Will be called whenever N bytes have been received. [begin, end) is the data
 * range. (In byte mode)
 *
 * void line_received(const_buffer_iterator begin, const_buffer_iterator end);
 *
 * Will be called whenever a line delimited by the delimiter has been received.
 * [begin, end) is the data
 * range. The range does not include the delimiter. (In line mode)
 */
template <typename ChildProtocol>
class mixed_receiver
    : public twisted::protocol_core<
          ChildProtocol,
          boost::iterator_range<boost::container::vector<char>::iterator>> {
public:
    typedef boost::container::vector<char> internal_buffer_type;
    typedef internal_buffer_type::size_type size_type;
    typedef boost::iterator_range<internal_buffer_type::iterator> buffer_type;
    typedef boost::iterator_range<internal_buffer_type::const_iterator>
    const_buffer_type;

    /**
     * @brief constructor
     * @param delimiter delimiter for the string mode
     */
    mixed_receiver(std::string delimiter = std::string("\r\n"))
        : is_byte_mode(false), _current_begin(0), _current_count(0),
          _delimiter(std::move(delimiter)), _next_bytes_size(0),
          _read_buffer(32) {}

    template <typename Iter>
    void on_message(Iter begin, Iter end) {
        _current_count += std::distance(begin, end);

        bool process = false;

        Iter line_start;
        Iter search_iter;

        process = loop_condition(search_iter, end, line_start, _current_begin,
                                 _read_buffer, _delimiter);

        while (process) {
            if (is_byte_mode) {
                byte::core_loop(_current_count, _current_begin, _read_buffer,
                                _next_bytes_size, this->this_protocol());
            } else {
                line::core_loop(line_start, search_iter, _delimiter,
                                _current_count, _current_begin,
                                this->this_protocol());
            }

            process = loop_condition(search_iter, end, line_start,
                                     _current_begin, _read_buffer, _delimiter);
        }

        if (is_byte_mode) {
            byte::prepare_buffers(_current_count, _current_begin, _read_buffer);
        } else {
            line::prepare_buffers(_current_count, _current_begin, _read_buffer);
        }
    }

    /**
     * @brief Sends a line. Automatically adds the current delimiter
     * @param begin range begin
     * @param end range end
     */
    template <typename Iter>
    void send_line(Iter begin, Iter end) {
        std::array<boost::asio::const_buffer, 2> buffers{
            { boost::asio::buffer(&*begin, std::distance(begin, end)),
              boost::asio::buffer(&*_delimiter.begin(), _delimiter.size()) }
        };

        this->send_buffers(buffers);
    }

    buffer_type read_buffer() {
        return boost::make_iterator_range(
            std::next(_read_buffer.begin(), _current_begin + _current_count),
            _read_buffer.end());
    }

    const const_buffer_type& read_buffer() const {
        return boost::make_iterator_range(
            std::next(_read_buffer.begin(), _current_begin + _current_count),
            _read_buffer.end());
    }

    /**
     * @brief puts the mixed receiver into byte mode
     */
    void set_byte_mode() { is_byte_mode = true; }

    /**
     * @brief puts the mixed receiver into line mode
     */
    void set_line_mode() { is_byte_mode = false; }

    /**
     * @brief sets the size in bytes of the next packet to read (byte mode)
     */
    void set_package_size(size_type package_size) {
        byte::set_package_size(package_size, _next_bytes_size, _read_buffer);
    }

private:
    template <typename Iter>
    bool loop_condition(Iter& search_iter, Iter end, Iter& line_start,
                        std::size_t _current_begin,
                        boost::container::vector<char>& _read_buffer,
                        const std::string& _delimiter) const {
        if (is_byte_mode) {
            return byte::loop_condition(_current_count, _next_bytes_size);
        } else {
            return line::loop_condition(search_iter, end, line_start,
                                        _current_begin, _read_buffer,
                                        _delimiter);
        }
    }

    // TODO evaluate whether copying each time is faster than tracking
    // _current_begin
    bool is_byte_mode;
    size_type _current_begin; // start-position of not processed data in buffer
    size_type _current_count; // current amount of not processed data
    const std::string _delimiter;
    size_type _next_bytes_size;
    internal_buffer_type _read_buffer;
};
}

#endif
