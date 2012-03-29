#include "bitfield.h"

#include <stdlib.h>
#include <errno.h>

//--------------------------------------------------------------------------
// Allocation
//--------------------------------------------------------------------------

// alloc

bitfield_t * bitfield_create(size_t size_in_bits) {
    bitfield_t * bitfield = calloc(1, sizeof(bitfield_t));
    if (!bitfield) goto FAILURE;

    bitfield->mask = malloc(size_in_bits / 8);
    if (!bitfield->mask) goto FAILURE;

    bitfield->size_in_bits = size_in_bits;
    return bitfield;    
FAILURE:
    bitfield_free(bitfield);
    return NULL;
}

// free

void bitfield_free(bitfield_t * bitfield) {
    if (bitfield){
        if (bitfield->mask) free(bitfield->mask);
        free(bitfield);
    }
}

//--------------------------------------------------------------------------
// Metafield's setter/getter 
//--------------------------------------------------------------------------

inline void bitfield_set_0(unsigned char *mask, size_t offset_in_bits) {
    mask[offset_in_bits / 8] |= 1 << (offset_in_bits % 8);
}

// internal usage

inline void bitfield_set_1(unsigned char *mask, size_t offset_in_bits) {
    mask[offset_in_bits / 8] &= ~(1 << (offset_in_bits % 8));
}

// bitfield initialization (per bit)

int bitfield_set_mask_bit(
    bitfield_t * bitfield,
    int          value,
    size_t       offset_in_bits
) {
    if (!bitfield) return EINVAL;
    if (offset_in_bits >= bitfield->size_in_bits) return EINVAL;

    if (value) bitfield_set_0(bitfield->mask, offset_in_bits);
    else       bitfield_set_1(bitfield->mask, offset_in_bits);

    return 0;
}

// bitfield initialization (per block of bits)

int bitfield_set_mask_bits(
    bitfield_t * bitfield,
    int          value,
    size_t       offset_in_bits,
    size_t       num_bits
) {
    size_t offset;
    size_t offset_end = offset_in_bits + num_bits;

    if (!bitfield) return EINVAL;
    if (offset_in_bits + num_bits >= bitfield->size_in_bits) return EINVAL;

    if (num_bits) {
        // to improve to set byte per byte
        for (offset = offset_in_bits; offset < offset_end; offset++) {
            if (value) bitfield_set_0(bitfield->mask, offset);
            else       bitfield_set_1(bitfield->mask, offset);
        }
    }
    return 0;
}

size_t bitfield_get_num_1(const bitfield_t * bitfield) {
    typedef unsigned char cell_t;
    size_t i, j, size;
    size_t res = 0;
    size_t size_cell_in_bits = sizeof(cell_t) * 8;

    if (!bitfield) return 0; // invalid parameter 

    size = bitfield->size_in_bits / 8;
    for (i = 0; i < size; i++) {
        cell_t cur_byte = bitfield->mask[i];
        for (j = 0; j < size_cell_in_bits; j++) {
            if (i * size_cell_in_bits + j == bitfield->size_in_bits) break;
            if (cur_byte & (1 << j)) res++;
        } 
    }
    return res;
}

//--------------------------------------------------------------------------
// Operators 
//--------------------------------------------------------------------------

static inline size_t min(size_t x, size_t y) {
    return x < y ? x : y;
}

/**
 * \brief Apply &= to each byte (tgt &= src)
 * \param tgt The left operand of &= 
 * \param src The right operand of &=
 */

void bitfield_and(bitfield_t * tgt, const bitfield_t * src) {
    size_t i, j, size_in_bits, size;
    if (!tgt || !src) return;
    size_in_bits = min(tgt->size_in_bits, src->size_in_bits);
    size = size_in_bits / 8;
    for (i = 0; i < size; ++i) {
        if (i + 1 == size) {
            for (j = 0; j < size_in_bits % 8; ++j) {
                if (!(src->mask[i] & (1 << j))) {
                    bitfield_set_0(tgt->mask, 8 * i + j);
                }
            }
        } else {
            tgt->mask[i] &= src->mask[i]; 
        }
    }
}

/**
 * \brief Apply |= to each byte (tgt |= src)
 * \param tgt The left operand of |= 
 * \param src The right operand of |=
 */

void bitfield_or(bitfield_t * tgt, const bitfield_t * src) {
    size_t i, j, size_in_bits, size;
    if (!tgt || !src) return;
    size_in_bits = min(tgt->size_in_bits, src->size_in_bits);
    size = size_in_bits / 8;
    for (i = 0; i < size; ++i) {
        if (i + 1 == size) {
            for (j = 0; j < size_in_bits % 8; ++j) {
                if (src->mask[i] & (1 << j)) {
                    bitfield_set_1(tgt->mask, 8 * i + j);
                }
            }
        } else {
            tgt->mask[i] |= src->mask[i]; 
        }
    }
}

/**
 * \brief Apply ~ to each byte (tgt ~= tgt)
 * \param tgt The bitfield we modify 
 */

void bitfield_not(bitfield_t * tgt) {
    size_t i, j, size_in_bits, size, offset;
    if (!tgt) return;
    size_in_bits = tgt->size_in_bits;
    size = size_in_bits / 8;
    for (i = 0; i < size; ++i) {
        if (i + 1 == size) {
            for (j = 0; j < size_in_bits % 8; ++j) {
                if (tgt->mask[i] & (1 << j)) {
                    bitfield_set_1(tgt->mask, 8 * i + j);
                } else {
                    bitfield_set_0(tgt->mask, 8 * i + j);
                }
            }
        } else {
            tgt->mask[i] = ~tgt->mask[i]; 
        }
    }
}

