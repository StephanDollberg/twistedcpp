#ifndef TWISTEDCPP_BYTE_RECEIVER_PARSER
#define TWISTEDCPP_BYTE_RECEIVER_PARSER

#include "../byte_receiver.hpp"

#include <vector>
#include <cstddef>

#include <boost/range/iterator_range.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/container_fwd.hpp>

namespace twisted {

namespace byte {

template <typename Protocol>
void core_loop(std::size_t& _current_count, std::size_t& _current_begin,
               boost::container::vector<char>& _read_buffer,
               std::size_t _next_bytes_size, Protocol& protocol) {
    /* protocol user can change package size in between calls
        -> we need to cache the old package-size for the _current_* updates
       */
    std::size_t old_package_size = _next_bytes_size;

    protocol.bytes_received(
        std::next(_read_buffer.begin(), _current_begin),
        std::next(_read_buffer.begin(), _current_begin + _next_bytes_size));

    _current_count -= old_package_size;
    _current_begin += old_package_size;
}

inline bool loop_condition(std::size_t _current_count,
                           std::size_t _next_bytes_size) {
    return _current_count >= _next_bytes_size;
}

inline void prepare_buffers(std::size_t _current_count,
                            std::size_t& _current_begin,
                            boost::container::vector<char>& _read_buffer) {
    // un-fragmented data
    if (_current_count == 0) {
        _current_begin = 0;
    }
    /* fragmented data and buffer is full
        -> we need to copy the existing data to the beginning
        to make space for a full packet size */
    else if (_current_begin + _current_count == _read_buffer.size()) {
        std::copy(std::next(_read_buffer.begin(), _current_begin),
                  _read_buffer.end(), _read_buffer.begin());

        _current_begin = 0;
    }
}

template <typename Protocol, typename Iter>
void parse(Iter begin, Iter end, std::size_t& _current_begin,
           std::size_t& _current_count, std::size_t& _next_bytes_size,
           boost::container::vector<char>& _read_buffer, Protocol& protocol) {

    _current_count += std::distance(begin, end);

    while (loop_condition(_current_count, _next_bytes_size)) {
        core_loop(_current_count, _current_begin, _read_buffer, _next_bytes_size, protocol);
    }

    prepare_buffers(_current_count, _current_begin, _read_buffer);
}

constexpr std::size_t calculate_buffer_size(std::size_t new_min_size) {
    return 3 * new_min_size;
}

inline void set_package_size(std::size_t package_size, std::size_t& _next_bytes_size,
                      boost::container::vector<char>& _read_buffer) {
    if (_next_bytes_size < package_size) {
        _read_buffer.resize(calculate_buffer_size(package_size));
    }

    _next_bytes_size = package_size;

    // evaluate different kinds of copying existing data + buffer size here
}

}
}

#endif
