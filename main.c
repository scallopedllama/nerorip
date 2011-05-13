#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> // strerror()
#include <stdint.h> // uintXX_t types
#include <byteswap.h> // for bswap_XX functions

#define NER5 0x4e455235
#define NERO 0x4e45524f
#define version1 1
#define version2 2


/**
 * File reading convenience functions
 *
 * These functions read an unsigned integer of the indicated size from the
 * passed file and handles read errors and the like.
 * Before returning, convert the number from little endian to big endian.
 */
uint16_t fread16u(FILE* input) {
  uint16_t r;
  if (fread(&r, sizeof(uint16_t), 1, input) != 1) {
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  return bswap_16(r);
}

uint32_t fread32u(FILE* input) {
  uint32_t r;
  if (fread(&r, sizeof(uint32_t), 1, input) != 1) {
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  return bswap_32(r);
}

uint64_t fread64u(FILE* input) {
  uint64_t r;
  if (fread(&r, sizeof(uint64_t), 1, input) != 1) {
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  return bswap_64(r);
}



int main(int argc, char **argv) {
  // TODO: Actually parse some command line arguments here
  FILE *input_image = fopen("test.nrg", "rb");
  if (input_image == NULL) {
    fprintf(stderr, "Error opening test.nrg: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  int image_ver;
  // The header is actually stored in the last 8 or 12 bytes of the file, depending on the version.
  // Seek to 12 bytes from the end and see if it's version 2. If so, get the header.
  uint64_t first_chunk_offset;
  fseek(input_image, -12, SEEK_END);

  if (fread32u(input_image) == NER5) {
    first_chunk_offset = fread64u(input_image);
    image_ver = version2;
  }
  else if (fread32u(input_image) == NERO) {
    first_chunk_offset = (uint64_t) fread32u(input_image);
    image_ver = version1;
  }
  else {
    fprintf(stderr, "This does not appear to be a Nero image. Aborting.\n");
    exit(EXIT_FAILURE);
  }

  printf("Image is a version %d Nero image with first chunk offset of %X\n", image_ver, first_chunk_offset);


  fclose(input_image);
}

