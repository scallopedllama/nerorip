#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> // strerror()
#include <stdint.h> // uintXX_t types
#include <byteswap.h> // for bswap_XX functions

#define NER5 0x4e455235
#define NERO 0x4e45524f
#define CUES 0x43554553
#define CUEX 0x43554558
#define DAOI 0x44414f49
#define DAOX 0x44414f58
#define CDTX 0x43445458
#define ETNF 0x45544e46
#define ETN2 0x45544e32
#define SINF 0x53494e46
#define MTYP 0x4d545950
#define END  0x454e4421

#define version1 1
#define version2 2


/**
 * File reading convenience functions
 *
 * These functions read an unsigned integer of the indicated size from the
 * passed file and handles read errors and the like.
 * Before returning, convert the number from little endian to big endian.
 */
uint16_t fread8u(FILE* input) {
  uint8_t r;
  if (fread(&r, sizeof(uint8_t), 1, input) != 1) {
    fprintf(stderr, "Error reading from file: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  return r;
}

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




/**
 * Process the next chunk of data starting from the file's current position.
 */
void process_next_chunk(FILE *input_image) {
  // The chunk ID is always a 32 bit integer
  const uint32_t chunk_id = fread32u(input_image);

  if (chunk_id == CUES || chunk_id == CUEX) {
    // CUES and CUEX are both followed by a chunk size value
    uint32_t chunk_size = fread32u(input_image);
    printf("CUE : Size - %d\n", chunk_size);

    // Skip that data and the pointless additional 4 bytes
    fseek(input_image, chunk_size, SEEK_CUR);
  }
  else if (chunk_id == DAOI) {
    /**  DAOI format:
      *
      *   4 B   32bit   Chunk size (bytes) big endian
      *   4 B   32bit   Chunk size (bytes) little endian
      *   14 B          UPC
      *   4 B   32bit   Toc type
      *   1 B   8bit   First track
      *   1 B   8bit   Last track
      *   12 B          ISRC Code
      *   4 B   32bit   Sector size
      *   4 B   32bit   Mode
      *   4 B   32bit   Index0 (Pre gap)
      *   4 B   32bit   Index1 (Start of track)
      *   4 B   32bit   Index2 (End of track + 1)
      */
    // Chunk Size
    uint32_t chunk_size = fread32u(input_image);
    fread32u(input_image);

    // Skip UPC
    fseek(input_image, 14, SEEK_CUR);

    uint32_t toc_type    = fread32u(input_image);
    uint32_t first_track = fread8u(input_image);
    uint32_t last_track  = fread8u(input_image);

    // Skip ISRC Code
    fseek(input_image, 12, SEEK_CUR);

    uint32_t sector_size = fread32u(input_image);
    uint32_t mode        = fread32u(input_image);
    uint32_t index0      = fread32u(input_image);
    uint32_t index1      = fread32u(input_image);
    uint32_t index2      = fread32u(input_image);

    printf("DAOI: Size - %X, Toc Type - %X, First Track - %X, Last Track - %X, Sector Size - %X\n", chunk_size, toc_type, first_track, last_track, sector_size);
    printf("      Mode - %X, Index0 (Pre gap) - %X, Index1 (Start of track) - %X, Index2 (End of track + 1) - %X\n", mode, index0, index1, index2);
  }
  else if (chunk_id == DAOX) {
    /**  DAOI format:
      *
      *   4 B   32bit   Chunk size (bytes) big endian
      *   4 B   32bit   Chunk size (bytes) little endian
      *   14 B          UPC
      *   4 B   32bit   Toc type
      *   1 B   8bit   First track
      *   1 B   8bit   Last track
      *   8 B          ISRC Code
      *   4 B  32bit   Sector size
      *   4 B  32bit   Mode
      *   8 B  64bit   Index0 (Pre gap)
      *   8 B  64bit   Index1 (Start of track)
      *   8 B  64bit   Index2 (End of track + 1)
      *   2 B          Unknown
      */
    // Chunk Size
    uint32_t chunk_size = fread32u(input_image);
    fread32u(input_image);

    // Skip UPC
    fseek(input_image, 14, SEEK_CUR);

    uint32_t toc_type    = fread32u(input_image);
    uint32_t first_track = fread8u(input_image);
    uint32_t last_track  = fread8u(input_image);

    // Skip ISRC Code
    fseek(input_image, 8, SEEK_CUR);

    uint32_t sector_size = fread32u(input_image);
    uint32_t mode        = fread32u(input_image);
    uint64_t index0      = fread64u(input_image);
    uint64_t index1      = fread64u(input_image);
    uint64_t index2      = fread64u(input_image);

    // Skip unknown bit
    fseek(input_image, 2, SEEK_CUR);

    printf("DAOX: Size - %X, Toc Type - %X, First Track - %X, Last Track - %X, Sector Size - %X\n", chunk_size, toc_type, first_track, last_track, sector_size);
    printf("      Mode - %X, Index0 (Pre gap) - %X, Index1 (Start of track) - %X, Index2 (End of track + 1) - %X\n", mode, index0, index1, index2);
  }
  else if (chunk_id == CDTX) {
  }
  else if (chunk_id == ETNF) {
  }
  else if (chunk_id == ETN2) {
  }
  else if (chunk_id == SINF) {
  }
  else if (chunk_id == MTYP) {
  }
  else if (chunk_id == END) {
    printf("End of image data.\n");
  }
  else {
    fprintf(stderr, "Unrecognized Chunk ID: %X. Aborting.\n", chunk_id);
    exit(EXIT_FAILURE);
  }
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

  // Seek to the location of that first chunk
  fseek(input_image, first_chunk_offset, SEEK_SET);
  while (1)
    process_next_chunk(input_image);

  fclose(input_image);
}

