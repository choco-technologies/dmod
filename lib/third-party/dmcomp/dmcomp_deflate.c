/**
 * @file dmcomp_deflate.c
 * @brief DEFLATE encoder (RFC 1951), memory to memory
 *
 * LZ77 with hash chains over the whole input (it is in memory, so there is no sliding window to manage), optionally
 * with lazy matching, then every block of up to DMCOMP_BLOCK_TOKENS tokens is written as the smallest of a stored,
 * a fixed-code and a dynamic-code block. The code lengths are length limited Huffman codes.
 *
 * It is a tool for the host: ~330 kB of work memory. Everything the format needs is here, nothing of the decoder is.
 */
#include <stdlib.h>
#include <string.h>
#include "dmcomp.h"
#include "dmcomp_tables.h"

#ifndef DMCOMP_MALLOC
#   define DMCOMP_MALLOC    malloc
#   define DMCOMP_FREE      free
#endif

#define WINDOW_SIZE         32768u
#define WINDOW_MASK         (WINDOW_SIZE - 1u)
#define MAX_DISTANCE        (WINDOW_SIZE - 1u)      // 32768 would do for the format; this keeps the chains simple
#define HASH_BITS           15
#define BLOCK_TOKENS        16384u
#define TOO_FAR             4096u                   // A match of 3 bytes this far is not worth its bits
#define STORED_MAX          65535u
#define MAX_INPUT           0xFFFF0000u             // Positions are 32 bit

typedef struct
{
    uint16_t    good;       //!< A previous match this long: search a quarter of the chain
    uint16_t    lazy;       //!< A match this long is taken at once (lazy matching only)
    uint16_t    nice;       //!< A match this long ends the search
    uint16_t    chain;      //!< Candidates looked at per position
    uint8_t     lazy_on;    //!< Look whether the next position has a longer match
} level_t;

static const level_t g_levels[10] =
{
    {  0,   0,   0,    0, 0 },
    {  4,   4,   8,    4, 0 },
    {  4,   5,  16,    8, 0 },
    {  4,   6,  32,   32, 0 },
    {  4,   4,  16,   16, 1 },
    {  8,  16,  32,   32, 1 },
    {  8,  16, 128,  128, 1 },
    {  8,  32, 128,  256, 1 },
    { 32, 128, 258, 1024, 1 },
    { 32, 258, 258, 4096, 1 },
};

// =====================================================================================================================
//      Bit writer
// =====================================================================================================================

typedef struct
{
    uint8_t*    start;
    uint8_t*    p;
    uint8_t*    end;
    uint32_t    acc;        //!< Bits not written yet, the first one lowest
    unsigned    n;          //!< Their number (always < 8 between the calls)
    int         overflow;   //!< The output buffer was too small - the rest is counted, not written
} bitwriter_t;

static void put( bitwriter_t* w, uint32_t value, unsigned count )
{
    w->acc |= value << w->n;
    w->n   += count;
    while( w->n >= 8 )
    {
        if( w->p < w->end )
        {
            *w->p++ = (uint8_t)w->acc;
        }
        else
        {
            w->overflow = 1;
        }
        w->acc >>= 8;
        w->n   -= 8;
    }
}

static void put_stored( bitwriter_t* w, const uint8_t* data, size_t length, int final )
{
    do
    {
        size_t chunk = length > STORED_MAX ? STORED_MAX : length;

        length -= chunk;
        put( w, (final && length == 0) ? 1u : 0u, 1 );
        put( w, 0, 2 );
        put( w, 0, (8 - w->n) & 7u );           // To the next byte
        put( w, (uint32_t)chunk, 16 );
        put( w, (uint32_t)(~chunk & 0xFFFFu), 16 );
        if( (size_t)(w->end - w->p) >= chunk )
        {
            if( chunk > 0 )
            {
                memcpy( w->p, data, chunk );
            }
            w->p += chunk;
        }
        else
        {
            w->overflow = 1;
        }
        if( chunk > 0 )
        {
            data += chunk;
        }
    }
    while( length > 0 );
}

