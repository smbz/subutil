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

typedef struct {

  // The ID number of the subtitle.  If the file format itself does not have an ID, a unique ID is generated internally.
  unsigned int id;

  // When the subtitles appears, measured from the beginning of the video, in milliseconds
  unsigned long start;

  // When the subtitle disappears, measured from the beginning of the video, in milliseconds
  unsigned long end;

  // The text of the subtitle; this may be realloc'd as necessary
  char* text;

  // The current length of the buffer pointed to by text
  unsigned int buf_len;

  // The current length of the string in the buffer
  unsigned int len;

} sub_text;
