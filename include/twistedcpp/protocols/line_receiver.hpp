#ifndef TWISTEDCPP_LINE_RECEIVER
#define TWISTEDCPP_LINE_RECEIVER

#include "basic_protocol.hpp"

#include <boost/tokenizer.hpp>

#include <deque>

namespace twisted {

template<typename ChildProtocol>
class line_receiver : public twisted::basic_protocol<ChildProtocol> {
public:
    line_receiver(std::string delimiter = std::string("\r\n"))
        : _delimiter(std::move(delimiter)) {

    }

    template<typename Iter>
    void on_message(Iter begin, Iter end) {
        _line_buffer.insert(_line_buffer.end(), begin, end);

        auto line_start = _line_buffer.begin();
        auto search_iter = find_next(_line_buffer.begin(), _line_buffer.end());

        while(search_iter != _line_buffer.end()) {
            this->this_protocol().line_received(line_start, search_iter);
            line_start = std::next(search_iter, _delimiter.size());
            search_iter = find_next(line_start, _line_buffer.end());
        }

        _line_buffer.assign(line_start, _line_buffer.end());
    }

    template<typename Iter>
    void send_line(Iter begin, Iter end) {
        std::array<boost::asio::const_buffer, 2> buffers{ {
            boost::asio::buffer(&*begin, std::distance(begin, end)),
            boost::asio::buffer(&*_delimiter.begin(), _delimiter.size())
        } };

        this->send_buffers(buffers);
    }

private:
    template<typename Iter>
    Iter find_next(Iter begin, Iter end) const {
        return std::search(
            begin, end, _delimiter.begin(), _delimiter.end());
    }

    const std::string _delimiter;
    std::vector<char> _line_buffer;
};

}

#endif
