# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -O2 -pthread

# Libraries to link
LDFLAGS = -lportaudio -lopus -lpulse-simple -lpulse

# Source and output files
#hello there
SRCS = streamplayer.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = client

# Build the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Run the client
run: $(TARGET)
	./$(TARGET)
