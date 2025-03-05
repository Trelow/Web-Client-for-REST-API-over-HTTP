CXX = g++
CXXFLAGS = -Wall -g -Werror -Wno-error=unused-variable
INCLUDES = -I.

# Define the source files and objects
SOURCES = client.cpp helpers.cpp requests.cpp buffer.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = client

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

# Generic rule for building objects
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean

clean:
	rm -rf $(EXECUTABLE) *.o
