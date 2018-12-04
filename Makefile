src = $(wildcard *.cpp Altera/*.cpp Xilinx/*.cpp)
obj = $(src:.cpp=.o)

CXXFLAGS = -std=c++11 -g -O3
all: polymer

polymer: $(obj)
	$(CXX) -o $@ $^ $(LDFLAGS)


.PHONY: clean
clean:
	rm -f $(obj) polymer
