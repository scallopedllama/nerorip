#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> // strerror()
#include <stdint.h> // uintXX_t types


uint16_t fread16(FILE* input) {
  uint16_t r;
  if (fread(&r, sizeof(uint16_t), 1, input) != 1) {
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  return r;
}

uint32_t fread32(FILE* input) {
  uint32_t r;
  if (fread(&r, sizeof(uint32_t), 1, input) != 1) {
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  return r;
}

uint64_t fread64(FILE* input) {
  uint64_t r;
  if (fread(&r, sizeof(uint64_t), 1, input) != 1) {
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  return r;
}



int main(int argc, char **argv) {
  // TODO: Actually parse some command line arguments here
  FILE *input_image = fopen("test.nrg", "rb");
  if (input_image == NULL) {
    fprintf(stderr, "Error opening test.nrg: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // The header is actually stored in the last 8 or 12 bytes of the file, so seek to those positions and get reading
  fseek(input_image, -12, SEEK_END);
  uint32_t v2_head_txt = fread32(input_image);
  uint64_t v2_file_offset = fread64(input_image);


  fseek(input_image, -8, SEEK_END);
  uint32_t v1_head_txt = fread32(input_image);
  uint32_t v1_file_offset = fread32(input_image);

  printf("If version 1: Head text: %x, File offset %x\n", v1_head_txt, v1_file_offset);
  printf("If version 2: Head text: %x, File offset %x\n", v2_head_txt, v2_file_offset);


  fclose(input_image);
}

