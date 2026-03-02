# Compiler and Flags
CXX = g++
CXXFLAGS = -O3 -Wall `pkg-config --cflags opencv4`
LDFLAGS = `pkg-config --libs opencv4`

# Executable Names
CAPTURE_BIN = capture_tool
PROCESS_BIN = feature_matcher

# Source Files
CAPTURE_SRC = capture.cpp
PROCESS_SRC = Homography.cpp

# Default: Build both programs
all: $(CAPTURE_BIN) $(PROCESS_BIN)

# Build the Capture Tool
$(CAPTURE_BIN): $(CAPTURE_SRC)
	$(CXX) $(CXXFLAGS) $(CAPTURE_SRC) -o $(CAPTURE_BIN) $(LDFLAGS)

# Build the Processing Tool
$(PROCESS_BIN): $(PROCESS_SRC)
	$(CXX) $(CXXFLAGS) $(PROCESS_SRC) -o $(PROCESS_BIN) $(LDFLAGS)

# Clean up binaries and saved images
clean:
	rm -f $(CAPTURE_BIN) $(PROCESS_BIN) capture_1.jpg capture_2.jpg ransac_result.jpg featuremap_result.jpg

.PHONY: all clean

