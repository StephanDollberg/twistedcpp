#include "catch/single_include/catch.hpp"

#include "../include/twistedcpp/protocols/line_receiver.hpp"
#include "../include/twistedcpp/reactor.hpp"

#include <boost/range/algorithm.hpp>
#include <boost/asio/read.hpp>
#include <boost/scope_exit.hpp>

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

void messsage_tester(const std::vector<std::string>& send_input, const std::vector<std::string>& results) {
    twisted::reactor reac;
    auto fut = std::async([&]() {
        twisted::run<line_receiver_test>(reac, 12345);
        reac.run();
    });

    BOOST_SCOPE_EXIT((&reac)) {
        reac.stop();
    } BOOST_SCOPE_EXIT_END

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    boost::asio::io_service io_service;
    tcp::socket socket(io_service);
    socket.connect(tcp::endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), 12345));

    boost::for_each(send_input, [&] (const std::string& input) {
        boost::asio::write(socket, boost::asio::buffer(input));
    });

    boost::for_each(results, [&] (const std::string& input) {
        std::string buffer(input.size(), '\0');
        boost::asio::read(socket, boost::asio::buffer(&buffer[0], buffer.size()));
        CHECK(boost::equal(buffer, input));
    });
}

TEST_CASE("line_receiver behaviour tests", "[line_receiver][protocols]") {
    SECTION("perfect match") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\nBBB\r\n");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");
        test_results.push_back("BBB\r\n");

        messsage_tester(test_data, test_results);
    }
}
