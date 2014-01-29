DEFINES= -D_GLIBCXX_USE_SCHED_YIELD -D_GLIBCXX_USE_NANOSLEEP
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	ifeq ($(CXX),clang++)
		LIBS= -pthread -lc++abi -lboost_coroutine -lboost_context -lboost_system
	else
		LIBS= -lboost_coroutine -lboost_context -lboost_system
	endif
endif
ifeq ($(UNAME), Darwin)
	LIBS= -lboost_coroutine-mt -lboost_context-mt -lboost_system-mt
endif
ARGS= -std=c++11 -g  -Wall -Werror -Wextra -pedantic-errors  $(INCLUDES) $(DEFINES)

ifeq ($(CXX),clang++-3.5)
	ADD_INC_FLAGS = -nostdinc++ -I/usr/local/lib/llvm-3.5/include/c++/v1 -fsanitize=memory -fno-omit-frame-pointer
	ADD_LIB_FLAGS = -L/usr/local/lib/llvm-3.5/usr/lib
else
 	ADD_INC_FLAGS =
 	ADD_LIB_FLAGS =
endif

ifeq ($(CXX),clang++)
	standard_library = -stdlib=libc++
else
	standard_library =
endif

SOURCES = $(wildcard *.cpp)
OBJS = $(addprefix ./, $(notdir $(SOURCES:.cpp=.o)))

%.o: %.cpp
	$(CXX) $(standard_library) $(ARGS) $(CXXFLAGS) $(ADD_INC_FLAGS) -c $< -o $@

test_main: $(OBJS)
	$(CXX) $(standard_library) $(ARGS) $(LDFLAGS) $(OBJS) -o $@ $(LIBS) $(ADD_LIB_FLAGS)

run:
	./test_main

clean:
	rm *.o test_main
