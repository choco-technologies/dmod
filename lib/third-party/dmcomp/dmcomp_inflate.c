/**
 * @file dmcomp_inflate.c
 * @brief DEFLATE decoder (RFC 1951), memory to memory
 *
 * Design: the output buffer is the window, the state is a few kB on the stack, there is no heap and no global state
 * (reentrant). Huffman codes are decoded with a table of the first DMCOMP_*_FAST_BITS bits of the stream; the rare
 * longer codes are walked canonically, bit by bit. The input is never read beyond its end: the missing bytes read as
 * zeros, and the decoder notices that it used them (DMCOMP_ERR_TRUNCATED) before it acts on them.
 */
#include <string.h>
#include "dmcomp.h"
#include "dmcomp_tables.h"

/** Bits of the table of literal/length codes: 2^bits * 2 bytes of stack. 9 covers almost every code of real data. */
#ifndef DMCOMP_LIT_FAST_BITS
#   define DMCOMP_LIT_FAST_BITS     9
#endif

/** Bits of the table of distance codes: 2^bits * 2 bytes of stack */
#ifndef DMCOMP_DIST_FAST_BITS
#   define DMCOMP_DIST_FAST_BITS    7
#endif

#define CLEN_FAST_BITS      DMCOMP_MAX_CLEN_BITS   // The whole alphabet of code lengths is in the table
#define LITLEN_TABLE_SIZE   288                    // Fixed codes have 288 symbols (286 and 287 are invalid)
#define DIST_TABLE_SIZE     32                     // Fixed codes have 32 symbols (30 and 31 are invalid)

// =====================================================================================================================
//      Bit reader
// =====================================================================================================================

typedef struct
{
    const uint8_t*  p;      //!< Next byte of the input
    const uint8_t*  end;    //!< End of the input
    uint32_t        bits;   //!< Bits not used yet (the next one is the lowest)
    unsigned        n;      //!< Number of valid bits in 'bits'
    unsigned        virt;   //!< Number of the bits that were added past the end of the input (they are the highest)
} bitreader_t;

/** Fills the buffer to 25..32 bits. Past the end of the input zeros are added (and counted in 'virt'). */
static void refill( bitreader_t* b )
{
    while( b->n <= 24 )
    {
        uint32_t byte = 0;
        if( b->p < b->end )
        {
            byte = *b->p++;
        }
        else
        {
            b->virt += 8;
        }
        b->bits |= byte << b->n;
        b->n += 8;
    }
}

/** True when a bit that does not exist was used - the input is too short */
static int overrun( const bitreader_t* b )
{
    return b->n < b->virt;
}

/** Takes @p count bits (0..16); the buffer must have been filled */
static uint32_t take( bitreader_t* b, unsigned count )
{
    uint32_t value = b->bits & ((1u << count) - 1u);
    b->bits >>= count;
    b->n -= count;
    return value;
}

// =====================================================================================================================
//      Huffman codes
// =====================================================================================================================

/**
 * A code is described by three arrays:
 *  - fast[]  : indexed by the next 'fast_bits' bits of the stream: (symbol << 4) | length of the code; 0: the code is
 *              longer than the table (or does not exist)
 *  - count[] : the number of codes of every length
 *  - sym[]   : the symbols sorted by the length of their codes, then by the value
 */
typedef struct
{
    uint16_t    lit_fast[1u << DMCOMP_LIT_FAST_BITS];
    uint16_t    lit_count[DMCOMP_MAX_BITS + 1];
    uint16_t    lit_sym[LITLEN_TABLE_SIZE];
    uint16_t    dist_fast[1u << DMCOMP_DIST_FAST_BITS];
    uint16_t    dist_count[DMCOMP_MAX_BITS + 1];
    uint16_t    dist_sym[DIST_TABLE_SIZE];

    // The header of a dynamic block (the lengths of the codes, and the code of the lengths)
    uint8_t     lengths[LITLEN_TABLE_SIZE + DIST_TABLE_SIZE];
    uint8_t     clen[DMCOMP_CLEN_SYMBOLS];
    uint16_t    cfast[1u << CLEN_FAST_BITS];
    uint16_t    ccount[DMCOMP_MAX_BITS + 1];
    uint16_t    csym[DMCOMP_CLEN_SYMBOLS];
} workspace_t;

static unsigned reverse_bits( unsigned code, unsigned length )
{
    unsigned reversed = 0;
    while( length-- > 0 )
    {
        reversed = (reversed << 1) | (code & 1u);
        code >>= 1;
    }
    return reversed;
}

