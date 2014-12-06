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
          _read_buffer(byte::calculate_buffer_size(start_bytes_size)) {}

    template <typename Iter>
    void on_message(Iter begin, Iter end) {
        byte::parse(begin, end, _current_begin, _current_count,
                    _next_bytes_size, _read_buffer, this->this_protocol());

        byte::prepare_buffers(_current_count, _current_begin, _read_buffer);
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
        byte::set_package_size(package_size, _next_bytes_size, _read_buffer);
    }

    buffer_type next_packet() {
        byte::prepare_buffers(_current_count, _current_begin, _read_buffer);

        if (_next_bytes_size < _current_count) {
            _current_begin += _next_bytes_size;
            _current_count -= _next_bytes_size;
        } else {
            this->read_more(std::next(_read_buffer.begin(),
                                      _current_begin + _current_count),
                            std::next(_read_buffer.begin(),
                                      _current_begin + _next_bytes_size));
            _current_count = 0;
        }

        auto ret = boost::make_iterator_range(
            std::next(_read_buffer.begin(), _current_begin),
            std::next(_read_buffer.begin(), _current_begin + _next_bytes_size));

        return ret;
    }

private:
    size_type _next_bytes_size; // size per block
    // TODO evaluate whether copying each time is faster than tracking
    // _current_begin
    size_type _current_begin; // start-position of not processed data in buffer
    size_type _current_count; // current amount of not processed data
    internal_buffer_type _read_buffer;
};
}

#endif
