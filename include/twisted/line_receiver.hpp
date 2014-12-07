#ifndef TWISTEDCPP_LINE_RECEIVER
#define TWISTEDCPP_LINE_RECEIVER

#include "protocol_core.hpp"
#include "detail/line_receiver_parser.hpp"

#include <boost/container/vector.hpp>

namespace twisted {

/**
 * @brief Protocol to receive lines delimited by a certain delimiter
 *
 * Provided callbacks:
 *
 * void line_received(const_buffer_iterator begin, const_buffer_iterator end);
 *
 * Will be called whenever a line delimited by the delimiter has been received.
 *[begin, end) is the data
 * range. The range does not include the delimiter.
 */
template <typename ChildProtocol>
class line_receiver
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
     * @brief Sets the delimiter
     * @param delimiter delimiter
     */
    line_receiver(std::string delimiter = std::string("\r\n"))
        : _current_begin(0), _current_count(0),
          _delimiter(std::move(delimiter)), _line_buffer(32) {}

    template <typename Iter>
    void on_message(Iter begin, Iter end) {
        line::parse(begin, end, _current_begin, _current_count, _delimiter,
                    _line_buffer, this->this_protocol());
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
            std::next(_line_buffer.begin(), _current_begin + _current_count),
            _line_buffer.end());
    }

    const const_buffer_type& read_buffer() const {
        return boost::make_iterator_range(
            std::next(_line_buffer.begin(), _current_begin + _current_count),
            _line_buffer.end());
    }

private:
    // TODO evaluate whether copying each time is faster than tracking
    // _current_begin
    size_type _current_begin; // start-position of not processed data in buffer
    size_type _current_count; // current amount of not processed data
    const std::string _delimiter;
    internal_buffer_type _line_buffer;
};
}

#endif
