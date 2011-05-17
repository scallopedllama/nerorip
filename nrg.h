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
#ifndef NRG_H
#define NRG_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> // strerror()
#include <stdint.h> // uintXX_t types
#include <byteswap.h> // for bswap_XX functions
#include <assert.h>
#include "util.h"

// Nero Image defines
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
#define MODE2 0x41
#define AUDIO 0x01

// Defines nero image versions
#define NRG_VER_55 2
#define NRG_VER_5 1
// Defines burn modes
#define DAO 0
#define TAO 1
// Indicates that the nrg_image struct hasn't been processed yet
#define UNPROCESSED 0
// Indicates that the image file does not appear to be a nero image
#define NOT_NRG -1
// Indicates that the passed structure was not properly allocated first
#define NON_ALLOC -2
// Indicates that something unexpected happened while parsing the file.
#define NRG_WARN -3

/*
 * DATA STRUCTURES
 */

/**
 * Nero image track struct
 *
 * Manages all data relevant to a track in the nero image.
 * Keeps a pointer to the next track making this a linked list.
 *
 * @author Joe Balough
 */
typedef struct {
  // Pointer to the next track
  struct nrg_track *next;

  /*
   * Track data
   */
  uint8_t pretrack_mode;
  uint32_t pretrack_lba;
  uint8_t track_mode;
  uint32_t track_lba;

} nrg_track;


/**
 * Nero image session struct
 *
 * Manages all data relevant to a session in the nero image.
 * Keeps a pointer to the next session making this a linked list.
 *
 * @author Joe Balough
 */
typedef struct {
  // Pointer to the next session
  struct nrg_session *next;

  // Pointer to the front of the list of tracks in this session and the number of them
  nrg_track *first_track, *last_track;
  int number_tracks;

  /*
   * Session Data
   */
  // The Mode to be used for burning this session. Either DAO or TAO
  uint8_t burn_mode;
  // The Mode defined for this session. Either MODE2 or AUDIO
  uint8_t session_mode;
  // The lba at which the session starts and ends
  uint32_t start_lba, end_lba;

} nrg_session;


/**
 * Nero image track struct
 *
 * Manages all data relevant to the nero image.
 *
 * @author Joe Balough
 */
typedef struct {
  // Pointer to the list of sessions and number of sessions
  nrg_session *first_session, *last_session;
  int number_sessions;

  /*
   * Image data
   */

  // Where the first bit of chunk data lies
  uint64_t first_chunk_offset;
  // Version of this image file. Should be NRG_VER_5 or NRG_VER_55
  int nrg_version;


} nrg_image;



/*
 * FUNCTIONS
 */


/**
 * Allocate memory for an nrg_image struct and return a pointer to it.
 * @return *nrg_image
 *   A pointer to the properly allocated nrg_image struct
 * @author Joe Balough
 */
nrg_image *alloc_nrg_image();


/**
 * Free memory used by an nrg_image struct properly.
 * Will also free memory for all nrg_session and nrg_track data.
 * If image wasn't allocated, it will simply return without doing anything.
 *
 * @param *nrg_image image
 *   nrg_image structure to free
 * @author Joe Balough
 */
void free_nrg_image(nrg_image *image);


/**
 * Allocate memory for an nrg_session struct and return a pointer to it.
 * @return *nrg_session
 *   A pointer to the properly allocated nrg_session struct
 * @author Joe Balough
 */
nrg_session *alloc_nrg_session();


/**
 * Allocate memory for an nrg_track struct and return a pointer to it.
 * @return *nrg_track
 *   A pointer to the properly allocated nrg_track struct
 * @author Joe Balough
 */
nrg_track *alloc_nrg_track();

/**
 * Adds an nrg_track to the list in the passed nrg_session.
 * The track should be allocated before calling.
 * If session or track is NULL, the function will simply return having done nothing.
 *
 * @param nrg_session *session
 *   The nrg_session to which the track should be added
 * @param nrg_track *track
 *   The nrg_track to add
 * @author Joe Balough
 */
void add_nrg_track(nrg_session *session, nrg_track *track);


/**
 * Adds an nrg_session to the list in the passed nrg_image.
 * The session should be allocated before calling.
 * If image or session is NULL, the function will simply return having done nothing.
 *
 * @param nrg_image *image
 *   The nrg_image to which the session should be added
 * @param nrg_session *session
 *   The nrg_session to add
 * @author Joe Balough
 */
void add_nrg_session(nrg_image *image, nrg_session *session);


/**
 * Parse the nero image chunk data from the image_file
 * and fill in the image data structure.
 * When this function returns, image will completely describe the image file.
 *
 * @param FILE* image_file
 *   File pointer to already opened nero image file
 * @param nrg_image* image
 *   The already allocated nrg_image struct to fill
 * @return
 *   0 on success
 *   NRG_WARN if unrecognized chunks were encountered
 *   NON_ALLOC if nrg_image or image_file not allocated
 * @author Joe Balough
 */
int nrg_parse(FILE *image_file, nrg_image *image);


#endif
