# Compiler to use
CXX = g++

# Compiler flags: -Wall turns on most warnings, -g adds debug info
# We use pkg-config to get the correct include paths for OpenCV
CXXFLAGS = -Wall -g $(shell pkg-config --cflags opencv4)

# Linker flags: We use pkg-config to get the correct library paths and libs
LDFLAGS = $(shell pkg-config --libs opencv4)

# Name of your final executable
TARGET = sift_app

# The source file
SRC = sift_test.cpp

# The build target
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Clean rule to remove the executable
clean:
	rm -f $(TARGET)




libcamera-hello --list-cameras
