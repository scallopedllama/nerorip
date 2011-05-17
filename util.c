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

#include "util.h"


/**
 * Global variable verbosity
 *
 * Indicates what level of verbosity the user wants to see.
 * Defaults to 1, can be dropped to 0 (print nothing) with the --quiet, -q option.
 * --verbose, -v increse it one value.
 */
int verbosity = 1;

// Increments verbosity
void inc_verbosity() {
  verbosity++;
}

// Decrements verbosity
void dec_verbosity() {
  verbosity--;
}

// Returns the verbosity
int get_verbosity() {
  return verbosity;
}


// printf wrapper to only print if verbosity requirment is met
int ver_printf(int v, char* fmt, ...) {
  // Make sure the verbosity level is ok
  if (v > verbosity)
    return 0;

  // Pass the rest off to printf
  va_list ap;
  va_start(ap, fmt);
  int r = vprintf(fmt, ap);
  va_end(ap);
  return r;
}


// File reading convenience functions
uint8_t fread8u(FILE* input) {
  uint8_t r = 0;
  if (fread(&r, sizeof(uint8_t), 1, input) != 1)
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
  return r;
}

uint16_t fread16u(FILE* input) {
  uint16_t r = 0;
  if (fread(&r, sizeof(uint16_t), 1, input) != 1)
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
  return bswap_16(r);
}

uint32_t fread32u(FILE* input) {
  uint32_t r = 0;
  if (fread(&r, sizeof(uint32_t), 1, input) != 1)
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
  return bswap_32(r);
}

uint64_t fread64u(FILE* input) {
  uint64_t r = 0;
  if (fread(&r, sizeof(uint64_t), 1, input) != 1)
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
  return bswap_64(r);
}

