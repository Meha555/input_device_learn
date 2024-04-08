CFLAGS = -Wall -g -o0
CFLAGS += `pkg-config --cflags --libs libinput`
CFLAGS += `pkg-config --cflags --libs libudev`

simplest: simplest.cpp
	$(CXX) $(CFLAGS) $^ -o $@

clean:
	$(RM) simplest
.PHONY: clean