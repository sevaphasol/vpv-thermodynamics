CC = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
EXEC = $(BIN_DIR)/brownian_motion

RESOURCES = res/DejaVuSans.ttf

all: dirs $(EXEC)

dirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR) $(dir $(RESOURCES))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

$(EXEC): $(OBJECTS)
	$(CC) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

copy_resources:
	cp DejaVuSans.ttf $(RESOURCES)

clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

mrproper: clean
	rm -rf $(dir $(RESOURCES))

.PHONY: all clean mrproper copy_resources