// =====================================================================================================================
//      Huffman codes
// =====================================================================================================================

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

/** Canonical codes of the given lengths, ready to be put into the stream (first bit lowest) */
static void make_codes( const uint8_t* lengths, unsigned symbols, uint16_t* codes )
{
    unsigned count[DMCOMP_MAX_BITS + 1] = { 0 };
    unsigned next[DMCOMP_MAX_BITS + 2];
    unsigned i, length, code = 0;

    for( i = 0; i < symbols; i++ )
    {
        count[lengths[i]]++;
    }
    count[0] = 0;
    for( length = 1; length <= DMCOMP_MAX_BITS; length++ )
    {
        code = (code + count[length - 1]) << 1;
        next[length] = code;
    }
    for( i = 0; i < symbols; i++ )
    {
        codes[i] = lengths[i] ? (uint16_t)reverse_bits( next[lengths[i]]++, lengths[i] ) : 0;
    }
}

/**
 * Code lengths of the symbols of the given frequencies, none longer than @p limit.
 *
 * Huffman's tree is built by merging the two lightest nodes (leaves sorted once, the merged nodes come out in
 * order, so two queues are enough). A tree deeper than the limit is cut the way zlib does it: the leaves below the
 * limit move up and make room by pushing a shallower leaf down. At least two symbols get a code (a stream with one
 * code is valid but some decoders do not take it).
 */
static void build_lengths( const uint32_t* freq, unsigned symbols, unsigned limit, uint8_t* lengths )
{
    uint32_t    weight[2 * DMCOMP_LITLEN_SYMBOLS];
    uint16_t    parent[2 * DMCOMP_LITLEN_SYMBOLS];
    uint16_t    depth[2 * DMCOMP_LITLEN_SYMBOLS];
    uint32_t    f[DMCOMP_LITLEN_SYMBOLS];
    uint16_t    order[DMCOMP_LITLEN_SYMBOLS];       // The used symbols, the rarest first
    unsigned    bl_count[DMCOMP_MAX_BITS + 1] = { 0 };
    unsigned    i, j, used = 0, overflow = 0;

    for( i = 0; i < symbols; i++ )
    {
        f[i] = freq[i];
        lengths[i] = 0;
        if( f[i] != 0 )
        {
            used++;
        }
    }
    for( i = 0; used < 2 && i < symbols; i++ )
    {
        if( f[i] == 0 )
        {
            f[i] = 1;
            used++;
        }
    }

    // Insertion sort by frequency (then by value): there are at most 286 symbols
    used = 0;
    for( i = 0; i < symbols; i++ )
    {
        if( f[i] == 0 )
        {
            continue;
        }
        for( j = used; j > 0 && f[order[j - 1]] > f[i]; j-- )
        {
            order[j] = order[j - 1];
        }
        order[j] = (uint16_t)i;
        used++;
    }

    // The leaves are nodes 0..used-1, the merged nodes follow
    {
        unsigned leaf = 0, node = used, created = used;
        unsigned step;

        for( i = 0; i < used; i++ )
        {
            weight[i] = f[order[i]];
        }
        for( step = 0; step + 1 < used; step++ )
        {
            unsigned pick[2];
            for( j = 0; j < 2; j++ )
            {
                if( leaf < used && (node >= created || weight[leaf] <= weight[node]) )
                {
                    pick[j] = leaf++;
                }
                else
                {
                    pick[j] = node++;
                }
            }
            weight[created]  = weight[pick[0]] + weight[pick[1]];
            parent[pick[0]]  = (uint16_t)created;
            parent[pick[1]]  = (uint16_t)created;
            created++;
        }

        // The root is the last node; every parent comes after its children. A node deeper than the limit is cut to it
        // (its children are measured from there) - every node that is counts as overflow, the inner ones too: that is
        // what the correction below is made of
        depth[created - 1] = 0;
        for( i = created - 1; i-- > 0; )
        {
            unsigned d = depth[parent[i]] + 1u;
            if( d > limit )
            {
                d = limit;
                overflow++;
            }
            depth[i] = (uint16_t)d;
        }
        for( i = 0; i < used; i++ )
        {
            bl_count[depth[i]]++;
        }
    }

    while( overflow > 0 )
    {
        unsigned bits = limit - 1;
        while( bl_count[bits] == 0 )
        {
            bits--;
        }
        bl_count[bits]--;           // A leaf moves down...
        bl_count[bits + 1] += 2;    // ...and takes one of the leaves that were cut as its brother
        bl_count[limit]--;
        overflow = overflow > 2 ? overflow - 2 : 0;
    }

    // The rarest symbols get the longest codes
    {
        unsigned k = 0, length;
        for( length = limit; length >= 1; length-- )
        {
            for( i = 0; i < bl_count[length]; i++ )
            {
                lengths[order[k++]] = (uint8_t)length;
            }
        }
    }
}

