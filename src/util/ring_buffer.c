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

#include "ring_buffer.h"

#include <stdlib.h>

Ring *ring_alloc(size_t max_size) {
  /*
   * Initialises and returns a handle for a ring buffer which can
   * store at most max_size bytes.  Returns NULL if memory cannot be alloc'd.
   */

  Ring *r = malloc(sizeof(Ring));
  if (r == NULL) {
    return NULL;
  }

  r->buf = malloc(max_size + 1);
  if (r->buf == NULL) {
    free(r);
    return NULL;
  }

  r->size = max_size + 1;
  r->buf_start = r->buf;
  r->buf_end = r->buf;
  return r;
}
  

int ring_read(FILE *fin, Ring *r, uint8_t *couldnt_read) {
  /*
   * Reads data from a file into the ring buffer until the buffer is
   * full.  Returns the number of bytes read.  Sets the EOF and error
   * flags on fin appropriately.  If less than the requested number of
   * bytes was read, sets couldnt_read, otherwises clears it.
   */
  int total_read = 0;
  int to_read, read;

  if (r->buf_start <= r->buf_end) {
    // If the buffer has two spaces, fill the first one
    if (r->buf_start == r->buf) {
      // No opportunity to keep a gap of 1 bytes between beginning and end later, need it now
      to_read = r->buf + r->size - r->buf_end - 1;
    } else {
      to_read = r->buf + r->size - r->buf_end;
    }
    if (to_read > 0) {
      read = fread(r->buf_end, 1, to_read, fin);
      if (read < to_read) {
	r->buf_end += read;
	*couldnt_read = 1;
	return read;
      }
      r->buf_end += read;
      if (r->buf_end >= r->buf + r->size) {
	r->buf_end -= r->size;
      }
      total_read += read;
    }
  }
  
  // The buffer now has a single continuous empty space, so fill that
  to_read = r->buf_start - r->buf_end - 1;
  if (to_read <= 0) return total_read;
  read = fread(r->buf_end, 1, to_read, fin);
  total_read += read;
  r->buf_end += read;
  if (read < to_read) {
    *couldnt_read = 1;
  } else {
    *couldnt_read = 0;
  }
  return total_read;

}


void ring_free(Ring *r) {
  free (r->buf);
  free (r);
}


size_t ring_get_fill(Ring *r) {
  /*
   * Returns the number of bytes currently in the buffer.
   */
  if (r->buf_end >= r->buf_start) {
    return r->buf_end - r->buf_start;
  } else {
    return r->size - (r->buf_start - r->buf_end);
  }
}


int ring_get_exact(Ring *r, size_t len, uint8_t *buf) {
  /*
   * Gets len bytes from the ring and writes them to buf.  If this
   * number of bytes cannot be fetched (i.e. there is not enough data
   * in the buffer), makes no modifications and returns a negative
   * number.  Otherwise returns 0.
   *
   * @ return 0 for success; -1 if there were not enough bytes in the
   * buffer at this time; -2 of there will never be enough bytes in
   * the buffer because of the size of the buffer
   */

  int to_copy;

  if (ring_get_fill(r) < len) {
    if (r->size-1 < len) {
      return -2;
    }
    return -1;
  }
  
  if (r->buf_start + len > r->buf + r->size) {
    // Need to read from two separate bits of memory
    to_copy = r->buf + r->size - r->buf_start;
    memcpy(buf, r->buf_start, to_copy);
    len -= to_copy;
    buf += to_copy;
    r->buf_start = r->buf;
  }

  // Now, we are guaranteed to need to copy from a single continuous
  // bit of memory starting at buf_start
  memcpy(buf, r->buf_start, len);
  r->buf_start += len;
  return 0;
}


int ring_skip (Ring *r, size_t len) {
  /*
   * Discards the next len bytes from the buffer.  Returns 0 on
   * success, -1 if there were not enough bytes in the buffer, or -2
   * if there will never be enough bytes because of the size of the
   * buffer.  In either of the last two cases the buffer is
   * unmodified.
   */

  if (ring_get_fill(r) < len) {
    if (r->size-1 < len) {
      return -2;
    }
    return -1;
  }

  r->buf_start += len;
  if (r->buf_start >= r->buf + r->size) {
    r->buf_start -= r->size;
  }

  return 0;
}