/**
 * Builds the decoding tables of a code described by the lengths of its symbols.
 *
 * The set of codes has to be complete (it may not leave any bit sequence unused), with one exception that zlib allows
 * too: a single code of one bit. No codes at all is accepted, every decoding fails then.
 */
static int huff_build( uint16_t* fast, unsigned fast_bits, uint16_t* count, uint16_t* sym, const uint8_t* lengths,
                       unsigned symbols )
{
    uint16_t    offsets[DMCOMP_MAX_BITS + 2];
    unsigned    i, length, used;
    int         left = 1;

    for( length = 0; length <= DMCOMP_MAX_BITS; length++ )
    {
        count[length] = 0;
    }
    for( i = 0; i < symbols; i++ )
    {
        count[lengths[i]]++;
    }
    used = symbols - count[0];

    for( i = 0; i < (1u << fast_bits); i++ )
    {
        fast[i] = 0;
    }
    if( used == 0 )
    {
        return DMCOMP_OK;
    }

    for( length = 1; length <= DMCOMP_MAX_BITS; length++ )
    {
        left <<= 1;
        left -= count[length];
        if( left < 0 )
        {
            return DMCOMP_ERR_DATA;     // Over-subscribed
        }
    }
    if( left > 0 && !(used == 1 && count[1] == 1) )
    {
        return DMCOMP_ERR_DATA;         // Incomplete
    }

    offsets[1] = 0;
    for( length = 1; length < DMCOMP_MAX_BITS; length++ )
    {
        offsets[length + 1] = (uint16_t)(offsets[length] + count[length]);
    }
    for( i = 0; i < symbols; i++ )
    {
        if( lengths[i] != 0 )
        {
            sym[offsets[lengths[i]]++] = (uint16_t)i;
        }
    }

    // The canonical codes are given out in the order of 'sym'; the stream has them with the first bit lowest
    unsigned code = 0, index = 0;
    for( length = 1; length <= DMCOMP_MAX_BITS; length++ )
    {
        for( i = 0; i < count[length]; i++, index++, code++ )
        {
            if( length <= fast_bits )
            {
                unsigned j;
                uint16_t entry = (uint16_t)((sym[index] << 4) | length);
                for( j = reverse_bits( code, length ); j < (1u << fast_bits); j += (1u << length) )
                {
                    fast[j] = entry;
                }
            }
        }
        code <<= 1;
    }
    return DMCOMP_OK;
}

/** Decodes one symbol; the buffer must have been filled. Returns the symbol or -1 for a bit sequence of no code. */
static int huff_decode( bitreader_t* b, const uint16_t* fast, unsigned fast_bits, const uint16_t* count,
                        const uint16_t* sym )
{
    uint16_t entry = fast[b->bits & ((1u << fast_bits) - 1u)];
    if( entry != 0 )
    {
        take( b, entry & 15u );
        return entry >> 4;
    }

    // A code longer than the table (or no code): walk it canonically, one bit at a time
    int      code = 0, first = 0, index = 0;
    uint32_t bits = b->bits;
    unsigned length;
    for( length = 1; length <= DMCOMP_MAX_BITS; length++ )
    {
        int c;
        code |= (int)(bits & 1u);
        bits >>= 1;
        c = count[length];
        if( code - c < first )
        {
            take( b, length );
            return sym[index + (code - first)];
        }
        index += c;
        first += c;
        first <<= 1;
        code <<= 1;
    }
    return -1;
}

// =====================================================================================================================
//      Blocks
// =====================================================================================================================

static int build_fixed( workspace_t* t )
{
    uint8_t* lengths = t->lengths;
    unsigned i;
    for( i = 0; i < 144; i++ ) lengths[i] = 8;
    for( ; i < 256; i++ )      lengths[i] = 9;
    for( ; i < 280; i++ )      lengths[i] = 7;
    for( ; i < 288; i++ )      lengths[i] = 8;
    int r = huff_build( t->lit_fast, DMCOMP_LIT_FAST_BITS, t->lit_count, t->lit_sym, lengths, LITLEN_TABLE_SIZE );
    if( r != DMCOMP_OK )
    {
        return r;
    }
    for( i = 0; i < DIST_TABLE_SIZE; i++ ) lengths[i] = 5;
    return huff_build( t->dist_fast, DMCOMP_DIST_FAST_BITS, t->dist_count, t->dist_sym, lengths, DIST_TABLE_SIZE );
}

