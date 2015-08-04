TARGET=dsd-player
CXXFLAGS=-std=c++11 -Wall -O2 -lasound

all: $(TARGET)
clean:
	rm -f $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<
