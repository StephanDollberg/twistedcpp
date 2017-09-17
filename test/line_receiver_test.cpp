#include "catch/single_include/catch.hpp"

#include "test_utils.hpp"

#include "../include/twisted/line_receiver.hpp"
#include "../include/twisted/reactor.hpp"

#include <boost/range/algorithm.hpp>
#include <boost/asio/read.hpp>
#include <boost/scope_exit.hpp>

#include <vector>
#include <string>
#include <future>

using boost::asio::ip::tcp;

struct line_receiver_test : twisted::line_receiver<line_receiver_test> {
    void line_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_line(begin, end);
    }
};

struct line_receiver_test_alt_delimiter
    : twisted::line_receiver<line_receiver_test_alt_delimiter> {

    line_receiver_test_alt_delimiter() : line_receiver("XXX") {}

    void line_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_line(begin, end);
    }
};

TEST_CASE("line_receiver behavior tests",
          "[line_receiver][protocols][behavior]") {
    SECTION("perfect match 2 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\nBBB\r\n");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");
        test_results.push_back("BBB\r\n");

        test::multi_send_and_recv<line_receiver_test>(test_data, test_results);
    }

    SECTION("perfect match 3 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("\r");
        test_data.push_back("\n");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");

        test::multi_send_and_recv<line_receiver_test>(test_data, test_results);
    }

    SECTION("perfect match 2 on 2") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\n");
        test_data.push_back("BBB\r\n");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");
        test_results.push_back("BBB\r\n");

        test::multi_send_and_recv<line_receiver_test>(test_data, test_results);
    }

    SECTION("open match 2 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\nBBB\r\nCCC");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");
        test_results.push_back("BBB\r\n");

        test::multi_send_and_recv<line_receiver_test>(test_data, test_results);
    }

    SECTION("open match 2 on 2") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\n");
        test_data.push_back("BBB\r\n");
        test_data.push_back("CCC");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");
        test_results.push_back("BBB\r\n");

        test::multi_send_and_recv<line_receiver_test>(test_data, test_results);
    }

    SECTION("end match 3 on 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");
        test_data.push_back("CCC\r\n");

        std::vector<std::string> test_results;
        test_results.push_back("AAABBBCCC\r\n");

        test::multi_send_and_recv<line_receiver_test>(test_data, test_results);
    }

    SECTION("no match 2 on 0") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");

        std::vector<std::string> test_results;
        test_results.push_back("");

        test::multi_send_and_recv<line_receiver_test>(test_data, test_results);
    }

    SECTION("alternative delimiter") {
        std::vector<std::string> test_data;
        test_data.push_back("AAAXXX");

        std::vector<std::string> test_results;
        test_results.push_back("AAAXXX");

        test::multi_send_and_recv<line_receiver_test_alt_delimiter>(test_data,
                                                         test_results);
    }

    SECTION("buffer expansion") {
        std::vector<std::string> test_data;
        // needs to be bigger than the internal buffer
        std::string test_string(48, 'X');
        test_string.append("\r\n");
        test_data.push_back(test_string);

        std::vector<std::string> test_results;
        test_results.push_back(test_string);

        test::multi_send_and_recv<line_receiver_test>(test_data, test_results);
    }
}
