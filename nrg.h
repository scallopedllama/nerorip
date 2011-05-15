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
