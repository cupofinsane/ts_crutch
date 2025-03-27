CXX = g++
CXXFLAGS = -lX11
SRCS = main.cc
TARGET = ts_crutch

all: $(TARGET)

$(TARGET): $(SRCS)
	g++ main.cc -lX11 -o ts_crutch

clean:
	rm -f $(TARGET)

.PHONY: all clean
