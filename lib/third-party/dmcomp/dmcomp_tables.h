/**
 * @file dmcomp_tables.h
 * @brief Constants of the DEFLATE format shared by the decoder and the encoder (RFC 1951, 3.2.5 - 3.2.7)
 */
#ifndef DMCOMP_TABLES_H
#define DMCOMP_TABLES_H

#include <stdint.h>

#define DMCOMP_MAX_BITS         15      //!< Longest Huffman code of literals/lengths and distances
#define DMCOMP_MAX_CLEN_BITS    7       //!< Longest code of the code length alphabet
#define DMCOMP_LITLEN_SYMBOLS   286     //!< Literals 0..255, end of block 256, length codes 257..285
#define DMCOMP_DIST_SYMBOLS     30
#define DMCOMP_CLEN_SYMBOLS     19
#define DMCOMP_MIN_MATCH        3
#define DMCOMP_MAX_MATCH        258

/** Base length of the length codes 257..285 (index: code - 257) */
static const uint16_t dmcomp_len_base[29] =
{
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};

/** Extra bits of the length codes */
static const uint8_t dmcomp_len_extra[29] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

/** Base distance of the distance codes */
static const uint16_t dmcomp_dist_base[30] =
{
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};

/** Extra bits of the distance codes */
static const uint8_t dmcomp_dist_extra[30] =
{
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

/** Order in which the lengths of the code length alphabet are stored in a dynamic block header */
static const uint8_t dmcomp_clen_order[DMCOMP_CLEN_SYMBOLS] =
{
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

#endif /* DMCOMP_TABLES_H */
