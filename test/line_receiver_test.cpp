#include "catch/single_include/catch.hpp"

#include "../include/twistedcpp/protocols/line_receiver.hpp"
#include "../include/twistedcpp/reactor.hpp"

#include <boost/asio/read.hpp>

#include <vector>
#include <string>
#include <future>

using boost::asio::ip::tcp;

struct line_receiver_test : twisted::line_receiver<line_receiver_test>  {
    template<typename Iter>
    void line_received(Iter begin, Iter end) {
        send_line(begin, end);
    }
};

TEST_CASE("line_receiver behaviour tests", "[line_receiver][protocols]") {
    SECTION("perfect match") {
        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");

        twisted::reactor reac;
        auto fut = std::async([&]() {
            twisted::run<line_receiver_test>(reac, 12345);
            reac.run();
        });

        boost::asio::io_service io_service;
        std::string test_data("AAA\r\nBBB\r\n");

        tcp::socket socket(io_service);
        socket.connect(tcp::endpoint(
            boost::asio::ip::address::from_string("127.0.0.1"), 12345));

        boost::asio::write(socket, boost::asio::buffer(test_data));
        std::vector<char> result(test_data.size());

        boost::asio::read(socket, boost::asio::buffer(result));
        CHECK(std::equal(test_data.begin(), test_data.end(), result.begin()));
        reac.stop();
        fut.get();
    }
}
