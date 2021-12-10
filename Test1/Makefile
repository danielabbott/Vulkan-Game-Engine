CXX=clang++
# AR=ar

OPT_DBG_ARGS = -O0 -DDEBUG -ggdb
# OPT_DBG_ARGS = -O2 -DNDEBUG -g0

CFLAGS=-c -I../Pigeon -std=c++17 -Wall -Wextra -Wshadow -Wold-style-cast -pipe -isystem ../Pigeon/deps -fstack-protector $(OPT_DBG_ARGS)


SOURCES=$(wildcard *.cpp)
DEPS=$(SOURCES:%.cpp=build/%.d)
OBJECTS=$(SOURCES:%.cpp=build/%.o)
EXECUTABLE=build/test1

OBJECTS += ../Pigeon/build/*.o ../Pigeon/build/Vulkan/*.o ../Pigeon/build/WGI/*.o

#-L../Pigeon/build -lpigeon
LDFLAGS=-lglfw -lspdlog -lzstd -lvulkan -lX11 -lXi -lpthread -lm -lstdc++ -fuse-ld=lld $(OPT_DBG_ARGS)

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) #../Pigeon/build/libpigeon.a
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -MMD $(CFLAGS) $< -o $@

-include $(DEPS)

clean:
	-rm -r build/*