/**
 * Run-length codes the lengths of the codes (the alphabet of RFC 1951, 3.2.7): 0..15 as they are, 16 repeats the
 * previous one 3-6 times, 17 and 18 repeat zero 3-10 and 11-138 times.
 *
 * @return number of symbols; symbols[] and extras[] (the value of the extra bits) are filled
 */
static unsigned rle_lengths( const uint8_t* lengths, unsigned total, uint8_t* symbols, uint8_t* extras )
{
    unsigned i = 0, k = 0;

    while( i < total )
    {
        uint8_t  value = lengths[i];
        unsigned run = 1;

        while( i + run < total && lengths[i + run] == value )
        {
            run++;
        }
        i += run;

        if( value == 0 )
        {
            while( run >= 11 )
            {
                unsigned r = run > 138 ? 138 : run;
                symbols[k] = 18;
                extras[k++] = (uint8_t)(r - 11);
                run -= r;
            }
            if( run >= 3 )
            {
                symbols[k] = 17;
                extras[k++] = (uint8_t)(run - 3);
                run = 0;
            }
            while( run-- > 0 )
            {
                symbols[k] = 0;
                extras[k++] = 0;
            }
        }
        else
        {
            symbols[k] = value;
            extras[k++] = 0;
            run--;
            while( run >= 3 )
            {
                unsigned r = run > 6 ? 6 : run;
                symbols[k] = 16;
                extras[k++] = (uint8_t)(r - 3);
                run -= r;
            }
            while( run-- > 0 )
            {
                symbols[k] = value;
                extras[k++] = 0;
            }
        }
    }
    return k;
}

// =====================================================================================================================
//      Encoder state
// =====================================================================================================================

typedef struct
{
    const uint8_t*  in;
    size_t          n;
    const level_t*  cfg;
    bitwriter_t     w;

    size_t          ins;            //!< Positions below it are in the hash chains
    size_t          cov;            //!< The input covered by the tokens so far (the written blocks included)
    size_t          blk_start;      //!< Where the block being collected starts
    unsigned        ntok;
    uint32_t        lit_freq[DMCOMP_LITLEN_SYMBOLS];
    uint32_t        dist_freq[DMCOMP_DIST_SYMBOLS];
    uint8_t         lcode[DMCOMP_MAX_MATCH + 1];    //!< Length -> length code (0..28)
    uint8_t         dcode[512];                     //!< Distance - 1 -> distance code, see DIST_CODE
    uint32_t        head[1u << HASH_BITS];          //!< Newest position (+1) of every hash
    uint32_t        prev[WINDOW_SIZE];              //!< Older position (+1) with the same hash, by position
    uint32_t        tokens[BLOCK_TOKENS];           //!< A literal: its value; a match: (length << 16) | distance
} encoder_t;

#define DIST_CODE( e, d )   ((e)->dcode[(d) < 256u ? (d) : 256u + ((d) >> 7)])

