CXX=clang++
# AR=ar

OPT_DBG_ARGS = -O0 -DDEBUG -ggdb
# OPT_DBG_ARGS = -O2 -DNDEBUG -g0

CFLAGS=-c -std=c++17 -Wall -Wextra -Wshadow -Wold-style-cast -Wno-missing-field-initializers -pipe -isystem deps -fstack-protector $(OPT_DBG_ARGS)



SOURCES=$(wildcard *.cpp) $(wildcard Vulkan/*.cpp) $(wildcard WGI/*.cpp)
DEPS=$(SOURCES:%.cpp=build/%.d)
OBJECTS=$(SOURCES:%.cpp=build/%.o)
# ARCHIVE=build/libpigeon.a


all: $(OBJECTS)
# all: $(ARCHIVE)

# $(ARCHIVE): $(OBJECTS)
# 	$(AR) rcs $(ARCHIVE) $(OBJECTS)

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -MMD $(CFLAGS) $< -o $@

-include $(DEPS)

clean:
	-rm -r build/*

