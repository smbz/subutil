CC?=gcc
CFLAGS=$(CCFLAGS) -Wall -O3
EXECUTABLES=forced_unforced srt_offset srt_interpolate srt_renumber

.PHONY: util

all: $(EXECUTABLES)

util:
	make -C util all

clean:
	rm -f $(EXECUTABLES)
	make -C util clean

forced_unforced: forced_unforced.c util/ring_buffer.o

srt_offset: srt_offset.c util/srt.o

srt_interpolate: srt_interpolate.c util/srt.o

srt_renumber: srt_renumber.c util/srt.o
