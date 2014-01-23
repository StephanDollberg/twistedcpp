#ifndef TWISTEDCPP_LINE_RECEIVER
#define TWISTEDCPP_LINE_RECEIVER

#include "basic_protocol.hpp"

namespace twisted {

template<typename ChildProtocol>
class line_receiver : public twisted::basic_protocol<ChildProtocol> {
public:
    line_receiver() : delimiter("\r\n") {

    }

    template<typename Iter>
    void on_message(Iter begin, Iter end) {
        auto search_iter =
            std::search(begin, end, delimiter.begin(), delimiter.end());
        auto next_iter = end;
        auto line_start = this->_read_buffer.begin();

        while(search_iter != end) {
            this->this_protocol().line_received(line_start, search_iter);
            next_iter = std::next(search_iter, delimiter.size());
            search_iter =
                std::search(next_iter, end, delimiter.begin(), delimiter.end());
                line_start = next_iter;
        }

        this->_read_index = std::copy(next_iter, end, this->_read_buffer.begin());
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
};

}

#endif
