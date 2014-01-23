#include "catch/single_include/catch.hpp"

#include "../include/twistedcpp/protocols/line_receiver.hpp"

#include <vector>
#include <string>

struct test_protocol : twisted::line_receiver<test_protocol> {
    test_protocol(std::vector<std::string> results, std::vector<char> test_data)
        : _results(std::move(results)), current_index(0) {
            _read_buffer = test_data;
            _read_index = _read_buffer.begin();
        }

    template<typename Iter>
    void line_received(Iter begin, Iter end) {
        std::copy(begin, end, std::ostream_iterator<char>(std::cout, " | "));
        std::cout << std::endl;
        std::copy(_results[current_index].begin(), _results[current_index].end(), std::ostream_iterator<char>(std::cout, " | "));
        std::cout << std::endl;
        CHECK(std::equal(begin, end,
            _results[current_index++].begin()));
        std::copy(begin, end, std::ostream_iterator<char>(std::cout, " | "));
        std::cout << std::endl;
    }

    std::vector<std::string> _results;
    int current_index;

};

TEST_CASE("line_receiver::on_message", "[line_receiver][protocols]") {
    SECTION("perfect match") {
        std::vector<std::string> test_results;
        test_results.push_back("AAA");
        test_results.push_back("BBB");


        std::string test_data_str("AAA\r\nBBB\r\n");
        std::vector<char> test_data(test_data_str.begin(), test_data_str.end());

        test_protocol tester(std::move(test_results), test_data);

        tester.on_message(test_data.begin(), test_data.end());
    }
}
