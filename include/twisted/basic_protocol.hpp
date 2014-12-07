#ifndef TWISTEDCPP_BASIC_PROTOCOL
#define TWISTEDCPP_BASIC_PROTOCOL

#include "protocol_core.hpp"

namespace twisted {

/**
 * @brief Basic protocol around the protocol interface
 *
 * Provided callbacks:
 *
 * void on_message(const_buffer_iterator begin, const_buffer_iterator end);
 *
 * Will be called whenever data is received. [begin, end) is the data range.
 */
template <typename ChildProtocol>
class basic_protocol : public protocol_core<ChildProtocol, std::vector<char>> {
public:
    typedef std::vector<char> buffer_type;

    basic_protocol() : _read_buffer(initial_buffer_size(), '\0') {}

    buffer_type& read_buffer() { return _read_buffer; }

    const buffer_type& read_buffer() const { return _read_buffer; }

private:
    std::size_t initial_buffer_size() const { return 1024; }

    std::vector<char> _read_buffer;
};

} // namespace twisted

#endif
