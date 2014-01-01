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

#include "srt.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  STATE_INITIAL,
  STATE_EXPECT_TIMES,
  STATE_EXPECT_SUBTITLES
} state;

int SRT_ERROR_ID = -1;
int SRT_ERROR_TIMES = -2;
int SRT_ERROR_ALLOC = -3;
int SRT_ERROR_WRITE = -4;
int SRT_ERROR_MODE_CANNOT_READ = -5;
int SRT_ERROR_MODE_CANNOT_WRITE = -6;
int SRT_ERROR_PREVIOUS_ERROR = -7;
int SRT_EOF = -8;
int SRT_ERROR_SEEK = -9;

srt_file* srt_open_read(char* filename) {
  /* 
   * Opens filename for reading subtitles.  Returns NULL if opening
   * the file failed; errno may be inspected to determine the cause.
   * The file must be closed with srt_close() when it is no longer
   * needed.
   */
  
  FILE* f = fopen(filename, "rt");
  if (f == NULL) {
    return NULL;
  }

  srt_file* file = malloc(sizeof(srt_file));
  if (file == NULL) {
    fclose(f);
    return NULL;
  }

  file->f = f;
  file->delimiter = NULL;
  file->mode = SRT_MODE_READ;
  file->line_no = 0;
  file->error = 0;
  file->line = NULL;
  file->len = 0;

  return file;
}


srt_file* srt_open_write(char* filename) {
  /*
   * Opens an SRT file for writing.  Returns NULL if opening failed.
   * The file must be closed with srt_close when it is no longer
   * needed.  By default, the newline delimiter is set to \r\n
   * (windows-style); this can be changed with srt_set_delimiter.
   */
  
  FILE* f = fopen(filename, "wt");
  if (f == NULL) {
    return NULL;
  }

  srt_file* file = malloc(sizeof(srt_file));
  if (file == NULL) {
    fclose(f);
    return NULL;
  }

  file->f = f;
  file->delimiter = "\r\n";
  file->mode = SRT_MODE_WRITE;
  file->line_no = 0;
  file->error = 0;

  return file;
}


void srt_close(srt_file* file) {
  /*
   * Closes an open file.
   */
  fclose(file->f);
  if (file->line != NULL) {
    free(file->line);
  }
  free(file);
}


int srt_isempty(char*s) {
  /*
   * Returns true if a string is empty, apart from whitespace
   */
  while (*s != 0) {
    if (!isspace(*s)) return 0;
    ++s;
  }
  return 1;
}


int srt_read(srt_file* file, sub_text* subtitle) {
  /*
   * Reads a subtitle from the file into subtitle.  Reallocs the
   * subtitle text buffer if necessary to accomodate the length, and
   * updates buf_len appropriately.  If the subtitle doesn't currently
   * have a text buffer, it is allocated, and must be freed by the *
   * caller.
   *
   * If there was an error reading from the file, or if the file has
   * been opened for reading rather than writing, the error flag in
   * file is set and a negative number is returned.  On success, 0 is
   * returned.
   */

  if (file->mode != SRT_MODE_READ) {
    return SRT_ERROR_MODE_CANNOT_READ;
  }

  if (file->error) {
    return SRT_ERROR_PREVIOUS_ERROR;
  }

  state s = STATE_INITIAL;
  char*line;
  ssize_t line_len;

  unsigned int id;
  unsigned long start=0, end=0;

  while (1) {
    line_len = getline(&file->line, &file->len, file->f);
    line = file->line;
    ++file->line_no;

    // If EOF or some other error
    if (line_len < 0) {
      if (s == STATE_EXPECT_SUBTITLES) {
        s = STATE_INITIAL;
        break;
      } else {
        return SRT_EOF;
      }
    }

    // Detect line delimiters; if we can't detect, default to \r\n
    if (file->delimiter == NULL) {

      if (line_len >= 2) {
        if (line[line_len-1] == '\n') {
          if (line[line_len-2] != '\r') {
            file->delimiter = "\n";
          } else {
            file->delimiter = "\r\n";
          }
        } else {
          file->delimiter = "\r\n";
        }
      } else if (line_len == 1) {
        if (line[line_len-1] == '\n') {
          file->delimiter = "\n";
        } else {
          file->delimiter = "\r\n";
        }
      } else {
        file->delimiter = "\r\n";
      }
    }

    if (s == STATE_INITIAL) {
      // Expect a  subtitle ID
      if (srt_isempty(line)) continue;
      if (sscanf(line, " %u ", &id) != 1) {
        file->error = SRT_ERROR_ID;
        return SRT_ERROR_ID;
      }
      s = STATE_EXPECT_TIMES;
      continue;
    }

    else if (s == STATE_EXPECT_TIMES) {
      if (srt_isempty(line)) continue;
      unsigned int start_hr, start_min, start_sec, start_msec;
      unsigned int end_hr, end_min, end_sec, end_msec;
      if (sscanf(line, " %02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u ",
                 &start_hr, &start_min, &start_sec, &start_msec,
                 &end_hr, &end_min, &end_sec, &end_msec) != 8) {
        file->error = SRT_ERROR_TIMES;
        return SRT_ERROR_TIMES;
      }
      start = start_hr*3600000 + start_min*60000 + start_sec*1000 + start_msec;
      end = end_hr*3600000 + end_min*60000 + end_sec*1000 + end_msec;
      s = STATE_EXPECT_SUBTITLES;
      subtitle->len = 0;
      continue;
    }

    else if (s == STATE_EXPECT_SUBTITLES) {
      if (srt_isempty(line)) {
        s = STATE_INITIAL;
        break;
      }
      if (subtitle->len+line_len+1 > subtitle->buf_len) {
        char*new = realloc(subtitle->text, subtitle->len+line_len+1);
        if (new == NULL) {
          file->error = SRT_ERROR_ALLOC;
          return SRT_ERROR_ALLOC;
        }
        subtitle->text = new;
      }  
      memcpy(subtitle->text+subtitle->len, line, line_len+1);
      subtitle->len += line_len;
    }

  }

  subtitle->id = id;
  subtitle->start = start;
  subtitle->end = end;

  return 0;
}


