#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> // strerror()
#include <stdint.h> // uintXX_t types
#include <byteswap.h> // for bswap_XX functions

/**
 * File reading convenience functions
 *
 * These functions read an unsigned integer of the indicated size from the
 * passed file and handles read errors and the like.
 * Before returning, convert the number from little endian to big endian.
 */
uint8_t fread8u(FILE*);
uint16_t fread16u(FILE*);
uint32_t fread32u(FILE*);
uint64_t fread64u(FILE*);
