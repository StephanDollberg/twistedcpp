DEFINES= -D_GLIBCXX_USE_SCHED_YIELD -D_GLIBCXX_USE_NANOSLEEP
ARGS= -std=c++11 -O2 -Wall -Werror -Wextra -pedantic-errors  $(INCLUDES) $(DEFINES)

SOURCES = $(wildcard *.cpp)
OBJS = $(addprefix ./, $(notdir $(SOURCES:.cpp=.o)))

%.o: %.cpp
	$(CXX) $(ARGS) $(CXXFLAGS) -c $< -o $@

test_main: $(OBJS)
	$(CXX) $(ARGS) $(LDFLAGS) $(OBJS) -o $@

run: test_main
	./test_main

clean:
	rm *.o test_main
