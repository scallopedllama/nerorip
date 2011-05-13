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
  // Chunk data came from the source for libdiscmage at http://www.koders.com/c/fidCFCF40C7DBB1F855208886B92C3C77ED6DBE45F3.aspx
  // The chunk ID and chunk size are always 32 bit integers
  const long int start_offset = ftell(input_image);
  const uint32_t chunk_id = fread32u(input_image);
  const uint32_t chunk_size = fread32u(input_image);

  if (chunk_id == CUES || chunk_id == CUEX) {
    /**
     * CUE / CUEX (Cue Sheet) Format:
     *   Indicates a disc at once image
     *   Chunk size = (# tracks + 1) * 16
     *
     * 1 B     Mode                         (0x41 = mode2, 0x01 = audio)
     * 1 B     Track number                 (first is 0)
     * 1 B     Index                        (first is 0)
     * 1 B     00
     * 4 B     V1: MM:SS:FF=0; V2: LBA      (-150 (0xffffff6a) (MSF = 00:00:00))
     * ---
     * 1 B     Mode
     * 1 B     Track number
     * 1 B     Index                        (starts at 0)
     * 1 B     00
     * 4 B     V1: MMSSFF; V2: LBA          (MMSSFF = index) (LBA starts with index 0)
     * ... Repeat for each track in session
     * ---
     * 4 B    mm AA 01 00   mm = mode
     * 4 B    V1: Last MMSSFF; V2: Last LBA (MMSSFF == index1 + length)
     */
    // CUES and CUEX are both followed by a chunk size value
    printf("%s at 0x%X:\tSize - %dB\n", (chunk_id == CUES ? "CUES" : "CUEX"), start_offset, chunk_size);



    // Skip that data and the pointless additional 4 bytes
    fseek(input_image, chunk_size, SEEK_CUR);
  }
  else if (chunk_id == DAOI) {
    /**
     * DAOI (DAO Information) format:
     *   4  B   Chunk size (bytes) again        (= (# tracks * 30) + 22)
     *   14 B   UPC?
     *   1  B   Toc type                        (0x20 = Mode2, 0x00 = Audio?)
     *   1  B   close_cd?                       (1 = doesn't work, usually 0)
     *   1  B   First track                     (Usually 0x01)
     *   1  B   Last track                      (Usually = # tracks)
     * ---
     *   10 B   ISRC Code?
     *   4  B   Sector size
     *   4  B   Mode                            (mode2 = 0x03000001, audio = 0x07000001)
     *   4  B   Index0 start offset
     *   4  B   Index1 start offset             (= index0 + pregap length)
     *   4  B   Next offset                     (= index1 + track length
     * ... Repeat for each track in this session
     */
    // # tracks
    int number_tracks = (fread32u(input_image) - 22) / 30;

    // Skip UPC
    fseek(input_image, 14, SEEK_CUR);

    uint8_t toc_type    = fread8u(input_image);
    fread8u(input_image); // close cd
    uint8_t first_track = fread8u(input_image);
    uint8_t last_track  = fread8u(input_image);

    printf("DAOI at 0x%X:\tSize - %dB, Toc Type - 0x%X, First Track - 0x%X, Last Track - 0x%X\n", start_offset, chunk_size, toc_type, first_track, last_track);
    printf("\t\t\tSession has %d tracks:\n", number_tracks);


    int i = 0;
    for (i = 0; i < number_tracks; i++)
    {
      // Skip ISRC Code
      fseek(input_image, 10, SEEK_CUR);

      uint32_t sector_size = fread32u(input_image);
      uint32_t mode        = fread32u(input_image);
      uint32_t index0      = fread32u(input_image);
      uint32_t index1      = fread32u(input_image);
      uint32_t next_offset = fread32u(input_image);
      printf("\t\t\t\tTrack %d: Sector Size - %d B, Mode - %s, index0 start - 0x%X, index1 start - 0x%X, Next offset - 0x%X\n", i, sector_size, (mode == 0x03000001 ? "Mode2" : (mode == 0x07000001 ? "Audio" : "Other")), index0, index1, next_offset);
    }
  }
  else if (chunk_id == DAOX) {
    /**
     * DAOX (DAO Information) format:
     *   4  B    Chunk size (bytes) again    ( = (# tracks * 42) + 22 )
     *   14 B    UPC?
     *   1  B    Toc type                    (0x20 = Mode2, 0x00 = Audio?)
     *   1  B    Close CD?                   (1 = doesn't work)
     *   1  B    First track                 (Usually 0x01)
     *   1  B    Last track                  (Usually = tracks)
     * ---
     *   10 B    ISRC Code?
     *   4  B    Sector size
     *   4  B    Mode                        (mode2 = 0x03000001, audio = 0x07000001)
     *   8  B    Index0 start offset
     *   8  B    Index1                      (= index0 + pregap length)
     *   8  B    Next offset                 (= index1 + track length)
     *   ... Repeat for each track in session
     */
    // # tracks
    int number_tracks = (fread32u(input_image) - 22) / 30;

    // Skip UPC
    fseek(input_image, 14, SEEK_CUR);

    uint8_t toc_type    = fread8u(input_image);
    fread8u(input_image); // close cd
    uint8_t first_track = fread8u(input_image);
    uint8_t last_track  = fread8u(input_image);

    printf("DAOX at 0x%X:\tSize - %dB, Toc Type - 0x%X, First Track - 0x%X, Last Track - 0x%X\n", start_offset, chunk_size, toc_type, first_track, last_track);
    printf("\t\t\tSession has %d tracks:\n", number_tracks);

    int i = 0;
    for (i = 0; i < number_tracks; i++)
    {
      // Skip ISRC Code
      fseek(input_image, 10, SEEK_CUR);

      uint32_t sector_size = fread32u(input_image);
      uint32_t mode        = fread32u(input_image);
      uint64_t index0      = fread64u(input_image);
      uint64_t index1      = fread64u(input_image);
      uint64_t next_offset = fread64u(input_image);
      printf("\t\t\t\tTrack %d: Sector Size - %d B, Mode - %s, index0 start - 0x%X, index1 start - 0x%X, Next offset - 0x%X\n", i, sector_size, (mode == 0x03000001 ? "Mode2" : (mode == 0x07000001 ? "Audio" : "Other")), index0, index1, next_offset);
    }
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
     * Multisession / TAO Only
     *   4 B    Start offset
     *   4 B    Length (bytes)
     *   4 B    Mode                    (011 (0x03) = mode2/2336, 110 (0x06) = mode2/2352, 111 (0x07) = audio/2352)
     *   4 B    Start lba
     *   4 B    00
     * ... Repeat for each track (in session)
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
     *   8 B    Start offset
     *   8 B    Length (bytes)
     *   4 B    Mode                    (011 (0x03) = mode2/2336, 110 (0x06) = mode2/2352, 111 (0x07) = audio/2352)
     *   4 B    Start lba
     *   8 B    00
     * ... Repeat for each track (in session)
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
     *   4 B    Number tracks in session
     */
    uint32_t number_tracks = fread32u(input_image);

    printf("SINF at 0x%X:\tSize - %dB, Number of Tracks: %d\n", start_offset, chunk_size, number_tracks);
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

