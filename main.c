/*
 * This file is part of nerorip. (c)2011 Joe Balough
 *
 * Nerorip is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nerorip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with nerorip.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> // strerror()
#include <stdint.h> // uintXX_t types
#include "util.h"
#include "nrg.h"

#define VERSION "0.2"


void usage(char *argv0) {
  fprintf(stderr, "Usage: %s [OPTIONS]... [INPUT FILE] [OUTPUT DIRECTORY]\n", argv0);
  fprintf(stderr, "Nerorip takes a nero image file (.nrt extension) as input\n");
  fprintf(stderr, "and attempts to extract the track data as either ISO or audio data.\n\n");
  fprintf(stderr, "  -i, --info\t\tOnly disply information about the image file, do not rip\n");
  fprintf(stderr, "  -v, --verbose\t\tIncrement program verbosity by one tick\n");
  fprintf(stderr, "  -q, --quiet\t\tDecrement program verbosity by one tick\n");
  fprintf(stderr, "             \t\tVerbosity starts at 1, a verbosity of 0 will print nothing.\n");
  fprintf(stderr, "  -h, --help\t\tDisplay this help message and exit\n");
  fprintf(stderr, "      --version\t\tOutput version information and exit.\n\n");
  fprintf(stderr, "If output directory is omitted, image data is put in the same directory as the input file.\n\n");

  fprintf(stderr, "For each track found in the image, nerorip will output the following:\n");
  fprintf(stderr, "  one iso file named \"data.sSStTT.iso\" if the track is data and\n");
  fprintf(stderr, "  one wav file named \"audio.sSStTT.wav\" if the track is audio\n");
  fprintf(stderr, "where SS is the session number and TT is the track number.\n");
  fprintf(stderr, "Also, for each session in the iamge, nerorip will output one cue file.");

  fprintf(stderr, "For example, if your disc image is like the following\n");
  fprintf(stderr, "  Session 1:\n    Track 1: Audio\n    Track 2: Data\n  Session 2:\n    Track 1: Data\n");
  fprintf(stderr, "nerorip will output the following files:\n");
  fprintf(stderr, "  audio.s01t01.wav, data.s01t02.iso, data.s02t03.iso, session01.cue, session02.cue\n");
  fprintf(stderr, "Note that the track number does not reset between sessions.\n\n");

  fprintf(stderr, "Report all bugs at https://github.com/scallopedllama/nerorip\nVersion %s\n", VERSION);
  exit(EXIT_FAILURE);
}


int main(int argc, char **argv) {
  // TODO: Actually parse arguments.
  if (argc <= 1) {
    usage(argv[0]);
  }

  // Open file
  char *input_str = argv[1];
  FILE *image_file = fopen(input_str, "rb");
  if (image_file == NULL) {
    fprintf(stderr, "Error opening %s: %s\n", input_str, strerror(errno));
    exit(EXIT_FAILURE);
  }
  
  ver_printf(2, "Allocating memory and getting image version\n");
  nrg_image *image = malloc(sizeof(nrg_image));
  if (get_nrg_version(image_file, image) == NOT_NRG) {
    fprintf(stderr, "File not a Nero image. Aborting.\n");
    exit(EXIT_FAILURE);
  }

  printf("Image is a version %d Nero image with first chunk offset of 0x%X\n", image->nrg_version, image->first_chunk_offset);

  // Seek to the location of that first chunk
  fseek(image_file, image->first_chunk_offset, SEEK_SET);
  while (1)
    process_next_chunk(image_file);

  // Cloes file and free ram
  fclose(image_file);
  free(image);
}

