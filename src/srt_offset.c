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

#include "util/srt.h"
#include "util/subtitles.h"

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
  srt_file* fin = srt_open_read(fin_name);
  if (fin == NULL) {
    fprintf(stderr, "Error opening input file %s: %s\n", fin_name, strerror(errno));
    return 1;
  }
    

  srt_file* fout = srt_open_write(fout_name);
  if (fout == NULL) {
    fprintf(stderr, "Error opening output file %s: %s\n", fout_name, strerror(errno));
    return 1;
  }

  
  sub_text sub;
  sub.text = NULL;
  sub.buf_len = 0;
  int error = srt_read(fin, &sub);
  fout->delimiter = fin->delimiter;
  while (!error) {
    sub.start += sub.start * factor / 1000000;
    sub.end += sub.end * factor / 1000000;
    sub.start += translation;
    sub.end += translation;

    if (sub.end > 0) {
      if (sub.start < 0) sub.start = 0;
      if ((error = srt_write(fout, &sub))) break;
    }

    error = srt_read(fin, &sub);
  }

  if (error != SRT_EOF) {
    fprintf(stderr, "Error at input line %u: %s\n", fin->line_no, srt_strerror(error));
    return 2;
  }

  return 0;
}
