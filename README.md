twistedcpp
==========
Porting twisted to C++.

Note: Everything is very very alpha, yet. Help and projects very appreciated.


``` cpp
#include <twistedcpp/reactor.hpp>
#include <twistedcpp/protocols/basic_protocol.hpp>

struct echo_protocol : twisted::basic_protocol<echo_protocol> {
    template<typename Iter>
    void on_message(Iter begin, Iter end) {
        send_message(begin, end);
    }
};

int main() {
    twisted::run<echo_protocol>(12345);
}
```
