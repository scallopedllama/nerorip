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
#include <getopt.h> // getopt_long
#include <ctype.h> // getopt_long
#include "util.h"
#include "nrg.h"

#define VERSION "0.2"

/**
 * Whether only information about the file should be printed
 */
static int info_only = 0;

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
  fprintf(stderr, "Also, for each session in the image, nerorip will output one cue file.\n\n");

  fprintf(stderr, "For example, if your disc image is like the following\n");
  fprintf(stderr, "  Session 1:\n    Track 1: Audio\n    Track 2: Data\n  Session 2:\n    Track 1: Data\n");
  fprintf(stderr, "nerorip will output the following files:\n");
  fprintf(stderr, "  audio.s01t01.wav, data.s01t02.iso, data.s02t03.iso, session01.cue, session02.cue\n");
  fprintf(stderr, "Note that the track number does not reset between sessions.\n\n");

  fprintf(stderr, "Report all bugs at https://github.com/scallopedllama/nerorip\nVersion %s\n", VERSION);
  exit(EXIT_FAILURE);
}

void version(char *argv0) {
  printf("%s v%s\n", argv0, VERSION);
  printf("Licensed under GNU LGPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
  printf("This is free software: you are free to change and redistribute it.\n");
  printf("There is NO WARRANTY, to the extent permitted by law.\n");
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  // Configure the long getopt options
  static struct option long_options[] = {
    {"info",     no_argument, 0, 'i'},
    {"verbose",  no_argument, 0, 'v'},
    {"quiet",    no_argument, 0, 'q'},
    {"help",     no_argument, 0, 'h'},
    {"version",  no_argument, 0, 'V'},
    {0, 0, 0, 0}
  };

  // Loop through all the passed options
  int c;
  while ((c = getopt_long (argc, argv, "ivqhV", long_options, NULL)) != -1) {
    switch (c) {
      // Verbose
      case 'v':
        inc_verbosity();
        break;
      // Quiet
      case 'q':
        dec_verbosity();
        break;
      // Info
      case 'i':
        ver_printf(1, "Will only print information.\n");
        info_only = 1;
        break;
      // Help
      case 'h':
        usage(argv[0]);
        break;
      // Version
      case 'V':
        version(argv[0]);
        break;
      default:
        break;
    }
  }
  // Print simple welcome message
  ver_printf(1, "neorip v%s\n", VERSION);

  // Now that all the getopt options have been parsed, that only leaves the input file and output directory.
  // Those two values should be in argv[optind] and argv[optind + 1]
  // Make sure they were actually provided before accessing them
  if (optind == argc) {
    fprintf(stderr, "Error: No input file provided\n\n");
    usage(argv[0]);
  }

  char *input_str = argv[optind];
  ver_printf(2, "Opening file %s\n", input_str);
  FILE *image_file = fopen(input_str, "rb");
  if (image_file == NULL) {
    fprintf(stderr, "Error opening %s: %s\n", input_str, strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Figure out output directory
  char *output_dir = getenv("PWD");
  if (optind + 2 == argc)
    output_dir = argv[optind + 1];
  if (!info_only)
    ver_printf(2, "Outputing data to %s\n", output_dir);

  ver_printf(2, "Allocating memory and getting image version\n");
  nrg_image *image = malloc(sizeof(nrg_image));
  if (get_nrg_version(image_file, image) == NOT_NRG) {
    fprintf(stderr, "File not a Nero image. Aborting.\n");
    exit(EXIT_FAILURE);
  }

  printf("Image is a version %d Nero image with first chunk offset of 0x%X\n", image->nrg_version, image->first_chunk_offset);

  // Seek to the location of that first chunk
  fseek(image_file, image->first_chunk_offset, SEEK_SET);
  process_next_chunk(image_file);

  // Cloes file and free ram
  fclose(image_file);
  free(image);
}

