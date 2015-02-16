TARGET=dsd-player
CXXFLAGS=-std=c++11 -Wall -O2 -lasound -L../alsa-lib/src/.libs -I../alsa-lib/include

all: $(TARGET)
clean:
	rm -f $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<
