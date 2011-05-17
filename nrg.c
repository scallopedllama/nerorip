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

#include "nrg.h"

// Allocates memory for an nrg_image
nrg_image *alloc_nrg_image() {
  // Do the malloc
  nrg_image *r = malloc(sizeof(nrg_image));

  // Make sure it didn't fail
  if (!r) {
    fprintf(stderr, "Failed to allocate memory for nrg_image structure: %s\n", strerror(errno));
    return NULL;
  }

  //Initialize some variables in there and return the pointer
  r->nrg_version = UNPROCESSED;
  r->first_chunk_offset = 0x00;
  r->first_session = NULL;
  r->last_session = NULL;
  r->number_sessions = 0;

  return r;
}


// Frees memory used by an nrg_image
void free_nrg_image(nrg_image *image) {
  // Make sure image was allocated in the first place
  if (!image)
    return;

  // Loop through all the sessions
  nrg_session *s;
  for (s = image->first_session; s != NULL; ) {

    // Loop through all the tracks in this session
    nrg_track *t;
    for (t = s->first_track; t != NULL; ) {

      // Get the next track before freeing this one
      nrg_track *next_track = t->next;

      // Free this track
      free(t);

      // reset t
      t = next_track;
    }

    // Now that all the tracks have been freed, free this session
    nrg_session *next_session = s->next;
    free(s);
    s = next_session;
  }

  // It's now safe to free the image
  free(image);
  image = NULL;
}


// Allocates memory for an nrg_session
nrg_session *alloc_nrg_session() {
  // Do the malloc
  nrg_session *r = malloc(sizeof(nrg_session));

  // Make sure it didn't fail
  if (!r) {
    fprintf(stderr, "Failed to allocate memory for nrg_session structure: %s\n", strerror(errno));
    return NULL;
  }

  //Initialize some variables in there and return the pointer
  r->next = NULL;
  r->first_track = NULL;
  r->last_track = NULL;
  r->number_tracks = 0;

  return r;
}


// Allocates memory for an nrg_track
nrg_track *alloc_nrg_track() {
  // Do the malloc
  nrg_track *r = malloc(sizeof(nrg_track));

  // Make sure it didn't fail
  if (!r) {
    fprintf(stderr, "Failed to allocate memory for nrg_track structure: %s\n", strerror(errno));
    return NULL;
  }

  //Initialize some variables in there and return the pointer
  r->next = NULL;

  return r;
}


// Adds an nrg_session to an nrg_image
void add_nrg_session(nrg_image *image, nrg_session *session) {
  if (!image || !session) {
    fprintf(stderr, "add_nrg_session got an unallocated nrg_image or nrg_session.\n");
    return;
  }

  // If this is the first session going on the image, this session is both the first and last
  if (image->first_session == NULL) {
    image->first_session = session;
    image->last_session = session;
  }
  // Otherwise, it goes on the end and is the new last_session
  else {
    image->last_session->next = session;
    image->last_session = session;
  }
}


// Adds an nrg_track to an nrg_session
void add_nrg_track(nrg_session *session, nrg_track *track) {
  if (!track || !session) {
    fprintf(stderr, "add_nrg_track got an unallocated nrg_session or nrg_track.\n");
    return;
  }

  // If this is the first track going on the session, this track is both the first and last
  if (session->first_track == NULL) {
    session->first_track = track;
    session->last_track = track;
  }
  // Otherwise, it goes on the end and is the new last_track
  else {
    session->last_track->next = track;
    session->last_track = track;
  }
}


