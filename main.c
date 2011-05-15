#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> // strerror()
#include <stdint.h> // uintXX_t types
#include <byteswap.h> // for bswap_XX functions
#include <assert.h>

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

#define VERSION "0.1"
#define NRG_VER_5 1
#define NRG_VER_55 2


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
     * CUES / CUEX (Cue Sheet) Format:
     *   Indicates a disc at once image
     *   Chunk size = (# tracks + 1) * 16
     *
     * 1 B     Mode                         (0x41 = mode2, 0x01 = audio)
     * 1 B     Track number                 0x00
     * 1 B     Index                        0x00
     * 1 B     00
     * 4 B     V1: MM:SS:FF=0; V2: StartLBA ((LBA for S1T1 is 0xffffff6a) (MSF = 00:00:00))
     * ---
     * 1 B     Mode
     * 1 B     Track number                 (First is 1, increments over tracks in ALL sessions)
     * 1 B     Index                        (0x00, which is pregap for track)
     * 1 B     00
     * 4 B     V1: MMSSFF; V2: LBA          (MMSSFF = index) (S1T1's LBA is 0xffffff6a)
     * 1 B     Mode
     * 1 B     Track number                 (First is 1, increments over tracks in ALL sessions)
     * 1 B     Index                        (0x01, which is main track)
     * 1 B     00
     * 4 B     V1: MMSSFF; V2: LBA          (MMSSFF = index) (LBA is where track actually starts)
     * ... Repeat for each track in session
     * ---
     * 4 B    mm AA 01 00   mm = mode (if version 2)
     * 4 B    V1: Last MMSSFF; V2: Last LBA (MMSSFF == index1 + length)
     *
     * About the LBA: The first LBA is the starting LBA for this session. If it's the first session, it's always 0xffffff6a.
     *                The middle part repeats once for each session. The first LBA is the pre-start LBA for the track and the
     *                second is indicates where the track actually begins.
     *                Unless the track is audio with an intro bit (where the player starts at a negative time), the second LBA
     *                in each loop is 0x00000000
     */
    int number_tracks = chunk_size / 16 - 1;
    static int session_number = 1;
    static int track_number = 1;

    uint8_t session_mode = fread8u(input_image);

    // Skip junk
    assert(fread8u(input_image) == 0x00); // Track
    assert(fread8u(input_image) == 0x00); // 0x00
    assert(fread8u(input_image) == 0x00); // Index

    uint32_t session_start_LBA = fread32u(input_image);
    printf("%s at 0x%X:\tSize - %d B, Session %d has %d tracks using mode %s and starting at 0x%X.\n", (chunk_id == CUES ? "CUES" : "CUEX"), start_offset, chunk_size, session_number, number_tracks, (session_mode == 0x41 ? "Mode2" : (session_mode == 0x01 ? "Audio" : "Unknown")), session_start_LBA);

    int i = 1;
    for (i = 1; i <= number_tracks; i++, track_number++) {
      uint8_t pretrack_mode  = fread8u(input_image);
      assert(fread8u(input_image) == track_number);    // Track number
      assert(fread8u(input_image) == 0x00); // Track index
      assert(fread8u(input_image) == 0x00); // 0x00
      uint32_t pretrack_LBA = fread32u(input_image);

      uint8_t track_mode = fread8u(input_image);
      assert(fread8u(input_image) == track_number);    // Track number
      assert(fread8u(input_image) == 0x01); // Track index
      assert(fread8u(input_image) == 0x00); // 0x00
      uint32_t track_LBA = fread32u(input_image);

      printf ("\t\t\t\tTrack %d: Index 0 uses mode %s and starts at LBA 0x%X, ", i, (pretrack_mode == 0x41 ? "Mode2" : (pretrack_mode == 0x01 ? "Audio" : "Unknown")), pretrack_LBA);
      printf("Index 1 uses mode %s and starts at LBA 0x%X.\n", (track_mode == 0x41 ? "Mode2" : (track_mode == 0x01 ? "Audio" : "Unknown")), track_LBA);
    }

    // Skip junk
    // TODO: track the version number and re-enable this assertion, but only for NRG_VER_55
    //       assert(fread8u(input_image) == session_mode); // Mode
    fread8u(input_image);
    assert(fread8u(input_image) == 0xaa);         // 0xAA
    assert(fread8u(input_image) == 0x01);         // 0x01
    assert(fread8u(input_image) == 0x00);         // 0x00

    uint32_t session_end_LBA = fread32u(input_image);
    printf("\t\t\tSession ends at LBA 0x%X\n", session_end_LBA);

    session_number++;
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


    int i = 1;
    for (i = 1; i <= number_tracks; i++)
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
     *   
     *  It looks like either Index0 or Index1 is the file location where the actual image data lies. DC images' audio data is just straight 00's.
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

    int i = 1;
    for (i = 1; i <= number_tracks; i++)
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
    assert(fread32u(input_image) == 0x00);

    printf("ENTF at 0x%X:\tSize - %d B, Track Offset - 0x%X, Track Length - %d B, Mode - %s, Start LBA - 0x%X\n", start_offset, chunk_size, track_offset, track_length, (track_mode == 0x03 ? "Mode2/2336" : (track_mode == 0x06 ? "Mode2/2352" : (track_mode == 0x07 ? "Audio/2352" : "Unknown"))), start_lba);
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
    assert(fread64u(input_image) == 0x00);

    printf("ENT2 at 0x%X:\tSize - %d B, Track Offset - 0x%X, Track Length - %d B, Mode - %s, Start LBA - 0x%X\n", start_offset, chunk_size, track_offset, track_length, (track_mode == 0x03 ? "Mode2/2336" : (track_mode == 0x06 ? "Mode2/2352" : (track_mode == 0x07 ? "Audio/2352" : "Unknown"))), start_lba);
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
  // TODO: Actually parse arguments.
  if (argc <= 1) {
    fprintf(stderr, "Usage: %s [OPTIONS]... [INPUT FILE] [OUTPUT DIRECTORY]\n", argv[0]);
    fprintf(stderr, "Nero Image Ripper takes a nero image file (.nrt extension) as input\n");
    fprintf(stderr, "and attempts to extract the track data as either ISO or audio data.\n\n");
    fprintf(stderr, "  -i, --info\t\tOnly disply information about the image file, do not rip\n");
    fprintf(stderr, "  -h, --help\t\tDisplay this help message and exit\n");
    fprintf(stderr, "  -v, --version\t\tOutput version information and exit.\n\n");
    fprintf(stderr, "If output directory is omitted, image data is put in the same directory as the input file.\n\n");
    
    fprintf(stderr, "For each track found in the image, Nero Image Ripper will output the following:\n");
    fprintf(stderr, "  one iso file named \"data.sSStTT.iso\" if the track is data and\n");
    fprintf(stderr, "  one wav file named \"audio.sSStTT.wav\" if the track is audio\n");
    fprintf(stderr, "where SS is the session number and TT is the track number.\n\n");
    
    fprintf(stderr, "For example, if your disc image is like the following\n");
    fprintf(stderr, "  Session 1:\n    Track 1: Audio\n    Track 2: Data\n  Session 2:\n    Track 1: Data\n");
    fprintf(stderr, "the Nero Image Ripper will output the following files:\n");
    fprintf(stderr, "  audio.s01t01.wav, data.s01t02.iso, data.s02t03.iso\n");
    fprintf(stderr, "Note that the track number does not reset between sessions.\n\n");
    
    fprintf(stderr, "Report all bugs at https://github.com/scallopedllama/nerorip\nVersion %s\n", VERSION);
    exit(EXIT_FAILURE);
  }
  
  // Open file
  char *input_str = argv[1];
  FILE *input_image = fopen(input_str, "rb");
  if (input_image == NULL) {
    fprintf(stderr, "Error opening %s: %s\n", input_str, strerror(errno));
    exit(EXIT_FAILURE);
  }

  int image_ver;
  // The header is actually stored in the last 8 or 12 bytes of the file, depending on the version.
  // Seek to 12 bytes from the end and see if it's version 2. If so, get the header.
  uint64_t first_chunk_offset;
  fseek(input_image, -12, SEEK_END);

  if (fread32u(input_image) == NER5) {
    first_chunk_offset = fread64u(input_image);
    image_ver = NRG_VER_55;
  }
  else if (fread32u(input_image) == NERO) {
    first_chunk_offset = (uint64_t) fread32u(input_image);
    image_ver = NRG_VER_5;
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

