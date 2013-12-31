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
 *
 *
 *  NB This program uses getline() from glibc; this should be fine on
 *  linux, but won't work on mac/windows/anything which doesn't use
 *  glibc.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage(char* executable_name) {
  printf("Usage: %s <input.srt> <output.srt>\n", executable_name);
  printf("Modifies the timestamps of srt subtitles according to the following options:\n");
  printf("  -t seconds Translates the input by a number of seconds, i.e.\n");
  printf("             the value given is added to each timestamp.\n");
  printf("             Positive numbers make the subtitles later, negative\n");
  printf("             numbers make them sooner.  May be floating-point or\n");
  printf("             integer.\n");
  printf("  -f factor  Applies a multiplicative factor to all subtitle\n");
  printf("             timestamps.  This is applied before any translation.\n");
}

int main(int argc, char **argv) {

  if (argc < 3) {
    usage(argv[0]);
    return 127;
  }

  int i = 1;

  // The translation to apply to the timestamps, in milliseconds
  int translation = 0;

  // The multiplicative factor to apply to the timestamps, in ppm
  // difference from unity
  int factor = 0;

  char* fin_name = NULL;
  char* fout_name = NULL;

  while (i < argc) {
    if (strncmp(argv[i], "-", 1)) {
      // Not an option, must be in/out file
      if (fin_name == NULL) {
        fin_name = argv[i];
      } else if (fout_name == NULL) {
        fout_name = argv[i];
      } else {
        usage(argv[0]);
        return 127;
      }
    } else {

      ++i;
      // This is an option; check there is an associated argument
      if (i >= argc) {
        usage(argv[0]);
        return 127;
      }

      if (!strncmp(argv[i-1]+1, "t", 2)) {
        // Translation
        double t;
        if (sscanf(argv[i], "%lf", &t) != 1) {
          usage(argv[0]);
          return 127;
        }
        translation = (int) (t * 1000.0);
      } else if (!strncmp(argv[i-1]+1, "f", 2)) {
        // Multiplication
        double f;
        if (sscanf(argv[i], "%lf", &f) != 1) {
          usage(argv[0]);
          return 127;
        }
        factor = (int) ((f-1.0) * 1e6);
      } else {
        usage(argv[0]);
        return 127;
      }
    }

    ++i;
  }

  // Make sure we have an input and output file
  if (fin_name == NULL || fout_name == NULL) {
    usage(argv[0]);
    return 127;
  }

  // Open the input and output files
  FILE *fin = fopen(fin_name, "rt");
  if (fin == NULL) {
    fprintf(stderr, "Error opening input file %s: %s\n", fin_name, strerror(errno));
    return 1;
  }
    
  FILE *fout = fopen(fout_name, "wt");
  if (fout == NULL) {
    fprintf(stderr, "Error opening output file %s: %s\n", fout_name, strerror(errno));
    return 1;
  }

  /*
   * Keep track of what we expect next from the input file.  Start at
   * STATE_INITIAL, read a subtitle number, transition to
   * STATE_EXPECT_TIMES, read times, transition to
   * STATE_EXPECT_SUBTITLES, read subtitles, repeat.
   */
  typedef enum {
    STATE_INITIAL,
    STATE_EXPECT_TIMES,
    STATE_EXPECT_SUBTITLES
  } State;
  State state = STATE_INITIAL;

  // The id number of the current subtitle
  unsigned int id;

  // Whether the current subtitles are being copied or skipped
  uint8_t copy = 1;

  // loop through the input and do the actual processing
  char *line = NULL;
  size_t len;
  unsigned int line_no = 0;
  char *delimiter = NULL;
  while (!feof(fin)) {

    getline (&line, &len, fin);
    ++line_no;

    // Strip the trailing newline, and update the delimiter to \r\n or \n
    size_t line_len = strlen(line);
    if (line_len >= 2) {
      if (line[line_len-2] == '\r' && line[line_len-1] == '\n') {
        delimiter = "\r\n";
        line[line_len-2] = 0;
        line_len -= 2;
      } else if (line[line_len-1] == '\n') {
        delimiter = "\n";
        line[line_len-1] = 0;
        --line_len;
      }
    } else if (line_len == 1) {
      if (*line == '\n') {
        delimiter = "\n";
        *line = 0;
        line_len = 0;
      }
    }
    
    if (state == STATE_INITIAL) {
      // Expect a subtitle ID number
      if (*line == 0) continue;
      if (sscanf(line, "%u", &id) != 1) {
        fprintf(stderr, "Parse error at line %u of %s: expected subtitle ID number, but got '%s'\n", line_no, fin_name, line);
        return 2;
      }
      state = STATE_EXPECT_TIMES;
      continue;
    } else if (state == STATE_EXPECT_TIMES) {
      if (*line == 0) continue;
      unsigned int start_hr, start_min, start_sec, start_msec;
      unsigned int end_hr, end_min, end_sec, end_msec;
      if (sscanf(line, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u",
                 &start_hr, &start_min, &start_sec, &start_msec,
                 &end_hr, &end_min, &end_sec, &end_msec) != 8) {
        fprintf(stderr, "Parse error at line %u of %s: expected subtitle start and end time, but got '%s'\n", line_no, fin_name, line);
        return 2;
      }
      long start = start_hr*3600000 + start_min*60000 + start_sec*1000 + start_msec;
      long end = end_hr*3600000 + end_min*60000 + end_sec*1000 + end_msec;
      start += start * factor / 1000000;
      end += end * factor / 1000000;
      start += translation;
      end += translation;

      // Deal with special cases - if the subtitle gets moved to before t=0
      // By default, the subtitles do get written to the output
      copy = 1;
      if (start < 0) {
        if (end < 0) {
          copy = 0;
        } else {
          start = 0;
        }
      }	

      if (copy) {
        start_msec = start % 1000;
        start /= 1000;
        start_sec = start % 60;
        start /= 60;
        start_min = start % 60;
        start /= 60;
        start_hr = start;
 
        end_msec = end % 1000;
        end /= 1000;
        end_sec = end % 60;
        end /= 60;
        end_min = end % 60;
        end /= 60;
        end_hr = end;
    
        fprintf(fout, "%u%s", id, delimiter);
        fprintf(fout, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u%s",
                start_hr, start_min, start_sec, start_msec,
                end_hr, end_min, end_sec, end_msec, delimiter);
      }
      state = STATE_EXPECT_SUBTITLES;
      continue;
    } else if (state == STATE_EXPECT_SUBTITLES) {
      if (*line == 0) {
        fprintf(fout, "%s", delimiter);
        state = STATE_INITIAL;
        continue;
      }
      if (copy) fprintf(fout, "%s%s", line, delimiter);
      continue;
    }
  }

  free(line);

  fclose(fin);
  fclose(fout);

  return 0;
}
