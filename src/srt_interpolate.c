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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/srt.h"
#include "util/subtitles.h"

void usage(char *executable_name) {
  printf("Usage: %s id,time [id,time ...] <input.srt> <output.srt>\n", executable_name);
  printf("Interpolate/extrapolate the timestamps on SRT subtitles\n");
  printf("so that subtitles with the given IDs occur at the corresponding\n");
  printf("timestamps.  The time can be in hr:min:sec.msec format, or can just\n");
  printf("be in seconds.  The ID is an unsigned integer corresponding to the\n");
  printf("ID in the SRT input file.\n");
}


unsigned int INITIAL_MAX_POINTS = 8;


typedef struct {
  unsigned int id;
  long time_initial;
  long time_final;

  // Amount to offset and multiply everything after the previous point but before this by
  long ppm;
  long offset;
} point;


int main (int argc, char **argv) {

  if (argc < 4) {
    usage(argv[0]);
    return 127;
  }

  point* points = NULL;
  int nr_points = 0;
  int max_points = 0;
  
  points = malloc(INITIAL_MAX_POINTS*sizeof(point));
  if (points == NULL) {
    fprintf(stderr, "OOM\n");
    return 1;
  }
  max_points = INITIAL_MAX_POINTS;
  
  int arg;
  int i;
  for (arg=1; arg < argc-2; ++arg) {
    if (nr_points == max_points) {
      point* new = realloc(points, 2*max_points*sizeof(point));
      if (new == NULL) {
        fprintf(stderr, "OOM\n");
        return 1;
      }
      points = new;
      max_points *= 2;
    }
    unsigned int id;
    double time_float;
    unsigned long time;
    unsigned int hr=0, min=0;
    if (sscanf(argv[arg], "%u,%lf", &id, &time_float) != 2) {
      if (sscanf(argv[arg], "%u,%u:%lf", &id, &min, &time_float) != 3) {
        if (sscanf(argv[arg], "%u,%u:%u:%lf", &id, &hr, &min, &time_float) != 4) {
          usage(argv[0]);
          return 127;
        }
      }
    }
    if (time_float < 0) {
      usage(argv[0]);
      return 127;
    }
    time = (unsigned long) (time_float * 1000) + ((unsigned long)hr)*3600000 + ((unsigned long)min)*60000;

    // Insertion sort the new point into the list of points by ID
    for (i=0; i < nr_points && points[i].id < id; ++i) {}
    
    if (i > 0) {
      if (points[i-1].time_final > time) {
        fprintf(stderr, "Error: times should increase monotonically with ID\n");
      }
    }
    if (i < nr_points-1) {
      if (points[i].time_final < time) {
        fprintf(stderr, "Error: times should increase monotonically with ID %d\n", i);
      }
    }

    memmove(points+i+1, points+i, (nr_points-i)*sizeof(point));

    points[i].id = id;
    points[i].time_final = time;

    ++nr_points;
  }


  srt_file* fin = srt_open_read(argv[argc-2]);
  if (fin == NULL) {
    fprintf(stderr, "Could not open %s for reading: %s\n", argv[argc-2], strerror(errno));
    return 2;
  }

  srt_file* fout = srt_open_write(argv[argc-1]);
  if (fout == NULL) {
    fprintf(stderr, "Could not open %s for writing: %s\n", argv[argc-1], strerror(errno));
    return 2;
  }

  // Make a first pass through the file, populating the initial times
  sub_text sub;
  sub.text = NULL;
  sub.len = 0;
  int error = srt_read(fin, &sub);
  i = 0;
  while (!error && i < nr_points) {
    if (sub.id == points[i].id) {
      points[i].time_initial = sub.start;
      ++i;
    }
    error = srt_read(fin, &sub);
  }

  if (error != SRT_EOF && error != 0) {
    fprintf(stderr, "Error reading from %s: %s (%d)\n", argv[argc-2], srt_strerror(error), error);
    return 2;
  }
  
  if ((error = srt_seek_beginning(fin))) {
    fprintf(stderr, "Error seeking in %s: %s\n", argv[argc-2], srt_strerror(error));
    return 2;
  }

  // Go through the points and calculate the interpolation coefficients for each segment
  if (nr_points == 1) {
    points[0].ppm = 0;
    points[0].offset = points[0].time_final - points[0].time_initial;
  } else {
    for (i=1; i < nr_points; ++i) {
      points[i].ppm = (points[i].time_final - points[i-1].time_final);
      points[i].ppm *= 1000000;
      points[i].ppm /= (points[i].time_initial - points[i-1].time_initial);
      points[i].ppm -= 1000000;
      points[i].offset = points[i].time_final - points[i].time_initial - points[i].ppm * points[i].time_initial / 1000000;
    }
    points[0].ppm = points[1].ppm;
    points[0].offset = points[1].offset;
  }


  // Make a second pass, this timing adjusting the timestamps and writing
  error = srt_read(fin, &sub);
  i = 0;
  while (!error) {
    if (points[i].time_initial < sub.start && i+1 < nr_points) ++i;
    sub.start += points[i].ppm * sub.start / 1000000;
    sub.end += points[i].ppm * sub.end / 1000000;
    sub.start += points[i].offset;
    sub.end += points[i].offset;
    if ((error = srt_write(fout, &sub))) {
      fprintf(stderr, "Error writing to %s: %s\n", argv[argc-1], srt_strerror(error));
      return 2;
    }
    error = srt_read(fin, &sub);
  }

  srt_close(fin);
  srt_close(fout);
  if (sub.text != NULL) {
    free(sub.text);
  }
  free(points);

  if (error != SRT_EOF) {
    fprintf(stderr, "Error reading from %s: %s\n", argv[argc-2], srt_strerror(error));
  }

  return 0;

}