/** Reads the header of a dynamic block (RFC 1951, 3.2.7) and builds its tables */
static int build_dynamic( bitreader_t* b, workspace_t* t )
{
    uint8_t*    lengths = t->lengths;
    uint8_t*    clen = t->clen;
    uint16_t*   cfast = t->cfast;
    uint16_t*   ccount = t->ccount;
    uint16_t*   csym = t->csym;
    unsigned    hlit, hdist, hclen, i, total;
    int         r;

    for( i = 0; i < DMCOMP_CLEN_SYMBOLS; i++ )
    {
        clen[i] = 0;
    }

    refill( b );
    hlit  = take( b, 5 ) + 257;
    hdist = take( b, 5 ) + 1;
    hclen = take( b, 4 ) + 4;
    if( overrun( b ) )
    {
        return DMCOMP_ERR_TRUNCATED;
    }
    if( hlit > DMCOMP_LITLEN_SYMBOLS || hdist > DMCOMP_DIST_SYMBOLS )
    {
        return DMCOMP_ERR_DATA;
    }

    for( i = 0; i < hclen; i++ )
    {
        refill( b );
        clen[dmcomp_clen_order[i]] = (uint8_t)take( b, 3 );
    }
    if( overrun( b ) )
    {
        return DMCOMP_ERR_TRUNCATED;
    }
    r = huff_build( cfast, CLEN_FAST_BITS, ccount, csym, clen, DMCOMP_CLEN_SYMBOLS );
    if( r != DMCOMP_OK )
    {
        return r;
    }

    // The lengths of the literal/length and distance codes are one run-length coded sequence
    total = hlit + hdist;
    for( i = 0; i < total; )
    {
        int         s;
        unsigned    repeat;
        uint8_t     value;

        refill( b );
        s = huff_decode( b, cfast, CLEN_FAST_BITS, ccount, csym );
        if( s < 0 )
        {
            return overrun( b ) ? DMCOMP_ERR_TRUNCATED : DMCOMP_ERR_DATA;
        }
        if( s < 16 )
        {
            lengths[i++] = (uint8_t)s;
            continue;
        }
        if( s == 16 )
        {
            if( i == 0 )
            {
                return DMCOMP_ERR_DATA;
            }
            value  = lengths[i - 1];
            repeat = 3 + take( b, 2 );
        }
        else if( s == 17 )
        {
            value  = 0;
            repeat = 3 + take( b, 3 );
        }
        else
        {
            value  = 0;
            repeat = 11 + take( b, 7 );
        }
        if( overrun( b ) )
        {
            return DMCOMP_ERR_TRUNCATED;
        }
        if( i + repeat > total )
        {
            return DMCOMP_ERR_DATA;
        }
        while( repeat-- > 0 )
        {
            lengths[i++] = value;
        }
    }
    if( lengths[256] == 0 )
    {
        return DMCOMP_ERR_DATA;         // No end of block code
    }

    r = huff_build( t->lit_fast, DMCOMP_LIT_FAST_BITS, t->lit_count, t->lit_sym, lengths, hlit );
    if( r != DMCOMP_OK )
    {
        return r;
    }
    return huff_build( t->dist_fast, DMCOMP_DIST_FAST_BITS, t->dist_count, t->dist_sym, lengths + hlit, hdist );
}

/** Decodes the symbols of a Huffman coded block up to its end of block code */
static int inflate_codes( bitreader_t* b, const workspace_t* t, uint8_t* dst, uint8_t** pout, uint8_t* end )
{
    uint8_t* out = *pout;

    for( ;; )
    {
        int s;

        if( overrun( b ) )
        {
            return DMCOMP_ERR_TRUNCATED;
        }
        refill( b );
        s = huff_decode( b, t->lit_fast, DMCOMP_LIT_FAST_BITS, t->lit_count, t->lit_sym );
        if( s < 0 )
        {
            return overrun( b ) ? DMCOMP_ERR_TRUNCATED : DMCOMP_ERR_DATA;
        }
        if( s < 256 )
        {
            if( out == end )
            {
                return DMCOMP_ERR_OVERFLOW;
            }
            *out++ = (uint8_t)s;
            continue;
        }
        if( s == 256 )
        {
            break;
        }

        // A match: length, then distance (at most 15 + 5 and 15 + 13 bits, so the buffer is filled in between)
        unsigned length, dist;
        int      d;

        s -= 257;
        if( s >= 29 )
        {
            return DMCOMP_ERR_DATA;     // Length codes 286 and 287 are not used
        }
        length = dmcomp_len_base[s] + take( b, dmcomp_len_extra[s] );

        refill( b );
        d = huff_decode( b, t->dist_fast, DMCOMP_DIST_FAST_BITS, t->dist_count, t->dist_sym );
        if( d < 0 )
        {
            return overrun( b ) ? DMCOMP_ERR_TRUNCATED : DMCOMP_ERR_DATA;
        }
        if( d >= DMCOMP_DIST_SYMBOLS )
        {
            return DMCOMP_ERR_DATA;     // Distance codes 30 and 31 are not used
        }
        refill( b );
        dist = dmcomp_dist_base[d] + take( b, dmcomp_dist_extra[d] );

        if( overrun( b ) )
        {
            return DMCOMP_ERR_TRUNCATED;
        }
        if( dist > (size_t)(out - dst) )
        {
            return DMCOMP_ERR_DATA;     // Before the start of the output
        }
        if( length > (size_t)(end - out) )
        {
            return DMCOMP_ERR_OVERFLOW;
        }
        {
            // The source may overlap the destination (dist < length repeats a pattern), so no memcpy
            const uint8_t* from = out - dist;
            while( length-- > 0 )
            {
                *out++ = *from++;
            }
        }
    }

    *pout = out;
    return DMCOMP_OK;
}

