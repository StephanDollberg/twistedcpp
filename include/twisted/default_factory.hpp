#ifndef TWISTEDCPP_DEFAULT_FACTORY_HPP
#define TWISTEDCPP_DEFAULT_FACTORY_HPP

namespace twisted {

template <typename Protocol>
struct default_factory {
    Protocol operator()() const { return Protocol(); }
};

} // namespace twisted

#endif
