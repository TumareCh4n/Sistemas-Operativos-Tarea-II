CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread

TARGET = torneo

SRC = src/main.cpp \
      src/torneo.cpp \
      src/tablero.cpp

OBJ = $(SRC:.cpp=.o)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET) 2 1