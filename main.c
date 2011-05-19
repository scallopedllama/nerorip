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

#define AUD_WAV  0
#define AUD_RAW  1
#define AUD_CDA  2
#define AUD_AIFF 3

#define DAT_ISO 0
#define DAT_BIN 1
#define DAT_MAC 2

/**
 * Whether only information about the file should be printed
 */
static int info_only = 0;

/**
 * What format the data tracks should be saved as
 * Should be DAT_ISO, DAT_BIN, or DAT_MAC
 */
static int data_output = DAT_ISO;

// An array of strings describing the data output options
static char data_output_str[3][26] = {"converted ISO/2048", "raw BIN", "converted \"Mac\" ISO/2048"};

/**
 * What format the audio tracks should be output as
 * Should be AUD_WAV, AUD_RAW, AUD_CDA, or AUD_AIFF
 */
static int audio_output = AUD_WAV;

/**
 * Whether or not audio tracks should be swapped
 */
static int swap_audio_output = 0;

// An array of strings to represent the audio output types
static char audio_output_str[4][5] = {"WAV", "RAW", "CDA", "AIFF"};

void usage(char *argv0) {
  printf("Usage: %s [OPTIONS]... [INPUT FILE] [OUTPUT DIRECTORY]\n", argv0);
  printf("Nerorip takes a nero image file (.nrt extension) as input\n");
  printf("and attempts to extract the track data as either ISO or audio data.\n\n");
  printf("  Audio track saving options:\n");
  printf("    -r, --raw\t\tSave audio data as little endian raw data\n");
  printf("    -c, --cda\t\tSwitches data to big endian and saves as RAW\n");
  printf("    -a, --aiff\t\tSwitches data to big endian and saves as an AIFF file\n");
  printf("    -s, --swap\t\tChanges data between big and little endian (only affects --aiff and --cda)\n");
  printf("  If omitted, Audio tracks will be exported as WAV files\n\n");

  printf("  Data track saving options:\n");
  printf("    -b, --bin\t\tExport data directly out of image file\n");
  printf("    -m, --mac\t\tConvert data to \"Mac\" ISO/2056 format\n");
  printf("  If omitted, Data tracks will be converted to ISO/2048 format\n\n");

  printf("  General options:\n");
  printf("  -i, --info\t\tOnly disply information about the image file, do not rip\n");
  printf("  -v, --verbose\t\tIncrement program verbosity by one tick\n");
  printf("  -q, --quiet\t\tDecrement program verbosity by one tick\n");
  printf("             \t\tVerbosity starts at 1, a verbosity of 0 will print nothing.\n");
  printf("  -h, --help\t\tDisplay this help message and exit\n");
  printf("      --version\t\tOutput version information and exit.\n\n");
  printf("If output directory is omitted, image data is put in the same directory as the input file.\n\n");

  printf("For each track found in the image, nerorip will output the following:\n");
  printf("  one iso file named \"data.sSStTT.[iso/bin]\" if the track is data and\n");
  printf("  one wav file named \"audio.sSStTT.[wav/bin/cda/aiff]\" if the track is audio\n");
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
    // Audio
    {"raw",      no_argument, 0, 'r'},
    {"cda",      no_argument, 0, 'c'},
    {"aiff",     no_argument, 0, 'a'},
    {"swap",     no_argument, 0, 's'},
    // Data
    {"bin",      no_argument, 0, 'b'},
    {"mac",      no_argument, 0, 'm'},
    // General
    {"info",     no_argument, 0, 'i'},
    {"verbose",  no_argument, 0, 'v'},
    {"quiet",    no_argument, 0, 'q'},
    {"help",     no_argument, 0, 'h'},
    {"version",  no_argument, 0, 'V'},
    {0, 0, 0, 0}
  };

  // Loop through all the passed options
  int c;
  while ((c = getopt_long (argc, argv, "rcasbmivqhV", long_options, NULL)) != -1) {
    switch (c) {
      /*
       * Audio track options
       */

      // Raw
      case 'r': audio_output = AUD_RAW; break;
      // Cda
      case 'c': audio_output = AUD_CDA; break;
      // Aiff
      case 'a': audio_output = AUD_AIFF; break;
      // Swap
      case 's': swap_audio_output = 1; break;

      /*
       * Data track options
       */

      // Bin
      case 'b': data_output = DAT_BIN; break;
      // Mac
      case 'm': data_output = DAT_MAC; break;


      /*
       * General options
       */

      // Verbose
      case 'v': inc_verbosity(); break;
      // Quiet
      case 'q': dec_verbosity(); break;
      // Info
      case 'i': info_only = 1; break;
      // Help
      case 'h': usage(argv[0]); break;
      // Version
      case 'V': version(argv[0]); break;
      default: break;
    }
  }
  // Print simple welcome message
  ver_printf(1, "neorip v%s\n", VERSION);

  // Note any enabled options

  // Note if info_only is on. If it is, that's the only option to tell the user about.
  if (info_only)
    ver_printf(1, "Will only print disc image information.\n");

  else {
    // Check use of swap
    if (audio_output == AUD_WAV && swap_audio_output) {
      ver_printf(1, "Note: --swap option used but makes no sense in WAV output. Ignoring.\n");
      swap_audio_output = 0;
    }
    // Audio track information
    ver_printf(1, "Saving audio tracks as %s%s files.\n", (swap_audio_output ? "swapped " : ""), audio_output_str[audio_output]);

    // Data track information
    ver_printf(1, "Saving data tracks as %s files.\n", data_output_str[data_output]);
  }

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
        fwrite_wav_header(tf, t->length);
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

