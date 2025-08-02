CXX = g++
CXXFLAGS = -O2 -std=c++17

TARGET = reconstruction_ghwangbo
SRC = main.cpp orderbook.cpp 

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
