#include "../../include/bitmap.h"
#include "../../include/log.h"

/**
 * @brief Clear all bits in the bitmap
 * @param b Bitmap to clear
 */
void bitmap_clear(Bitmap *b) {
    memset(b->bits, 0, sizeof(b->bits));
}

/**
 * @brief Set a bit in the bitmap (add state to set)
 * @param b Bitmap to modify
 * @param id State ID to add
 */
void bitmap_set(Bitmap *b, u32 id) {
    if (id >= 0 && id < BITMAP_SIZE) {
        b->bits[id / U64_BITS_NB] |= (1ULL << (id % U64_BITS_NB));
    } else {
        ERR("State ID %d out of range for bitmap\n", id);
    }
}

/**
 * @brief Check if a bit is set in the bitmap (state in set)
 * @param b Bitmap to check
 * @param id State ID to check
 * @return 1 if state is in set, 0 otherwise
 */
s8 bitmap_is_set(Bitmap *b, u32 id) {
    if (id >= 0 && id < BITMAP_SIZE) {
        return (b->bits[id / U64_BITS_NB] & (1ULL << (id % U64_BITS_NB))) != 0;
    }
    return 0;
}