static void init_code_tables( encoder_t* e )
{
    unsigned c, v;

    for( c = 0; c < 28; c++ )
    {
        for( v = dmcomp_len_base[c]; v < dmcomp_len_base[c] + (1u << dmcomp_len_extra[c]); v++ )
        {
            e->lcode[v] = (uint8_t)c;
        }
    }
    e->lcode[258] = 28;     // 258 has a code of its own, the last of code 27's range is not used

    for( c = 0; c < DMCOMP_DIST_SYMBOLS; c++ )
    {
        for( v = dmcomp_dist_base[c] - 1u; v < dmcomp_dist_base[c] - 1u + (1u << dmcomp_dist_extra[c]); v++ )
        {
            e->dcode[v < 256u ? v : 256u + (v >> 7)] = (uint8_t)c;
        }
    }
}

// =====================================================================================================================
//      Matching
// =====================================================================================================================

static unsigned hash3( const uint8_t* p )
{
    uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    return (v * 2654435761u) >> (32 - HASH_BITS);
}

/** Puts the positions below @p upto into the hash chains */
static void insert_upto( encoder_t* e, size_t upto )
{
    while( e->ins < upto )
    {
        size_t pos = e->ins++;
        if( pos + 2 < e->n )
        {
            unsigned h = hash3( e->in + pos );
            e->prev[pos & WINDOW_MASK] = e->head[h];
            e->head[h] = (uint32_t)pos + 1;
        }
    }
}

/**
 * Longest match for the data at @p pos, only if it is longer than @p better_than (and at least 3 bytes). The
 * positions below @p pos have to be in the chains, @p pos itself not yet.
 *
 * @return the length, or 0
 */
static unsigned find_match( encoder_t* e, size_t pos, unsigned better_than, unsigned chain, unsigned* distance )
{
    size_t          avail = e->n - pos;
    const uint8_t*  cur = e->in + pos;
    unsigned        max_len = avail > DMCOMP_MAX_MATCH ? DMCOMP_MAX_MATCH : (unsigned)avail;
    unsigned        best = better_than < DMCOMP_MIN_MATCH - 1 ? DMCOMP_MIN_MATCH - 1 : better_than;
    unsigned        best_distance = 0;
    uint32_t        candidate;

    if( avail < DMCOMP_MIN_MATCH || best >= max_len )
    {
        return 0;
    }

    candidate = e->head[hash3( cur )];
    while( candidate != 0 && chain-- > 0 )
    {
        size_t          cand_pos = candidate - 1;
        size_t          d = pos - cand_pos;
        const uint8_t*  m = e->in + cand_pos;
        uint32_t        next;

        if( cand_pos >= pos || d > MAX_DISTANCE )
        {
            break;
        }
        if( m[best] == cur[best] && m[0] == cur[0] )
        {
            unsigned l = 0;
            while( l < max_len && m[l] == cur[l] )
            {
                l++;
            }
            if( l > best )
            {
                best = l;
                best_distance = (unsigned)d;
                if( l >= e->cfg->nice || l >= max_len )
                {
                    break;
                }
            }
        }
        next = e->prev[cand_pos & WINDOW_MASK];
        if( next >= candidate )
        {
            break;      // The chain goes back in time, always
        }
        candidate = next;
    }

    if( best_distance == 0 || (best == DMCOMP_MIN_MATCH && best_distance > TOO_FAR) )
    {
        return 0;
    }
    *distance = best_distance;
    return best;
}

// =====================================================================================================================
//      Blocks
// =====================================================================================================================

static uint64_t data_bits( const encoder_t* e, const uint8_t* lit_len, const uint8_t* dist_len )
{
    uint64_t bits = 0;
    unsigned s;

    for( s = 0; s < DMCOMP_LITLEN_SYMBOLS; s++ )
    {
        bits += (uint64_t)e->lit_freq[s] * lit_len[s];
    }
    for( s = 257; s < DMCOMP_LITLEN_SYMBOLS; s++ )
    {
        bits += (uint64_t)e->lit_freq[s] * dmcomp_len_extra[s - 257];
    }
    for( s = 0; s < DMCOMP_DIST_SYMBOLS; s++ )
    {
        bits += (uint64_t)e->dist_freq[s] * (dist_len[s] + dmcomp_dist_extra[s]);
    }
    return bits;
}

