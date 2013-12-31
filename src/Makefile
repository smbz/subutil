CC?=gcc
CFLAGS=$(CCFLAGS) -Wall -O3
EXECUTABLES=forced_unforced srt_offset

.PHONY: util

all: $(EXECUTABLES)

util:
	make -C util all

clean:
	rm -f $(EXECUTABLES)
	make -C util clean

forced_unforced: util/ring_buffer.o
	$(CC) $(CFLAGS) forced_unforced.c util/ring_buffer.o -o forced_unforced
