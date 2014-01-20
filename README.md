twistedcpp
==========
Porting twisted to C++.

Note: Everything is very very alpha, yet.


``` cpp
#include "reactor.hpp"
#include "protocols.hpp"

class echo_protocol: public twisted::basic_protocol<echo_protocol> {
public:
    template<typename Iter>
    void handle_read(Iter begin, Iter end) {
        write(begin, end);
    }
};

int main() {
    twisted::run<echo_protocol>(12345);
}
```
