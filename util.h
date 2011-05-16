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
#include <stdarg.h> // for printf wrapper


/**
 * Increments verbosity one tick
 * @author Joe Balough
 */
void inc_verbosity();

/**
 * Decrements verbosity one tick
 * @author Joe Balough
 */
void dec_verbosity();


/**
 * printf() wrapper
 *
 * Only prints when verbose enabled.
 * @param int verbosity
 *   Only prints the message if global verbosity is >= passed verbosity
 * @param char *format, ...
 *   What you'd normally pass off to printf
 * @return int
 *   Total number of characters written (0 if verbosity not met)
 * @author Joe Balough
 */
int ver_printf(int verbosity, char *format, ...);


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
