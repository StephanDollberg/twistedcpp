#ifndef TWISTEDCPP_BYTE_RECEIVER
#define TWISTEDCPP_BYTE_RECEIVER

#include "basic_protocol.hpp"
#include "protocol_core.hpp"
#include "detail/byte_receiver_parser.hpp"

#include <vector>

#include <boost/range/iterator_range.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/container_fwd.hpp>

namespace twisted {

template <typename ChildProtocol>
class byte_receiver
    : public twisted::protocol_core<
          ChildProtocol,
          boost::iterator_range<boost::container::vector<char>::iterator>> {
public:
    typedef boost::container::vector<char> internal_buffer_type;
    typedef internal_buffer_type::size_type size_type;
    typedef boost::iterator_range<internal_buffer_type::iterator> buffer_type;
    typedef boost::iterator_range<internal_buffer_type::const_iterator>
    const_buffer_type;

    byte_receiver(int start_bytes_size)
        : _next_bytes_size(start_bytes_size), _current_begin(0),
          _current_count(0),
          _read_buffer(calculate_buffer_size(start_bytes_size)) {}

    template <typename Iter>
    void on_message(Iter begin, Iter end) {
        byte::parse(begin, end, _current_begin, _current_count,
                    _next_bytes_size, _read_buffer, this->this_protocol());
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
        if (_next_bytes_size < package_size) {
            _read_buffer.resize(calculate_buffer_size(package_size));
        }

        _next_bytes_size = package_size;

        // evaluate different kinds of copying existing data + buffer size here
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
