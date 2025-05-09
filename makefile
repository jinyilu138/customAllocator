# Compiler and flags
CXX = clang++
CXXFLAGS = -Wall -Wextra -std=c++17 -g -fsanitize=address

# Directories
SRC_DIR = .
FREELIST_DIR = freelist
BUDDY_DIR = buddy

# Source files
SRCS = $(SRC_DIR)/main.cpp \
       $(FREELIST_DIR)/freelist.cpp \
       $(BUDDY_DIR)/buddy.cpp

# Output binary
TARGET = allocator

# Default rule
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean
