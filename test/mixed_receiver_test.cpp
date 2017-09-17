#include "catch/single_include/catch.hpp"

#include "test_utils.hpp"

#include "../include/twisted/mixed_receiver.hpp"
#include "../include/twisted/reactor.hpp"

#include <boost/range/algorithm.hpp>
#include <boost/asio/read.hpp>
#include <boost/scope_exit.hpp>

#include <vector>
#include <string>
#include <future>

using boost::asio::ip::tcp;

struct mixed_receiver_test : twisted::mixed_receiver<mixed_receiver_test> {
    mixed_receiver_test() : mixed_receiver() {
        set_package_size(5);
    }

    void line_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_line(begin, end);
        set_byte_mode();
    }

    void bytes_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);
        set_line_mode();
    }
};

struct double_mixed_receiver_test : twisted::mixed_receiver<double_mixed_receiver_test> {
    double_mixed_receiver_test() : mixed_receiver(), switch_mode(0) {
        set_package_size(5);
    }

    void line_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);

        if(switch_mode == 1) {
            set_byte_mode();
            switch_mode = 0;
        }
        else {
            switch_mode += 1;
        }
    }

    void bytes_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);

        if(switch_mode == 1) {
            set_line_mode();
            switch_mode = 0;
        }
        else {
            switch_mode += 1;
        }
    }

    int switch_mode;
};

TEST_CASE("mixed_receiver behavior tests",
          "[mixed_receiver][protocols][behavior]") {
    SECTION("perfect march 4 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\nBBBBBCCC\r\nDDDDD");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");
        test_results.push_back("BBBBB");
        test_results.push_back("CCC\r\n");
        test_results.push_back("DDDDD");

        test::multi_send_and_recv<mixed_receiver_test>(test_data, test_results);
    }

    SECTION("perfect match 4 in 4") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\n");
        test_data.push_back("BBBBB");
        test_data.push_back("CCC\r\n");
        test_data.push_back("DDDDD");

        std::vector<std::string> test_results;
        test_results.push_back("AAA\r\n");
        test_results.push_back("BBBBB");
        test_results.push_back("CCC\r\n");
        test_results.push_back("DDDDD");

        test::multi_send_and_recv<mixed_receiver_test>(test_data, test_results);
    }

    SECTION("double perfect march 8 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\n111\r\nBBBBB22222CCC\r\n333\r\nDDDDD44444");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("111");
        test_results.push_back("BBBBB");
        test_results.push_back("22222");
        test_results.push_back("CCC");
        test_results.push_back("333");
        test_results.push_back("DDDDD");
        test_results.push_back("44444");

        test::multi_send_and_recv<double_mixed_receiver_test>(test_data, test_results);
    }

    SECTION("double perfect match 8 in 4") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA\r\n111\r\n");
        test_data.push_back("BBBBB22222");
        test_data.push_back("CCC\r\n333\r\n");
        test_data.push_back("DDDDD44444");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("111");
        test_results.push_back("BBBBB");
        test_results.push_back("22222");
        test_results.push_back("CCC");
        test_results.push_back("333");
        test_results.push_back("DDDDD");
        test_results.push_back("44444");

        test::multi_send_and_recv<double_mixed_receiver_test>(test_data, test_results);
    }
}