/** Copies a stored block (RFC 1951, 3.2.4) */
static int inflate_stored( bitreader_t* b, uint8_t** pout, uint8_t* end )
{
    unsigned length, inverted;

    // The rest of the byte is skipped
    take( b, b->n & 7u );
    refill( b );
    length   = take( b, 16 );
    inverted = take( b, 16 );
    if( overrun( b ) )
    {
        return DMCOMP_ERR_TRUNCATED;
    }
    if( length != (~inverted & 0xFFFFu) )
    {
        return DMCOMP_ERR_DATA;
    }

    // The whole bytes in the buffer go back to the input
    b->p   -= (b->n - b->virt) / 8;
    b->bits = 0;
    b->n    = 0;
    b->virt = 0;

    if( length > (size_t)(b->end - b->p) )
    {
        return DMCOMP_ERR_TRUNCATED;
    }
    if( length > (size_t)(end - *pout) )
    {
        return DMCOMP_ERR_OVERFLOW;
    }
    if( length > 0 )
    {
        memcpy( *pout, b->p, length );
    }
    *pout += length;
    b->p  += length;
    return DMCOMP_OK;
}

// =====================================================================================================================
//      Public API
// =====================================================================================================================

size_t dmcomp_inflate_workspace_size( void )
{
    return sizeof(workspace_t);
}

int dmcomp_inflate_ws( const void* src, size_t src_size, void* dst, size_t dst_capacity, size_t* out_size,
                       void* workspace )
{
    bitreader_t b;
    uint8_t*    out = (uint8_t*)dst;
    uint8_t*    end;
    int         last;
    workspace_t* tables = (workspace_t*)workspace;

    // Checked before any pointer is moved: a NULL pointer is only valid for an empty buffer, and is not added to
    if( out_size == NULL || (src == NULL && src_size != 0) || (dst == NULL && dst_capacity != 0) ||
        workspace == NULL || ((uintptr_t)workspace & (sizeof(uint16_t) - 1u)) != 0 )
    {
        return DMCOMP_ERR_PARAM;
    }
    *out_size = 0;

    end    = dst_capacity != 0 ? out + dst_capacity : out;
    b.p    = (const uint8_t*)src;
    b.end  = src_size != 0 ? b.p + src_size : b.p;
    b.bits = 0;
    b.n    = 0;
    b.virt = 0;

    do
    {
        unsigned    type;
        int         r;

        refill( &b );
        last = (int)take( &b, 1 );
        type = take( &b, 2 );
        if( overrun( &b ) )
        {
            return DMCOMP_ERR_TRUNCATED;
        }

        if( type == 0 )
        {
            r = inflate_stored( &b, &out, end );
        }
        else
        {
            if( type == 1 )
            {
                r = build_fixed( tables );
            }
            else if( type == 2 )
            {
                r = build_dynamic( &b, tables );
            }
            else
            {
                return DMCOMP_ERR_DATA;     // Reserved
            }
            if( r == DMCOMP_OK )
            {
                r = inflate_codes( &b, tables, (uint8_t*)dst, &out, end );
            }
        }
        if( r != DMCOMP_OK )
        {
            return r;
        }
    }
    while( !last );

    if( overrun( &b ) )
    {
        return DMCOMP_ERR_TRUNCATED;
    }
    *out_size = (size_t)(out - (uint8_t*)dst);
    return DMCOMP_OK;
}

int dmcomp_inflate( const void* src, size_t src_size, void* dst, size_t dst_capacity, size_t* out_size )
{
    workspace_t workspace;      // A few kB of the stack: dmcomp_inflate_ws() puts it where the caller wants

    return dmcomp_inflate_ws( src, src_size, dst, dst_capacity, out_size, &workspace );
}
