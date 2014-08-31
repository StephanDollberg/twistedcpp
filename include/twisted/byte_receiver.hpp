#ifndef TWISTEDCPP_LINE_RECEIVER
#define TWISTEDCPP_LINE_RECEIVER

#include "basic_protocol.hpp"
#include "protocol_core.hpp"

#include <vector>

#include <boost/range/iterator_range.hpp>

namespace twisted {

template <typename ChildProtocol>
class byte_receiver : public twisted::protocol_core<ChildProtocol,
    boost::iterator_range<std::vector<char>::iterator>> {
public:
    typedef std::vector<char>::size_type size_type;
    typedef boost::iterator_range<std::vector<char>::iterator> buffer_type;
    typedef boost::iterator_range<std::vector<char>::const_iterator> const_buffer_type;

    byte_receiver(int start_bytes_size)
        : _next_bytes_size(start_bytes_size), _current_begin(0),
            _current_count(0), _read_buffer(3 * start_bytes_size) {}

    template <typename Iter>
    void on_message(Iter begin, Iter end) {
        _current_count += std::distance(begin, end);

        while(_current_count >= _next_bytes_size) {
            this->this_protocol().bytes_received(
                std::next(_read_buffer.begin(), _current_begin),
                std::next(_read_buffer.begin(), _current_begin + _next_bytes_size));

            _current_count -= _next_bytes_size;
            _current_begin += _next_bytes_size;
        }

        if(_current_begin + _current_count == _read_buffer.size()) {
            std::copy(
                std::next(_read_buffer.begin(), _current_begin),
                _read_buffer.end(),
                _read_buffer.begin());

            _current_begin = 0;
        }
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

private:
    size_type _next_bytes_size; // size per block
    size_type _current_begin; // start-position of not processed data in buffer
    size_type _current_count; // current amount of not processed data
    std::vector<char> _read_buffer;
};
}

#endif
