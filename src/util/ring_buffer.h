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

#pragma once

#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef struct {
  uint8_t *buf;
  uint8_t *buf_start;
  uint8_t *buf_end;
  size_t size;
} Ring;

Ring *ring_alloc(size_t max_size);
int ring_read(FILE *fin, Ring *r, uint8_t*couldnt_read);
void ring_free(Ring *r);
size_t ring_get_fill(Ring *r);
int ring_get_exact(Ring *r, size_t len, uint8_t *buf);
int ring_skip (Ring *r, size_t len);
