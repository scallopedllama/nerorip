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

#define AUD_WAV 0
#define AUD_RAW 0


/**
 * Whether only information about the file should be printed
 */
static int info_only = 0;
static int audio_output = AUD_WAV;

void usage(char *argv0) {
  printf("Usage: %s [OPTIONS]... [INPUT FILE] [OUTPUT DIRECTORY]\n", argv0);
  printf("Nerorip takes a nero image file (.nrt extension) as input\n");
  printf("and attempts to extract the track data as either ISO or audio data.\n\n");
  printf("  -r, --raw\t\tSave audio data as RAW (little endian) instead of a WAV file\n");
  printf("  -i, --info\t\tOnly disply information about the image file, do not rip\n");
  printf("  -v, --verbose\t\tIncrement program verbosity by one tick\n");
  printf("  -q, --quiet\t\tDecrement program verbosity by one tick\n");
  printf("             \t\tVerbosity starts at 1, a verbosity of 0 will print nothing.\n");
  printf("  -h, --help\t\tDisplay this help message and exit\n");
  printf("      --version\t\tOutput version information and exit.\n\n");
  printf("If output directory is omitted, image data is put in the same directory as the input file.\n\n");

  printf("For each track found in the image, nerorip will output the following:\n");
  printf("  one iso file named \"data.sSStTT.iso\" if the track is data and\n");
  printf("  one wav file named \"audio.sSStTT.wav\" if the track is audio\n");
  printf("where SS is the session number and TT is the track number.\n");
  printf("Also, for each session in the image, nerorip will output one cue file.\n\n");

  printf("For example, if your disc image is like the following\n");
  printf("  Session 1:\n    Track 1: Audio\n    Track 2: Data\n  Session 2:\n    Track 1: Data\n");
  printf("nerorip will output the following files:\n");
  printf("  audio.s01t01.wav, data.s01t02.iso, data.s02t03.iso, session01.cue, session02.cue\n");
  printf("Note that the track number does not reset between sessions.\n\n");

  printf("Report all bugs at %s\nVersion %s\n", WEBSITE, VERSION);
  exit(EXIT_SUCCESS);
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
    {"raw",      no_argument, 0, 'r'},
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
      // Raw
      case 'r':
        ver_printf(1, "Dumping RAW audio data.\n");
        audio_output = AUD_RAW;
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

  ver_printf(3, "Allocating memory\n");
  nrg_image *image = alloc_nrg_image();

  // Parse the image file
  nrg_parse(image_file, image);
  ver_printf(3, "\n");

  // Print the collected information
  nrg_print(1, image);

  if (info_only)
    goto quit;

  ver_printf(1, "Writing out track data\n");
  // Try to extract that data
  unsigned int track = 1;
  nrg_session *s;
  for(s = image->first_session; s != NULL; s=s->next) {
    nrg_track *t;
    for (t = s->first_track; t!=NULL; t=t->next) {

      // Seek to the track lba
      fseek(image_file, t->index1, SEEK_SET);

      char buffer[256];
      sprintf(buffer, "%s/track%02d.bin", output_dir, track);
      ver_printf(1, "%s: 00%%", buffer);

      // Open up a file to dump stuff into
      FILE *tf = fopen(buffer, "wb");

      // Add WAV header if the track was audio and that's the audio mode
      if (audio_output == AUD_WAV && t->track_mode == AUDIO) {
        // Following WAV header format found at https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
        unsigned int written = fwrite("RIFF", 1, 4, tf);
        written += fwrite32u(t->length + 36, tf); // Length of data + 36
        written += fwrite("WAVE", 1, 4, tf);
        written += fwrite("fmt ", 1, 4, tf);
        written += fwrite32u(16,     tf);  // PCM
        written += fwrite16u(1,      tf);  // No Compression
        written += fwrite16u(2,      tf);  // 2 channels
        written += fwrite32u(44100,  tf);  // Sample Rate
        written += fwrite32u(176400, tf);  // Byte Rate
        written += fwrite16u(4,      tf);  // Block Align
        written += fwrite16u(16,     tf);  // Bits per sample
        written += fwrite("data", 1, 4, tf);
        written += fwrite32u(t->length, tf); // Data length
      }


      // Write length bytes
      unsigned int b;
      for (b = 0; b < t->length; b += t->sector_size)
      {
        ver_printf(1, "\b\b\b%02d%%", (int)( ((float) b / (float) t->length) * 100.0));

        uint8_t *writebuf = malloc(sizeof(uint8_t) * t->sector_size);
        if (fread(writebuf, sizeof(uint8_t), t->sector_size, image_file) != t->sector_size) {
          fprintf(stderr, "Error reading track: %s\n", strerror(errno));
        }
        if (fwrite(writebuf, sizeof(uint8_t), t->sector_size, tf) != t->sector_size) {
          fprintf(stderr, "Error writing track: %s\n  Skipping this track.\n", strerror(errno));
          fclose(tf);
          free(writebuf);
          continue;
        }
        free(writebuf);
      }

      ver_printf(1, "\b\b\b100%%\n");

      // Close that file
      fclose(tf);

      track++;
    }
  }

quit:
  // Cloes file and free ram
  ver_printf(3, "Cleaning up\n");
  fclose(image_file);
  free_nrg_image(image);

  return EXIT_SUCCESS;
}

