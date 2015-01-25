#ifndef TWISTEDCPP_TEST_UTILS_HPP
#define TWISTEDCPP_TEST_UTILS_HPP

#include "../include/twisted/reactor.hpp"
#include "../include/twisted/basic_protocol.hpp"
#include "../include/twisted/default_factory.hpp"

#include <boost/asio/read.hpp>
#include <boost/scope_exit.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/range/algorithm.hpp>

#include <future>
#include <vector>
#include <string>
#include <memory>

using boost::asio::ip::tcp;

namespace test {

template<typename ProtocolType>
struct launcher {
    twisted::reactor& _reac;

    launcher(twisted::reactor& reac) : _reac(reac) {}

    template<typename... Args>
    void operator()(Args&&... args) {
        _reac.listen_tcp<ProtocolType>(50000, std::forward<Args>(args)...);
        _reac.run();
    }
};

template<typename ProtocolType, typename... Args>
void single_send_and_recv(std::string send, std::string recv, Args&&... args) {
    twisted::reactor reac;
    launcher<ProtocolType> launch(reac);
    auto fut = std::async(std::launch::async,
                          launch, std::ref(std::forward<Args>(args))...);

    boost::asio::io_service io_service;
    tcp::socket socket(io_service);

    BOOST_SCOPE_EXIT_TPL((&reac)(&socket)) {
        reac.stop();
        if (socket.is_open()) {
            socket.close();
        }
    }
    BOOST_SCOPE_EXIT_END

    std::this_thread::sleep_for(std::chrono::milliseconds(3));

    socket.connect(tcp::endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), 50000));

    boost::asio::write(socket, boost::asio::buffer(send));

    std::string buffer(recv.size(), '\0');
    boost::asio::read(socket, boost::asio::buffer(&buffer[0], buffer.size()));
    CHECK(buffer == recv);
}

template <typename ProtocolType>
void multi_send_and_recv(const std::vector<std::string>& send_input,
                    const std::vector<std::string>& results) {
    twisted::reactor reac;
    auto fut = std::async(std::launch::async, [&]() {
        reac.listen_tcp<ProtocolType>(50000);
        reac.run();
    });

    boost::asio::io_service io_service;
    tcp::socket socket(io_service);

    BOOST_SCOPE_EXIT_TPL((&reac)(&socket)) {
        reac.stop();
        if (socket.is_open()) {
            socket.close();
        }
    }
    BOOST_SCOPE_EXIT_END

    std::this_thread::sleep_for(std::chrono::milliseconds(3));

    socket.connect(tcp::endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), 50000));

    boost::for_each(send_input, [&](const std::string& input) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        boost::asio::write(socket, boost::asio::buffer(input));
    });

    boost::for_each(results, [&](const std::string& input) {
        std::string buffer(input.size(), '\0');
        boost::asio::read(socket,
                          boost::asio::buffer(&buffer[0], buffer.size()));
        CHECK(buffer == input);
    });
}

} // namespace test


#endif // TWISTEDCPP_TEST_UTILS_HPP
