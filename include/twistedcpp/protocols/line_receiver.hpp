#ifndef TWISTEDCPP_LINE_RECEIVER
#define TWISTEDCPP_LINE_RECEIVER

#include "basic_protocol.hpp"

#include <boost/tokenizer.hpp>

#include <deque>

namespace twisted {

template<typename ChildProtocol>
class line_receiver : public twisted::basic_protocol<ChildProtocol> {
public:
    line_receiver() : delimiter("\r\n") {

    }

    template<typename Iter>
    void on_message(Iter begin, Iter end) {
        _line_buffer.insert(_line_buffer.end(), begin, end);

        auto line_start = _line_buffer.begin();
        auto search_iter = std::search(
            _line_buffer.begin(), _line_buffer.end(), delimiter.begin(), delimiter.end());


        while(search_iter != _line_buffer.end()) {
            this->this_protocol().line_received(line_start, search_iter);
            line_start = std::next(search_iter, delimiter.size());
            search_iter = std::search(
                line_start, _line_buffer.end(), delimiter.begin(), delimiter.end());
        }

        _line_buffer.assign(line_start, _line_buffer.end());
    }

    template<typename Iter>
    void send_line(Iter begin, Iter end) {
        std::array<boost::asio::const_buffer, 2> buffers{ {
            boost::asio::buffer(&*begin, std::distance(begin, end)),
            boost::asio::buffer(&*delimiter.begin(), delimiter.size())
        } };

        this->send_buffers(buffers);
    }

private:
    const std::string delimiter;
    std::vector<char> _line_buffer;
};

}

#endif
