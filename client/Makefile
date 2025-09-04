# MCP Client Makefile
# Compiles all C++ files in the current directory with C++17 standard

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I../third_party/spdlog/include

# Source files
SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

# Target executable
TARGET = main

# Default target
all: $(TARGET)

# Link object files to create executable and clean up object files
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lcurl
	rm -f $(OBJS)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
