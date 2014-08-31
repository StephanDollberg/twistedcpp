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
    typedef std::vector<char> internal_buffer_type;
    typedef internal_buffer_type::size_type size_type;
    typedef boost::iterator_range<internal_buffer_type::iterator> buffer_type;
    typedef boost::iterator_range<internal_buffer_type::const_iterator> const_buffer_type;

    byte_receiver(int start_bytes_size)
        : _next_bytes_size(start_bytes_size), _current_begin(0),
            _current_count(0), _read_buffer(calculate_buffer_size(start_bytes_size)) {}

    template <typename Iter>
    void on_message(Iter begin, Iter end) {
        _current_count += std::distance(begin, end);

        while(_current_count >= _next_bytes_size) {
            /* protocol user can change package size in between calls
                we need to cache the old package-size for the _current_* updates */
            size_type old_package_size = _next_bytes_size;

            this->this_protocol().bytes_received(
                std::next(_read_buffer.begin(), _current_begin),
                std::next(_read_buffer.begin(), _current_begin + _next_bytes_size));

            _current_count -= old_package_size;
            _current_begin += old_package_size;
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

    void set_package_size(size_type package_size) {
        if(_next_bytes_size < package_size) {
            _read_buffer.resize(calculate_buffer_size(package_size));
        }

        _next_bytes_size = package_size;
    }

private:

    constexpr size_type calculate_buffer_size(size_type new_min_size) const {
        return 3 * new_min_size;
    }

    size_type _next_bytes_size; // size per block
    size_type _current_begin; // start-position of not processed data in buffer
    size_type _current_count; // current amount of not processed data
    internal_buffer_type _read_buffer;
};
}

#endif
