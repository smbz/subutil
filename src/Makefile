CC?=gcc
CFLAGS=$(CCFLAGS) -Wall -g
EXECUTABLES=forced_unforced srt_offset

.PHONY: util

all: $(EXECUTABLES)

util:
	make -C util all

clean:
	rm -f $(EXECUTABLES)
	make -C util clean

forced_unforced: forced_unforced.c util/ring_buffer.o

srt_offset: srt_offset.c util/srt.o