static void put_tokens( encoder_t* e, const uint16_t* lit_code, const uint8_t* lit_len, const uint16_t* dist_code,
                        const uint8_t* dist_len )
{
    unsigned i;

    for( i = 0; i < e->ntok; i++ )
    {
        uint32_t t = e->tokens[i];
        if( (t >> 16) == 0 )
        {
            put( &e->w, lit_code[t], lit_len[t] );
        }
        else
        {
            unsigned length = t >> 16;
            unsigned distance = t & 0xFFFFu;
            unsigned lc = e->lcode[length];
            unsigned dc = DIST_CODE( e, distance - 1u );

            put( &e->w, lit_code[257 + lc], lit_len[257 + lc] );
            if( dmcomp_len_extra[lc] != 0 )
            {
                put( &e->w, length - dmcomp_len_base[lc], dmcomp_len_extra[lc] );
            }
            put( &e->w, dist_code[dc], dist_len[dc] );
            if( dmcomp_dist_extra[dc] != 0 )
            {
                put( &e->w, distance - dmcomp_dist_base[dc], dmcomp_dist_extra[dc] );
            }
        }
    }
    put( &e->w, lit_code[256], lit_len[256] );
}

/** Writes the collected tokens as one block - the smallest of the three kinds - and starts the next one */
static void flush_block( encoder_t* e, int final )
{
    uint8_t     lit_len[DMCOMP_LITLEN_SYMBOLS], dist_len[DMCOMP_DIST_SYMBOLS];
    uint8_t     fixed_lit_len[288], fixed_dist_len[DMCOMP_DIST_SYMBOLS];
    uint8_t     all_len[DMCOMP_LITLEN_SYMBOLS + DMCOMP_DIST_SYMBOLS];
    uint8_t     rle_sym[DMCOMP_LITLEN_SYMBOLS + DMCOMP_DIST_SYMBOLS], rle_extra[DMCOMP_LITLEN_SYMBOLS + DMCOMP_DIST_SYMBOLS];
    uint32_t    clen_freq[DMCOMP_CLEN_SYMBOLS] = { 0 };
    uint8_t     clen_len[DMCOMP_CLEN_SYMBOLS];
    unsigned    hlit, hdist, hclen, nrle, i;
    uint64_t    dynamic_bits, fixed_bits, stored_bits;
    size_t      span = e->cov - e->blk_start;

    e->lit_freq[256] = 1;       // The end of the block

    // --- Dynamic codes
    build_lengths( e->lit_freq, DMCOMP_LITLEN_SYMBOLS, DMCOMP_MAX_BITS, lit_len );
    build_lengths( e->dist_freq, DMCOMP_DIST_SYMBOLS, DMCOMP_MAX_BITS, dist_len );
    for( hlit = DMCOMP_LITLEN_SYMBOLS; hlit > 257 && lit_len[hlit - 1] == 0; hlit-- ) {}
    for( hdist = DMCOMP_DIST_SYMBOLS; hdist > 1 && dist_len[hdist - 1] == 0; hdist-- ) {}
    memcpy( all_len, lit_len, hlit );
    memcpy( all_len + hlit, dist_len, hdist );
    nrle = rle_lengths( all_len, hlit + hdist, rle_sym, rle_extra );
    for( i = 0; i < nrle; i++ )
    {
        clen_freq[rle_sym[i]]++;
    }
    build_lengths( clen_freq, DMCOMP_CLEN_SYMBOLS, DMCOMP_MAX_CLEN_BITS, clen_len );
    for( hclen = DMCOMP_CLEN_SYMBOLS; hclen > 4 && clen_len[dmcomp_clen_order[hclen - 1]] == 0; hclen-- ) {}

    dynamic_bits = 3 + 14 + 3 * (uint64_t)hclen + data_bits( e, lit_len, dist_len );
    for( i = 0; i < nrle; i++ )
    {
        dynamic_bits += clen_len[rle_sym[i]] + (rle_sym[i] == 16 ? 2 : rle_sym[i] == 17 ? 3 : rle_sym[i] == 18 ? 7 : 0);
    }

    // --- Fixed codes
    for( i = 0; i < 144; i++ ) fixed_lit_len[i] = 8;
    for( ; i < 256; i++ )      fixed_lit_len[i] = 9;
    for( ; i < 280; i++ )      fixed_lit_len[i] = 7;
    for( ; i < 288; i++ )      fixed_lit_len[i] = 8;
    for( i = 0; i < DMCOMP_DIST_SYMBOLS; i++ ) fixed_dist_len[i] = 5;
    fixed_bits = 3 + data_bits( e, fixed_lit_len, fixed_dist_len );

    // --- Stored: 3 bits and the padding in front of the first chunk, 4 bytes in front of every chunk
    {
        uint64_t chunks = span == 0 ? 1 : (span + STORED_MAX - 1) / STORED_MAX;
        unsigned pad = (8 - ((e->w.n + 3) & 7u)) & 7u;
        stored_bits = 3 + pad + 32 * chunks + 8 * (chunks - 1) + 8 * (uint64_t)span;
    }

    if( stored_bits < fixed_bits && stored_bits < dynamic_bits )
    {
        put_stored( &e->w, e->in + e->blk_start, span, final );     // Writes the BFINAL bit itself
    }
    else if( fixed_bits <= dynamic_bits )
    {
        uint16_t lit_code[288], dist_code[DMCOMP_DIST_SYMBOLS];

        make_codes( fixed_lit_len, 288, lit_code );
        make_codes( fixed_dist_len, DMCOMP_DIST_SYMBOLS, dist_code );
        put( &e->w, final ? 1u : 0u, 1 );
        put( &e->w, 1, 2 );
        put_tokens( e, lit_code, fixed_lit_len, dist_code, fixed_dist_len );
    }
    else
    {
        uint16_t lit_code[DMCOMP_LITLEN_SYMBOLS], dist_code[DMCOMP_DIST_SYMBOLS], clen_code[DMCOMP_CLEN_SYMBOLS];

        make_codes( lit_len, DMCOMP_LITLEN_SYMBOLS, lit_code );
        make_codes( dist_len, DMCOMP_DIST_SYMBOLS, dist_code );
        make_codes( clen_len, DMCOMP_CLEN_SYMBOLS, clen_code );

        put( &e->w, final ? 1u : 0u, 1 );
        put( &e->w, 2, 2 );
        put( &e->w, hlit - 257, 5 );
        put( &e->w, hdist - 1, 5 );
        put( &e->w, hclen - 4, 4 );
        for( i = 0; i < hclen; i++ )
        {
            put( &e->w, clen_len[dmcomp_clen_order[i]], 3 );
        }
        for( i = 0; i < nrle; i++ )
        {
            put( &e->w, clen_code[rle_sym[i]], clen_len[rle_sym[i]] );
            if( rle_sym[i] == 16 )      put( &e->w, rle_extra[i], 2 );
            else if( rle_sym[i] == 17 ) put( &e->w, rle_extra[i], 3 );
            else if( rle_sym[i] == 18 ) put( &e->w, rle_extra[i], 7 );
        }
        put_tokens( e, lit_code, lit_len, dist_code, dist_len );
    }

    memset( e->lit_freq, 0, sizeof(e->lit_freq) );
    memset( e->dist_freq, 0, sizeof(e->dist_freq) );
    e->ntok = 0;
    e->blk_start = e->cov;
}