// Parses the chunk data in the image file to fill in nrg_image data structure
int nrg_parse(FILE *image_file, nrg_image *image) {
  // Make sure properly allocated
  if (!image_file || !image)
    return NON_ALLOC;

  ver_printf(3, "Detecting NRG file version:\n");

  // Seek to 12 bytes from the end and try to read the footer.
  fseek(image_file, -12, SEEK_END);

  // If the value there is NER5, it's version 5.5
  if (fread32u(image_file) == NER5) {
    image->first_chunk_offset = fread64u(image_file);
    image->nrg_version = NRG_VER_55;
    ver_printf(3, "  File appears to be a Nero 5.5 image\n");
  }
  // Otherwise, try to read the next chunk, if it's NERO, then its version 5
  else if (fread32u(image_file) == NERO) {
    image->first_chunk_offset = (uint64_t) fread32u(image_file);
    image->nrg_version = NRG_VER_5;
    ver_printf(3, "  File appears to be a Nero 5 image\n");
  }
  // If it wasn't either of the above, it must not be a nero image.
  else {
    image->nrg_version = NOT_NRG;
    ver_printf(3, "  File does not appear to be a Nero 5.5 image\n");
  }

  ver_printf(3, "Seeking to first chunk offset\n");
  fseek(image_file, image->first_chunk_offset, SEEK_SET);

  ver_printf(3, "Processing Chunk data:\n");

  // Keep track of what should be returned
  int r = 0;

  // Don't let this loop forever: break if we reach the end of the file
  while (!feof(image_file)) {

    // Chunk data came from the source for libdiscmage available at http://sourceforge.net/projects/discmage/
    // The chunk ID and chunk size are always 32 bit integers
    const long int chunk_offset = ftell(image_file);
    const uint32_t chunk_id = fread32u(image_file);
    const uint32_t chunk_size = fread32u(image_file);

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
      * About the LBA: The first LBA is the starting LBA for this session. If it's the first session, it's usually 0xffffff6a.
      *                The middle part repeats once for each track. The first LBA is the pre-start LBA for the track and the
      *                second is indicates where the track actually begins.
      *                Unless the track is audio with an intro bit (where the player starts at a negative time), the second LBA
      *                in each loop is 0x00000000
      */
      static int session_number = 1;
      static int track_number = 1;

      // CUES / CUEX indicates the start of a new DAO session so create a new session and add it to the image
      nrg_session *new_session = alloc_nrg_session();
      add_nrg_session(image, new_session);

      // Set burn mode and session mode and number of tracks
      new_session->number_tracks = chunk_size / 16 - 1;
      new_session->burn_mode = DAO;
      new_session->session_mode = fread8u(image_file);

      // Skip junk
      assert(fread8u(image_file) == 0x00); // Track
      assert(fread8u(image_file) == 0x00); // 0x00
      assert(fread8u(image_file) == 0x00); // Index

      new_session->start_lba = fread32u(image_file);
      ver_printf(3, "  %s at 0x%X:\n    Size - %d B, Session %d has %d track(s) using mode %s and starting at 0x%X.\n", (chunk_id == CUES ? "CUES" : "CUEX"), chunk_offset, chunk_size, session_number, new_session->number_tracks, (new_session->session_mode == 0x41 ? "Mode2" : (new_session->session_mode == 0x01 ? "Audio" : "Unknown")), new_session->start_lba);

      int i = 1;
      for (i = 1; i <= new_session->number_tracks; i++, track_number++) {
        // Each of these middle chunks holds a bit of data about one track for the session.
        // Allocate and add the new track
        nrg_track *new_track = alloc_nrg_track();
        add_nrg_track(new_session, new_track);

        new_track->pretrack_mode  = fread8u(image_file);
        assert(fread8u(image_file) == track_number);    // Track number
        assert(fread8u(image_file) == 0x00); // Track index
        assert(fread8u(image_file) == 0x00); // 0x00
        new_track->pretrack_lba = fread32u(image_file);

        new_track->track_mode = fread8u(image_file);
        assert(fread8u(image_file) == track_number);    // Track number
        assert(fread8u(image_file) == 0x01); // Track index
        assert(fread8u(image_file) == 0x00); // 0x00
        new_track->track_lba = fread32u(image_file);

        ver_printf(3, "      Track %d: Index 0 uses mode %s and starts at LBA 0x%X, ", i, (new_track->pretrack_mode == 0x41 ? "Mode2" : (new_track->pretrack_mode == 0x01 ? "Audio" : "Unknown")), new_track->pretrack_lba);
        ver_printf(3, "Index 1 uses mode %s and starts at LBA 0x%X.\n", (new_track->track_mode == 0x41 ? "Mode2" : (new_track->track_mode == 0x01 ? "Audio" : "Unknown")), new_track->track_lba);
      }

      // Skip junk
      // Nero 5.5 images always have the session mode here but 5.0 images generated by cdi2nero have a 0x00 here.
      if (image->nrg_version == NRG_VER_55)
        assert(fread8u(image_file) == new_session->session_mode);
      else
        fread8u(image_file);
      assert(fread8u(image_file) == 0xaa);         // 0xAA
      assert(fread8u(image_file) == 0x01);         // 0x01
      assert(fread8u(image_file) == 0x00);         // 0x00

      new_session->end_lba = fread32u(image_file);
      ver_printf(3, "    Session ends at LBA 0x%X\n", new_session->end_lba);

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
      int number_tracks = (fread32u(image_file) - 22) / 30;

      // Skip UPC
      fseek(image_file, 14, SEEK_CUR);

      uint8_t toc_type    = fread8u(image_file);
      fread8u(image_file); // close cd
      uint8_t first_track = fread8u(image_file);
      uint8_t last_track  = fread8u(image_file);

      ver_printf(3, "  DAOI at 0x%X:\n    Size - %dB, Toc Type - 0x%X, First Track - 0x%X, Last Track - 0x%X\n", chunk_offset, chunk_size, toc_type, first_track, last_track);
      ver_printf(3, "    Session has %d tracks:\n", number_tracks);


      int i = 1;
      for (i = 1; i <= number_tracks; i++)
      {
        // Skip ISRC Code
        fseek(image_file, 10, SEEK_CUR);

        uint32_t sector_size = fread32u(image_file);
        uint32_t mode        = fread32u(image_file);
        uint32_t index0      = fread32u(image_file);
        uint32_t index1      = fread32u(image_file);
        uint32_t next_offset = fread32u(image_file);
        ver_printf(3, "      Track %d: Sector Size - %d B, Mode - %s, index0 start - 0x%X, index1 start - 0x%X, Next offset - 0x%X\n", i, sector_size, (mode == 0x03000001 ? "Mode2" : (mode == 0x07000001 ? "Audio" : "Other")), index0, index1, next_offset);
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
      int number_tracks = (fread32u(image_file) - 22) / 30;

      // Skip UPC
      fseek(image_file, 14, SEEK_CUR);

      uint8_t toc_type    = fread8u(image_file);
      fread8u(image_file); // close cd
      uint8_t first_track = fread8u(image_file);
      uint8_t last_track  = fread8u(image_file);

      ver_printf(3, "  DAOX at 0x%X:\n    Size - %dB, Toc Type - 0x%X, First Track - 0x%X, Last Track - 0x%X\n", chunk_offset, chunk_size, toc_type, first_track, last_track);
      ver_printf(3, "    Session has %d track(s):\n", number_tracks);

      int i = 1;
      for (i = 1; i <= number_tracks; i++)
      {
        // Skip ISRC Code
        fseek(image_file, 10, SEEK_CUR);

        uint32_t sector_size = fread32u(image_file);
        uint32_t mode        = fread32u(image_file);
        uint64_t index0      = fread64u(image_file);
        uint64_t index1      = fread64u(image_file);
        uint64_t next_offset = fread64u(image_file);
        ver_printf(3, "      Track %d: Sector Size - %d B, Mode - %s, index0 start - 0x%X, index1 start - 0x%X, Next offset - 0x%X\n", i, sector_size, (mode == 0x03000001 ? "Mode2" : (mode == 0x07000001 ? "Audio" : "Other")), index0, index1, next_offset);
      }
    }
    else if (chunk_id == CDTX) {
      /**
      * CDTX (CD Text) format:
      *   18 B           CD-text pack
      */
      ver_printf(3, "  CDTX at 0x%X: Size - %dB\n", chunk_offset, chunk_size);
      fseek(image_file, chunk_size, SEEK_CUR);
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
      uint32_t track_offset = fread32u(image_file);
      uint32_t track_length = fread32u(image_file);
      uint32_t track_mode   = fread32u(image_file);
      uint32_t start_lba    = fread32u(image_file);
      assert(fread32u(image_file) == 0x00);

      ver_printf(3, "  ENTF at 0x%X:\n    Size - %d B, Track Offset - 0x%X, Track Length - %d B, Mode - %s, Start LBA - 0x%X\n", chunk_offset, chunk_size, track_offset, track_length, (track_mode == 0x03 ? "Mode2/2336" : (track_mode == 0x06 ? "Mode2/2352" : (track_mode == 0x07 ? "Audio/2352" : "Unknown"))), start_lba);
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
      uint64_t track_offset = fread64u(image_file);
      uint64_t track_length = fread64u(image_file);
      uint32_t track_mode   = fread32u(image_file);
      uint32_t start_lba    = fread32u(image_file);
      assert(fread64u(image_file) == 0x00);

      ver_printf(3, "  ENT2 at 0x%X:\n    Size - %d B, Track Offset - 0x%X, Track Length - %d B, Mode - %s, Start LBA - 0x%X\n", chunk_offset, chunk_size, track_offset, track_length, (track_mode == 0x03 ? "Mode2/2336" : (track_mode == 0x06 ? "Mode2/2352" : (track_mode == 0x07 ? "Audio/2352" : "Unknown"))), start_lba);
    }
    else if (chunk_id == SINF) {
      /**
      * SINF (Session Information) Format:
      *   4 B    Number tracks in session
      */
      uint32_t number_tracks = fread32u(image_file);

      ver_printf(3, "  SINF at 0x%X: Size - %dB, Number of Tracks: %d\n", chunk_offset, chunk_size, number_tracks);
    }
    else if (chunk_id == MTYP) {
      /**
      * MTYP (Media Type?) Format:
      *   4 B         unknown
      */
      uint32_t mystery_int  = fread32u(image_file);

      ver_printf(3, "  MTYP at 0x%X:  Size - %dB, ? - 0x%X\n", chunk_offset, chunk_size, mystery_int);
    }
    else if (chunk_id == END) {
      ver_printf(3, "  END! at 0x%X\n", chunk_offset);
      break;
    }
    else {
      fprintf(stderr, "  Unrecognized Chunk ID at ox%X: 0x%X.\n", chunk_offset, chunk_id);
      r = NRG_WARN;
    }
  }

  // If the eof was reached, there was a problem so tell the user.
  if (feof(image_file)) {
    ver_printf(1,   "WARNING: End of file reached. This should not have happened.\n");
    if (get_verbosity() < 3)
      ver_printf(1, "         Try running again with -vv to see chunk processing output to see what went wrong\n");
    else
      ver_printf(3, "         See output above to see what went wrong\n");
    ver_printf(1,   "         This was likely a bug in nerorip so please report to %s\n", WEBSITE);

    r = NRG_WARN;
  }

  ver_printf(3, "Done processing chunk data.\n");
  return r;
}
