CXX = gcc
CXX_FLAGS = 

SRC  = $(wildcard *.cpp)
OBJ  = $(patsubst %.cpp,%.o,$(SRC))
DEP  = $(patsubst %.cpp,%.d,$(SRC))
BIN  = $(wildcard *.out)

all: client.out server.out

client.out: client.o utils.o
	$(CXX) $(CXX_FLAGS) $^ -o $@

server.out: server.o utils.o
	$(CXX) $(CXX_FLAGS) $^ -o $@

-include $(DEP)

%.o: %.c
	$(CXX) $(CXX_FLAGS) -MMD -c $< -o $@

clean:
	rm -rf $(DEP) $(OBJ) $(BIN)