static void emit_literal( encoder_t* e, uint8_t value )
{
    e->tokens[e->ntok++] = value;
    e->lit_freq[value]++;
    e->cov += 1;
}

static void emit_match( encoder_t* e, unsigned length, unsigned distance )
{
    e->tokens[e->ntok++] = ((uint32_t)length << 16) | distance;
    e->lit_freq[257 + e->lcode[length]]++;
    e->dist_freq[DIST_CODE( e, distance - 1u )]++;
    e->cov += length;
}

static void compress_blocks( encoder_t* e )
{
    const level_t*  cfg = e->cfg;
    size_t          i = 0;
    unsigned        length = 0, distance = 0;
    int             have_match = 0;     // 'length' and 'distance' are the match at i (found while looking ahead)

    while( i < e->n )
    {
        if( !have_match )
        {
            insert_upto( e, i );
            length = find_match( e, i, 0, cfg->chain, &distance );
            insert_upto( e, i + 1 );
        }
        have_match = 0;

        if( length >= DMCOMP_MIN_MATCH )
        {
            if( cfg->lazy_on && length < cfg->lazy && i + 1 < e->n )
            {
                // A longer match one byte later is worth a literal now
                unsigned next_distance = 0;
                unsigned chain = length >= cfg->good ? cfg->chain >> 2 : cfg->chain;
                unsigned next_length = find_match( e, i + 1, length, chain, &next_distance );

                insert_upto( e, i + 2 );
                if( next_length > length )
                {
                    emit_literal( e, e->in[i] );
                    i++;
                    length = next_length;
                    distance = next_distance;
                    have_match = 1;
                    if( e->ntok >= BLOCK_TOKENS )
                    {
                        flush_block( e, 0 );
                    }
                    continue;
                }
            }
            emit_match( e, length, distance );
            insert_upto( e, i + length );
            i += length;
        }
        else
        {
            emit_literal( e, e->in[i] );
            i++;
        }

        if( e->ntok >= BLOCK_TOKENS )
        {
            flush_block( e, 0 );
        }
    }
    flush_block( e, 1 );
}

