twistedcpp
==========
Porting twisted to C++ using `boost::asio` coroutines.

``` cpp
#include <twisted/reactor.hpp>
#include <twisted/basic_protocol.hpp>
#include <twisted/default_factory.hpp>

struct echo_protocol : twisted::basic_protocol<echo_protocol> {
    void on_message(const_buffer_iterator begin, const_buffer_iterator end) { 
        send_message(begin, end);
    }
};

int main() {
    twisted::reactor reac;
    reac.listen_tcp(50000, twisted::default_factory<echo_protocol>());
    reac.run();
}
```
### Feature Support
 - [Line Receiver (Delimited String)] (https://github.com/StephanDollberg/twistedcpp/wiki/Tutorials#line-receiver)
 - [Byte Receiver (N Bytes)] (https://github.com/StephanDollberg/twistedcpp/wiki/Tutorials#byte-receiver)
 - [Mixed Receiver (Line + Byte)] (https://github.com/StephanDollberg/twistedcpp/wiki/Tutorials#mixed-receiver)
 - [Factories] (https://github.com/StephanDollberg/twistedcpp/wiki/Tutorials#factories)
 - [TCP + SSL Transports] (https://github.com/StephanDollberg/twistedcpp/wiki/Tutorials#transport-types)
 - [Deferreds] (https://github.com/StephanDollberg/twistedcpp/wiki/Tutorials#using-deferreds---aka-async-callbacks)
 - [Buffer Interface] (https://github.com/StephanDollberg/twistedcpp/wiki/Tutorials#the-buffer-interface---aka-using-the-protocol_core)

Check the [Tutorials] (https://github.com/StephanDollberg/twistedcpp/wiki/Tutorials) and [Reference](https://stephandollberg.github.io/twistedcpp/annotated.html) for a detailed explanation of everything.

### Using/Installing twistedcpp

twistedcpp is header only so you don't need to build anything. You can either add include/twisted to your compiler include path(e.g.: -I/path/to/twisted/include or put it to /usr/local/include) or copy the files to your local project. However, twistedcpp depends on boost asio and if the ssl part is used also on openssl. Meaning that you have to link against -lboost_coroutine, -lboost_context, -lboost_system, -lssl and -lcrypto

More work, help and projects very appreciated. 
