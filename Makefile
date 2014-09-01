RE2_DIR=/home/j/lab/re2/
GTEST_DIR=/home/j/lab/gtest-1.7.0/
MOCKCPP_HEADER_DIR=/home/j/lab/mockcpp/mockcpp/include
MOCKCPP_LIB_DIR=/usr/include/mockcpp/lib

CPPFLAGS= -g -I/usr/include -I$(GTEST_DIR)include -I$(MOCKCPP_HEADER_DIR) -I$(RE2_DIR)
LDFLAGS=
LDLIBS=-L$(RE2_DIR)obj -L$(GTEST_DIR)lib -L$(MOCKCPP_LIB_DIR) -lgtest -lpthread -lmockcpp -lre2

TEST_SRCS=$(shell find test -iname *.cpp)
TEST_OBJS=$(subst .cpp,.o,$(TEST_SRCS))

all: $(TEST_OBJS)
	g++ -I/usr/include -I$(RE2_DIR) -o msgflow src/msgflow.cpp -L$(RE2_DIR)obj -lre2 -lpthread
	g++ $(CPPFLAGS) $(LDFLAGS) -g -o main $^ $(LDLIBS) && ./main --gtest_filter=*xxx*

clean:
	rm -rdf $(OBJS) main main.dSYM test/*.o msgflow
