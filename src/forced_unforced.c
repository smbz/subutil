/*
 *  Copyright Andrew Ryrie 2013
 *
 *  This file is part of subutil.
 *
 *  subutil is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Foobar is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/ring_buffer.h"

#define BUF_MAX_SIZE 65536

enum segment_type {
  PALETTE_SEGMENT      = 0x14,
  PICTURE_SEGMENT      = 0x15,
  PRESENTATION_SEGMENT = 0x16,
  WINDOW_SEGMENT       = 0x17,
  DISPLAY_SEGMENT      = 0x80,
};

int get_be16(uint8_t*buf);

int main (int argc, char **argv) {

  if (argc != 2) {
    printf("Analyzes numbers of forced and unforced subtitles in a PGS stream.\n");
    printf("Usage: %s <input_file.pgs>\n", argv[0]);
    return 127;
  }
  
  FILE *fin;
  fin = fopen (argv[1], "rb");

  Ring *ring = ring_alloc(BUF_MAX_SIZE);
  if (ring == NULL) {
    fprintf(stderr, "malloc fail\n");
    return -1;
  }

  uint8_t *buf = malloc(BUF_MAX_SIZE);
  if (buf == NULL) {
    fprintf(stderr, "malloc fail\n");
    return -1;
  }

  unsigned int forced_objects = 0;
  unsigned int forced_presentations = 0;

  while (1) {

    uint8_t couldnt_read;
    ring_read (fin, ring, &couldnt_read);

    if (ring_get_exact(ring, 3, buf)) {
      break;
    }
    int segment_type = *buf;
    int segment_length = get_be16(buf+1);

    if (ring_get_exact(ring, segment_length, buf)) {
      fprintf (stderr, "Not enough data for a segment of length %d; try increasing buffer size\n", segment_length);
      return -1;
    }
    switch (segment_type) {
    case PALETTE_SEGMENT:
      //      printf("Palette, length %d\n", segment_length);
      break;
    case PICTURE_SEGMENT:
      //printf("Picture, length %d\n", segment_length);
      break;
    case WINDOW_SEGMENT:
      //printf("Window, length %d\n", segment_length);
      break;
    case DISPLAY_SEGMENT:
      //printf("Display, length %d\n", segment_length);
      break;
    default:
      printf("Unknown segment 0x%x, length %d\n", segment_type, segment_length);
      break;
    case PRESENTATION_SEGMENT:
      //printf("Presentation, length %d\n", segment_length);
      segment_length -= 11;
      int nr_objects = buf[10];
      uint8_t*buf_cur = buf + 11;
      if (segment_length != 8*nr_objects) {
	fprintf(stderr, "Inconsistency in presentation segment - expected %d objects, but data present for %d", nr_objects, segment_length/8);
	return -1;
      }
      int i;
      int forced_before = 0;
      for (i=0; i < nr_objects; i++) {
	int forced = buf_cur[8*i+3] & 0x40;
	if (forced) {
	  printf("Forced\n");
	  forced_objects++;
	  if (!forced_before) {
	    forced_before = 1;
	    forced_presentations++;
	  }
	}
      }
    }
  }

  printf("TOTAL: %d forced objects in %d presentation segments\n", forced_objects, forced_presentations);

  return 0;

}


int get_be16(uint8_t*buf) {
  uint16_t val = ((uint16_t)(*buf)) << 8;
  val |= (uint16_t)(buf[1]);
  return val;
}
