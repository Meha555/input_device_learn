CFLAGS = -Wall -g -o0
CFLAGS += `pkg-config --cflags --libs libevdev`

evtest: evtest.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(RM) evtest
.PHONY: clean