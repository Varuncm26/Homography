# Name of your executable
TARGET = feature_matcher

# Your source file
SRC = main.cpp

# Compiler
CXX = g++

# Compiler flags (Optimization for Raspberry Pi 5)
CXXFLAGS = -O3 -Wall `pkg-config --cflags opencv4`

# Linker flags
LDFLAGS = `pkg-config --libs opencv4`

# Default target
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

# Clean up
clean:
	rm -f $(TARGET)