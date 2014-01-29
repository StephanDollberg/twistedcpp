#ifndef TWISTEDCPP_DEFAULT_FACTORY_HPP
#define TWISTEDCPP_DEFAULT_FACTORY_HPP

#include <boost/asio/ip/tcp.hpp>

namespace twisted {

template <typename Protocol>
class default_factory {
public:
    Protocol operator()() const { return Protocol(); }

private:
};

} // namespace twisted

#endif
