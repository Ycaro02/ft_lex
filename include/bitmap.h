#ifndef BIMAP_IMPLEMENTATION_H
#define BIMAP_IMPLEMENTATION_H

#include "basic_define.h"

/* Bitmap Array Size in bytes */
#define BITMAP_STATE_ARRAY_SIZE 10ULL

/* Unsigned 64-bit integer type size in bits */
#define U64_BITS_NB (sizeof(u64) * 8ULL)  /* Number of bits in u64 */

#define BITMAP_SIZE(b_size) ((b_size) * U64_BITS_NB)   /* in bits */

/**
 * @brief Bitmap for efficient state set representation
 * 
 * Uses 8 64-bit unsigned integers to support up to 512 states efficiently.
 */
typedef struct Bitmap {
    u64 *bits;
    u32 size;  /* Size of the bitmap in bits (maximum number of states) */
} Bitmap;

void    bitmap_init(Bitmap *b, u32 size);
void    bitmap_clear(Bitmap *b);
void    bitmap_set(Bitmap *b, u32 id);
s8      bitmap_is_set(Bitmap *b, u32 id);
s8      bitmap_equal(Bitmap *a, Bitmap *b);
void    bitmap_copy(Bitmap *dest, Bitmap *src);

#endif /* BIMAP_IMPLEMENTATION_H */