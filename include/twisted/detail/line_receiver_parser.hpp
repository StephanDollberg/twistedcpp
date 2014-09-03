#ifndef TWISTEDCPP_LINE_RECEIVER_PARSER
#define TWISTEDCPP_LINE_RECEIVER_PARSER

#include "../byte_receiver.hpp"

#include <vector>
#include <cstddef>

#include <boost/range/iterator_range.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/container_fwd.hpp>

namespace twisted {

namespace line {

template <typename Iter>
Iter find_next(Iter begin, Iter end, const std::string& _delimiter) {
    return std::search(begin, end, _delimiter.begin(), _delimiter.end());
}

template <typename Protocol, typename Iter>
void parse(Iter begin, Iter end, std::size_t& _current_begin,
           std::size_t& _current_count, const std::string& _delimiter,
           boost::container::vector<char>& _line_buffer, Protocol& protocol) {
    _current_count += std::distance(begin, end);

    auto line_start = std::next(_line_buffer.begin(), _current_begin);
    auto search_iter = find_next(line_start, end, _delimiter);

    while (search_iter != end) {
        protocol.line_received(line_start, search_iter);
        _current_count -=
            std::distance(line_start, search_iter) + _delimiter.size();
        _current_begin +=
            std::distance(line_start, search_iter) + _delimiter.size();

        line_start = std::next(search_iter, _delimiter.size());
        search_iter = find_next(line_start, end, _delimiter);
    }

    if (_current_count == _line_buffer.size()) {
        _line_buffer.resize(_line_buffer.size() * 2);
    } else if (_current_begin + _current_count == _line_buffer.size()) {
        std::copy(std::next(_line_buffer.begin(), _current_begin),
                  _line_buffer.end(), _line_buffer.begin());

        _current_begin = 0;
    }
}
}
}

#endif
