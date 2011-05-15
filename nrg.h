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

#define NRG_VER_5 1
#define NRG_VER_55 2

/**
 * Process the next chunk of data starting from the file's current position.
 */
void process_next_chunk(FILE*);



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
struct nrg_track {
  // Pointer to the next track
  struct nrg_track *next;
};

/**
 * Nero image session struct
 *
 * Manages all data relevant to a session in the nero image.
 * Keeps a pointer to the next session making this a linked list.
 *
 * @author Joe Balough
 */
struct nrg_session {
  // Pointer to the next session
  struct nrg_session *next;

  // Pointer to the front of the list of tracks in this session and the number of them
  struct nrg_track *tracks;
  int number_tracks;
};

/**
 * Nero image track struct
 *
 * Manages all data relevant to the nero image.
 *
 * @author Joe Balough
 */
struct nrg_image {
  // Version of this image file. Should be NRG_VER_5 or NRG_VER_55
  int nrg_version;

  // Pointer to the list of sessions and number of sessions
  struct nrg_session *sessions;
  int number_sessions;
};