// =====================================================================================================================
//      Public API
// =====================================================================================================================

size_t dmcomp_deflate_bound( size_t src_size )
{
    // The worst case is a stored block: 5 bytes (the header, the padding, LEN and NLEN) per 64 kB, and a block is at
    // least BLOCK_TOKENS bytes long unless it is the last one
    return src_size + 6 * (src_size / BLOCK_TOKENS + 2) + 16;
}

int dmcomp_deflate( const void* src, size_t src_size, void* dst, size_t dst_capacity, size_t* out_size, int level )
{
    encoder_t*  e;
    int         result;

    if( out_size == NULL || (src == NULL && src_size != 0) || (dst == NULL && dst_capacity != 0) ||
        src_size > MAX_INPUT )
    {
        return DMCOMP_ERR_PARAM;
    }
    *out_size = 0;

    if( level < 0 )
    {
        level = DMCOMP_LEVEL_DEFAULT;
    }
    else if( level > 9 )
    {
        level = 9;
    }

    e = (encoder_t*)DMCOMP_MALLOC( sizeof(encoder_t) );
    if( e == NULL )
    {
        return DMCOMP_ERR_MEMORY;
    }
    memset( e, 0, offsetof( encoder_t, prev ) );    // The chains (prev) and the tokens are written before they are read
    e->in      = (const uint8_t*)src;
    e->n       = src_size;
    e->cfg     = &g_levels[level];
    e->w.start = (uint8_t*)dst;
    e->w.p     = (uint8_t*)dst;
    e->w.end   = dst_capacity != 0 ? (uint8_t*)dst + dst_capacity : (uint8_t*)dst;     // NULL is not moved
    init_code_tables( e );

    if( level == 0 )
    {
        put_stored( &e->w, e->in, e->n, 1 );
    }
    else
    {
        compress_blocks( e );
    }
    if( e->w.n > 0 )
    {
        put( &e->w, 0, 8 - e->w.n );
    }

    result = e->w.overflow ? DMCOMP_ERR_OVERFLOW : DMCOMP_OK;
    if( result == DMCOMP_OK )
    {
        *out_size = (size_t)(e->w.p - e->w.start);
    }
    DMCOMP_FREE( e );
    return result;
}
