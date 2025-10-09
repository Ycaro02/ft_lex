#ifndef BIMAP_IMPLEMENTATION_H
#define BIMAP_IMPLEMENTATION_H

#include "basic_define.h"

/* Bitmap Array Size in bytes */
#define BITMAP_ARRAY_SIZE 8ULL

/* Unsigned 64-bit integer type size in bits */
#define U64_BITS_NB (sizeof(u64) * 8ULL)  /* Number of bits in u64 */

/* Total Bitmap Size in bits */
#define BITMAP_SIZE (BITMAP_ARRAY_SIZE * U64_BITS_NB)  /* in bits */

/**
 * @brief Bitmap for efficient state set representation
 * 
 * Uses 8 64-bit unsigned integers to support up to 512 states efficiently.
 */
typedef struct Bitmap {
    u64 bits[BITMAP_ARRAY_SIZE];
} Bitmap;

void    bitmap_clear(Bitmap *b);
void    bitmap_set(Bitmap *b, u32 id);
s8      bitmap_is_set(Bitmap *b, u32 id);

#endif /* BIMAP_IMPLEMENTATION_H */