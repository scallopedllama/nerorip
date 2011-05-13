#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {
  FILE *input_image = fopen("test.nrg", "rb");
  if (input_image == NULL) {
    fprintf(stderr, "Error opening test.nrg: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  fclose(input_image);
}