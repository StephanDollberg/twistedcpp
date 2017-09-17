#include "catch/single_include/catch.hpp"

#include "test_utils.hpp"

#include "../include/twisted/byte_receiver.hpp"
#include "../include/twisted/reactor.hpp"

#include <boost/range/algorithm.hpp>
#include <boost/asio/read.hpp>
#include <boost/scope_exit.hpp>

#include <vector>
#include <string>
#include <future>

using boost::asio::ip::tcp;

struct byte_receiver_test : twisted::byte_receiver<byte_receiver_test> {
    byte_receiver_test() : byte_receiver(3) {}

    void bytes_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);
    }
};

struct byte_receiver_update_test : twisted::byte_receiver<byte_receiver_update_test> {
    byte_receiver_update_test() : byte_receiver(2) {}

    void bytes_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);

        set_package_size(20);
    }
};

struct byte_receiver_decrease_update_test : twisted::byte_receiver<byte_receiver_decrease_update_test> {
    byte_receiver_decrease_update_test() : byte_receiver(4) {}

    void bytes_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);

        set_package_size(2);
    }
};

struct next_packet_test : twisted::byte_receiver<next_packet_test> {
    next_packet_test() : byte_receiver(4) {}

    void bytes_received(const_buffer_iterator begin, const_buffer_iterator end) {
        send_message(begin, end);

        set_package_size(2);
        auto ret = next_packet();
        send_message(ret.begin(), ret.end());
    }
};

TEST_CASE("byte_receiver behavior tests",
          "[byte_receiver][protocols][behavior]") {
    SECTION("perfect match 2 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAABBB");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");

        test::multi_send_and_recv<byte_receiver_test>(test_data, test_results);
    }

    SECTION("perfect match 3 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("A");
        test_data.push_back("A");
        test_data.push_back("A");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");

        test::multi_send_and_recv<byte_receiver_test>(test_data, test_results);
    }

    SECTION("perfect match 2 on 2") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");

        test::multi_send_and_recv<byte_receiver_test>(test_data, test_results);
    }

    SECTION("open match 2 in 1") {
        std::vector<std::string> test_data;
        test_data.push_back("AAABBBCC");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");

        test::multi_send_and_recv<byte_receiver_test>(test_data, test_results);
    }

    SECTION("open match 2 on 2") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");
        test_data.push_back("CC");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");

        test::multi_send_and_recv<byte_receiver_test>(test_data, test_results);
    }

    SECTION("end match 3 on 1") {
        std::vector<std::string> test_data;
        test_data.push_back("A");
        test_data.push_back("B");
        test_data.push_back("C");

        std::vector<std::string> test_results;
        test_results.push_back("ABC");

        test::multi_send_and_recv<byte_receiver_test>(test_data, test_results);
    }

    SECTION("no match 2 on 0") {
        std::vector<std::string> test_data;
        test_data.push_back("A");
        test_data.push_back("B");

        std::vector<std::string> test_results;
        test_results.push_back("");

        test::multi_send_and_recv<byte_receiver_test>(test_data, test_results);
    }

    SECTION("even wrap around") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");
        test_data.push_back("CCC");
        test_data.push_back("DDD");
        test_data.push_back("EEE");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");
        test_results.push_back("CCC");
        test_results.push_back("DDD");
        test_results.push_back("EEE");

        test::multi_send_and_recv<byte_receiver_test>(test_data, test_results);
    }

    SECTION("uneven wrap around") {
        std::vector<std::string> test_data;
        test_data.push_back("AAA");
        test_data.push_back("BBB");
        test_data.push_back("C");
        test_data.push_back("CCD");
        test_data.push_back("DDE");
        test_data.push_back("EE");

        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");
        test_results.push_back("CCC");
        test_results.push_back("DDD");
        test_results.push_back("EEE");

        test::multi_send_and_recv<byte_receiver_test>(test_data, test_results);
    }

    SECTION("change package size - simple upgrade") {
        std::vector<std::string> test_data;
        test_data.push_back("AA");
        test_data.push_back(std::string(20, 'X'));

        std::vector<std::string> test_results;
        test_results.push_back("AA");
        test_results.push_back(std::string(20, 'X'));

        test::multi_send_and_recv<byte_receiver_update_test>(test_data, test_results);
    }

    SECTION("change package size - simple upgrade one shot") {
        std::vector<std::string> test_data;
        std::string test_string = std::string(20, 'X');
        test_string.insert(0, 2, 'A');
        test_data.push_back(test_string);

        std::vector<std::string> test_results;
        test_results.push_back("AA");
        test_results.push_back(std::string(20, 'X'));

        test::multi_send_and_recv<byte_receiver_update_test>(test_data, test_results);
    }

    SECTION("decrease package size - simple upgrade") {
        std::vector<std::string> test_data;
        test_data.push_back("AAAABBCCDD");

        std::vector<std::string> test_results;
        test_results.push_back("AAAA");
        test_results.push_back("BB");
        test_results.push_back("CC");
        test_results.push_back("DD");

        test::multi_send_and_recv<byte_receiver_decrease_update_test>(test_data, test_results);
    }


}

TEST_CASE("next_packet", "[byte_receiver][protocols][behavior][next_packet]") {
    std::vector<std::string> test_data;
    test_data.push_back("AAAABB");

    std::vector<std::string> test_results;
    test_results.push_back("AAAA");
    test_results.push_back("BB");

    test::multi_send_and_recv<next_packet_test>(test_data, test_results);
}