int srt_write(srt_file* file, sub_text* subtitle) {
  /*
   * Writes subtitle to file.  If an error occurs, file->error is set
   * to an error code and the same error code is returned.  Otherwise,
   * 0 is returned.
   */

  if (file->mode != SRT_MODE_WRITE) {
    return SRT_ERROR_MODE_CANNOT_WRITE;
  }

  if(fprintf(file->f, "%u%s", subtitle->id, file->delimiter) < 0) {
    file->error = SRT_ERROR_WRITE;
    return SRT_ERROR_WRITE;
  }

  unsigned long start = subtitle->start;
  unsigned long end = subtitle->end;
  unsigned int start_hr, start_min, start_sec, start_msec;
  unsigned int end_hr, end_min, end_sec, end_msec;

  start_msec = start % 1000UL;
  start /= 1000UL;
  start_sec = start % 60UL;
  start /= 60UL;
  start_min = start % 60UL;
  start /= 60UL;
  start_hr = start;

  end_msec = end % 1000UL;
  end /= 1000UL;
  end_sec = end % 60UL;
  end /= 60UL;
  end_min = end % 60UL;
  end /= 60UL;
  end_hr = end;
  
  if(fprintf(file->f, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u%s",
             start_hr, start_min, start_sec, start_msec,
             end_hr, end_min, end_sec, end_msec,
             file->delimiter) < 0) {
    file->error = SRT_ERROR_WRITE;
    return SRT_ERROR_WRITE;
  }

  // Need to do newline conversion on the subtitle string
  char*c;
  for (c=subtitle->text; c < subtitle->text+subtitle->len; c++) {
    if (*c == '\n') {
      if (fputs(file->delimiter, file->f) < 0) {
        file->error = SRT_ERROR_WRITE;
        return SRT_ERROR_WRITE;
      }
    } else if (*c == '\r') {
      continue;
    } else {
      if (fputc(*c, file->f) < 0) {
        file->error = SRT_ERROR_WRITE;
        return SRT_ERROR_WRITE;
      }
    }
  }

  if (subtitle->len == 0) {
    if(fputs(file->delimiter, file->f) < 0) {
      file->error = SRT_ERROR_WRITE;
      return SRT_ERROR_WRITE;
    }
  } else if (*(c-1) != '\n') {
    if(fputs(file->delimiter, file->f) < 0) {
      file->error = SRT_ERROR_WRITE;
      return SRT_ERROR_WRITE;
    }
  }

  if(fputs(file->delimiter, file->f) < 0) {
    file->error = SRT_ERROR_WRITE;
    return SRT_ERROR_WRITE;
  }

  return 0;
}


char* srt_strerror(int error_code) {
  /*
   * Returns a human-readable string explaining an error code.
   */
  if (error_code == SRT_ERROR_ID) {
    return "Parse error: expected an integer subtitle ID number";
  } else if (error_code == SRT_ERROR_TIMES) {
    return "Parse error: expected a line giving start and end times for the subtitle";
  } else if (error_code == SRT_ERROR_ALLOC) {
    return "Could not allocate memory";
  } else if (error_code == SRT_ERROR_WRITE) {
    return "Could not write to the output file";
  } else if (error_code == SRT_ERROR_MODE_CANNOT_WRITE) {
    return "This file has been opened for reading, and cannot be written to";
  } else if (error_code == SRT_ERROR_MODE_CANNOT_READ) {
    return "This file has been opened for writing, and cannot be read from";
  } else if (error_code == SRT_ERROR_PREVIOUS_ERROR) {
    return "There was a previous error on this file; cannot resume";
  } else if (error_code == SRT_EOF) {
    return "End of file";
  } else if (error_code == SRT_ERROR_SEEK) {
    return "Cannot seek in this file";
  } else {
    return "Unknown error code";
  }
}


int srt_seek_beginning(srt_file* file) {
  /*
   * Goes back to the beginning of the file.  Only for input files.
   * Returns a negative error code on failure, or 0 on success.
   */
  if (file->mode != SRT_MODE_READ) {
    return SRT_ERROR_MODE_CANNOT_READ;
  }
  if (file->error && file->error != SRT_EOF) {
    return SRT_ERROR_PREVIOUS_ERROR;
  }
  if (fseek(file->f, 0, SEEK_SET)) {
    file->error = SRT_ERROR_SEEK;
    return SRT_ERROR_SEEK;
  }
  return 0;
}
