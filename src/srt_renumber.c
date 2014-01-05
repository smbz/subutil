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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/srt.h"
#include "util/subtitles.h"

void usage(char* executable_name) {
  printf("Usage: %s <input.srt> <output.srt>\n", executable_name);
  printf("Changes the IDs in an SRT file to be numbers from 1 to the total number of subtitles in the file.\n");
}

int main(int argc, char **argv) {

  if (argc != 3) {
    usage(argv[0]);
    return 127;
  }

  char* fin_name = argv[1];
  char* fout_name = argv[2];

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
  int id = 1;
  while (!error) {
    sub.id = id++;
    if ((error = srt_write(fout, &sub))) break;
    error = srt_read(fin, &sub);
  }

  srt_close(fin);
  srt_close(fout);
  if (sub.text != NULL) {
    free(sub.text);
  }

  if (error != SRT_EOF) {
    fprintf(stderr, "Error at input line %u: %s\n", fin->line_no, srt_strerror(error));
    return 2;
  }

  return 0;
}
