# Project settings
TEST_TARGET = run_tests
MAKEFILE_DIR = $(dir $(realpath $(firstword $(MAKEFILE_LIST))))

PROJECT_DIR = $(MAKEFILE_DIR)
SRC_DIR = $(PROJECT_DIR)src
INC_DIR = $(PROJECT_DIR)inc
TEST_DIR = $(PROJECT_DIR)test
MOCK_DIR = $(TEST_DIR)/mocks
HOST_UTILS_DIR = $(PROJECT_DIR)utils/host
BUILD_DIR = $(PROJECT_DIR)build

CPPUTEST_HOME = /usr

# Compiler and flags
CC = gcc
CXX = g++
CFLAGS = -Wall -g -pedantic -Werror -I$(INC_DIR) -I$(CPPUTEST_HOME)/include -I$(MOCK_DIR) -DUNIT_TEST -I$(HOST_UTILS_DIR)
CXXFLAGS = -Wall -g -pedantic -Werror -I$(INC_DIR) -I$(CPPUTEST_HOME)/include -I$(MOCK_DIR) -DUNIT_TEST -I$(HOST_UTILS_DIR)
LDFLAGS = -L$(CPPUTEST_HOME)/lib -lCppUTest -lCppUTestExt

# Source files
C_SRCS = $(wildcard $(SRC_DIR)/*.c)
CPP_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter %.c, $(C_SRCS)))
CPP_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter %.cpp, $(CPP_SRCS)))

# Test source files
C_TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
CPP_TEST_SRCS = $(wildcard $(TEST_DIR)/*.cpp)
C_TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter %.c, $(C_TEST_SRCS)))
CPP_TEST_OBJS = $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter %.cpp, $(CPP_TEST_SRCS)))

# Mock source files
MOCK_SRCS = $(wildcard $(MOCK_DIR)/*.c) $(wildcard $(MOCK_DIR)/*.cpp)
MOCK_OBJS = $(patsubst $(MOCK_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter %.c, $(MOCK_SRCS))) \
            $(patsubst $(MOCK_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(filter %.cpp, $(MOCK_SRCS)))

# Create build directory
$(shell mkdir -p $(BUILD_DIR))

# Target
all: $(BUILD_DIR)/$(TEST_TARGET)

$(BUILD_DIR)/$(TEST_TARGET): $(C_TEST_OBJS) $(CPP_TEST_OBJS) $(C_OBJS) $(CPP_OBJS) $(MOCK_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(MOCK_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(MOCK_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
