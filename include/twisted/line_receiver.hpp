#ifndef TWISTEDCPP_LINE_RECEIVER
#define TWISTEDCPP_LINE_RECEIVER

#include "protocol_core.hpp"

#include <vector>

#include <boost/container/vector.hpp>

namespace twisted {

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

    line_receiver(std::string delimiter = std::string("\r\n"))
        : _current_count(0), _delimiter(std::move(delimiter)),
          _line_buffer(32) {}

    template <typename Iter>
    void on_message(Iter /*begin*/, Iter end) {
        auto line_start = _line_buffer.begin();
        auto search_iter = find_next(_line_buffer.begin(), _line_buffer.end());

        while (search_iter != _line_buffer.end()) {
            this->this_protocol().line_received(line_start, search_iter);
            line_start = std::next(search_iter, _delimiter.size());
            search_iter = find_next(line_start, _line_buffer.end());
        }

        _current_count = std::distance(line_start, end);

        if (_current_count == _line_buffer.size()) {
            expand_buffer();
        } else if (line_start != _line_buffer.begin()) {
            std::copy(line_start, end, _line_buffer.begin());
        }
    }

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
            std::next(_line_buffer.begin(), _current_count),
            _line_buffer.end());
    }

    const const_buffer_type& read_buffer() const {
        return boost::make_iterator_range(
            std::next(_line_buffer.begin(), _current_count),
            _line_buffer.end());
    }

private:
    template <typename Iter>
    Iter find_next(Iter begin, Iter end) const {
        return std::search(begin, end, _delimiter.begin(), _delimiter.end());
    }

    void expand_buffer() { _line_buffer.resize(_line_buffer.size() * 2); }

    size_type _current_count; // current amount of not processed data
    const std::string _delimiter;
    internal_buffer_type _line_buffer;
};
}

#endif
