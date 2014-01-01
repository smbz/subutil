/*
 *  Copyright Andrew Ryrie 2014
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

#include <stdio.h>
#include <string.h>

#include "subtitles.h"

typedef enum {
  SRT_MODE_READ,
  SRT_MODE_WRITE
} srt_mode;

// Errors which might be encountered while reading; correspond to
// parse errors on the ID line and the start/end times line
int SRT_ERROR_ID;
int SRT_ERROR_TIMES;
int SRT_ERROR_ALLOC;
int SRT_ERROR_WRITE;
int SRT_ERROR_MODE_CANNOT_READ;
int SRT_ERROR_MODE_CANNOT_WRITE;
int SRT_ERROR_PREVIOUS_ERROR;
int SRT_EOF;

typedef struct {

  // The file for reading/writing
  FILE *f;

  // The newline string - either \r\n or \n
  char* delimiter;

  // Either SRT_MODE_READ or SRT_MODE_WRITE
  srt_mode mode;

  // The current line number, for files being read
  unsigned int line_no;

  // A buffer for storing the current line, and its length
  char* line;
  size_t len;

  // Error flag, set if there was an error parsing the file
  int error;
} srt_file;


srt_file* srt_open_read(char* filename);
srt_file* srt_open_write(char* filename);
void srt_close(srt_file* file);
int srt_read(srt_file* file, sub_text* subtitle);
int srt_write(srt_file* file, sub_text* subtitle);
char* srt_strerror(int error_code);
