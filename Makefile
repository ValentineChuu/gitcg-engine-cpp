# Build with: make [-j4] [MODE=debug]
# Library sources are every .cpp under core/, query/, cards/, bridge/, definitions/ and programs/support/; each program
# directly in programs/ links against them.

MODE ?= release
CXX ?= g++
CXXFLAGS_COMMON := -std=c++23 -Wall -Wextra -Wno-unused-parameter -I. -MMD -MP
ifeq ($(MODE),debug)
  CXXFLAGS := $(CXXFLAGS_COMMON) -O0 -g -fsanitize=address,undefined
  LDFLAGS := -fsanitize=address,undefined
else
  CXXFLAGS := $(CXXFLAGS_COMMON) -O2 -g
  LDFLAGS :=
endif

BUILD := build/$(MODE)
LIBRARY_SOURCES := $(shell find core query cards bridge definitions programs/support -name '*.cpp' 2>/dev/null)
PROGRAM_SOURCES := $(wildcard programs/*.cpp)
LIBRARY_OBJECTS := $(LIBRARY_SOURCES:%.cpp=$(BUILD)/%.o)
PROGRAMS := $(PROGRAM_SOURCES:programs/%.cpp=$(BUILD)/%)

all: $(PROGRAMS)

$(BUILD)/libgitcg.a: $(LIBRARY_OBJECTS)
	@mkdir -p $(dir $@)
	ar rcs $@ $^

$(BUILD)/%: programs/%.cpp $(BUILD)/libgitcg.a
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< $(BUILD)/libgitcg.a $(LDFLAGS) -o $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build

-include $(LIBRARY_OBJECTS:.o=.d)

.PHONY: all clean
