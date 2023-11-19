/**
 * bits.c
 * Some helper macros and functions to shuffle around in bit land.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "bits.h"

#include <string.h>

/**
 * Duplicates a memory block. Kind of like strdup but for random bits of memory.
 *
 * @param p   Pointer to the start of the memory block to be duplicated.
 * @param len Length in bytes of the amount of memory to be duplicated.
 *
 * @return Duplicated piece of memory or NULL if an error occurred.
 */
void *memdup(const void *p, size_t len) {
	void *np;

	/* Allocate the new memory block. */
	np = malloc(len);
	if (np == NULL)
		return NULL;

	/* Copy the memory block over. */
	memcpy(np, p, len);

	return np;
}
