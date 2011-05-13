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
  // The chunk ID and chunk size are always 32 bit integers
  const long int start_offset = ftell(input_image);
  const uint32_t chunk_id = fread32u(input_image);
  const uint32_t chunk_size = fread32u(input_image);

  if (chunk_id == CUES || chunk_id == CUEX) {
    /**
     * CUES (Cue Sheet) Format:
     *   Undocumented
     */
    // CUES and CUEX are both followed by a chunk size value
    printf("%s at 0x%X:\tSize - %dB\n", (chunk_id == CUES ? "CUES" : "CUEX"), start_offset, chunk_size);

    // Skip that data and the pointless additional 4 bytes
    fseek(input_image, chunk_size, SEEK_CUR);
  }
  else if (chunk_id == DAOI) {
    /**
     * DAOI (DAO Information) format:
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
    fread32u(input_image);

    // Skip UPC
    fseek(input_image, 14, SEEK_CUR);

    uint32_t toc_type    = fread32u(input_image);
    uint8_t first_track = fread8u(input_image);
    uint8_t last_track  = fread8u(input_image);

    // Skip ISRC Code
    fseek(input_image, 12, SEEK_CUR);

    uint32_t sector_size = fread32u(input_image);
    uint32_t mode        = fread32u(input_image);
    uint32_t index0      = fread32u(input_image);
    uint32_t index1      = fread32u(input_image);
    uint32_t index2      = fread32u(input_image);

    printf("DAOI at 0x%X:\tSize - %dB, Toc Type - 0x%X, First Track - 0x%X, Last Track - 0x%X, Sector Size - %dB\n", start_offset, chunk_size, toc_type, first_track, last_track, sector_size);
    printf("\t\t\tMode - 0x%X, Index0 (Pre gap) - 0x%X, Index1 (Start of track) - 0x%X, Index2 (End of track + 1) - 0x%X\n", mode, index0, index1, index2);
  }
  else if (chunk_id == DAOX) {
    /**
     * DAOI (DAO Information) format:
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
    fread32u(input_image);

    // Skip UPC
    fseek(input_image, 14, SEEK_CUR);

    uint32_t toc_type    = fread32u(input_image);
    uint8_t first_track = fread8u(input_image);
    uint8_t last_track  = fread8u(input_image);

    // Skip ISRC Code
    fseek(input_image, 8, SEEK_CUR);

    uint32_t sector_size = fread32u(input_image);
    uint32_t mode        = fread32u(input_image);
    uint64_t index0      = fread64u(input_image);
    uint64_t index1      = fread64u(input_image);
    uint64_t index2      = fread64u(input_image);

    printf("DAOX at 0x%X:\tSize - %dB, Toc Type - 0x%X, First Track - 0x%X, Last Track - 0x%X, Sector Size - %dB\n", start_offset, chunk_size, toc_type, first_track, last_track, sector_size);
    printf("\t\t\tMode - 0x%X, Index0 (Pre gap) - 0x%X, Index1 (Start of track) - 0x%X, Index2 (End of track + 1) - 0x%X\n", mode, index0, index1, index2);
  }
  else if (chunk_id == CDTX) {
    /**
     * CDTX (CD Text) format:
     *   18 B           CD-text pack
     */
    printf("CDTX at 0x%X:\tSize - %dB\n", start_offset, chunk_size);
    fseek(input_image, chunk_size, SEEK_CUR);
  }
  else if (chunk_id == ETNF) {
    /**
     * ETNF (Extended Track Information) format:
     *   4 B  32bit   Track offset in image
     *   4 B  32bit   Track length (bytes)
     *   4 B  32bit   Mode
     *   4 B  32bit   Start lba on disc
     *   4 B       ?
     */
    uint32_t track_offset = fread32u(input_image);
    uint32_t track_length = fread32u(input_image);
    uint32_t track_mode   = fread32u(input_image);
    uint32_t start_lba    = fread32u(input_image);
    uint32_t mystery_int  = fread32u(input_image);

    printf("ENTF at 0x%X:\tSize - %dB, Track Offset - 0x%X, Track Length - 0x%X, Mode - 0x%X, Start LBA - 0x%X, ? - 0x%X\n", start_offset, chunk_size, track_offset, track_length, track_mode, start_lba, mystery_int);
  }
  else if (chunk_id == ETN2) {
    /**
     * ETN2 (Extended Track Information) format:
     *   8 B  64bit   Track offset in image
     *   8 B  64bit   Track length (bytes)
     *   4 B  32bit   Mode
     *   4 B  32bit   Start lba on disc
     *   4 B       ?
     */
    uint64_t track_offset = fread64u(input_image);
    uint64_t track_length = fread64u(input_image);
    uint32_t track_mode   = fread32u(input_image);
    uint32_t start_lba    = fread32u(input_image);
    uint32_t mystery_int  = fread32u(input_image);

    printf("ENT2 at 0x%X:\tSize - %dB, Track Offset - 0x%X, Track Length - 0x%X, Mode - 0x%X, Start LBA - 0x%X, ? - 0x%X\n", start_offset, chunk_size, track_offset, track_length, track_mode, start_lba, mystery_int);
  }
  else if (chunk_id == SINF) {
    /**
     * SINF (Session Information) Format:
     *   4 B  32bit   # tracks in session
     */
    uint32_t number_tracks = fread32u(input_image);

    printf("SINF at 0x%X:\tSize - %dB, Number of Tracks: %d\n", start_offset, number_tracks);
  }
  else if (chunk_id == MTYP) {
    /**
     * MTYP (Media Type?) Format:
     *   4 B         unknown
     */
    uint32_t mystery_int  = fread32u(input_image);

    printf("MTYP at 0x%X:\tSize - %dB, ? - 0x%X\n", start_offset, chunk_size, mystery_int);
  }
  else if (chunk_id == END) {
    printf("END! at 0x%X\n", start_offset);
    exit(EXIT_SUCCESS);
  }
  else {
    fprintf(stderr, "Unrecognized Chunk ID: 0x%X. Aborting.\n", start_offset, chunk_id);
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

  printf("Image is a version %d Nero image with first chunk offset of 0x%X\n", image_ver, first_chunk_offset);

  // Seek to the location of that first chunk
  fseek(input_image, first_chunk_offset, SEEK_SET);
  while (1)
    process_next_chunk(input_image);

  fclose(input_image);
}

