/**
 * @file dmcomp.h
 * @brief DMOD compression - DEFLATE (RFC 1951) coder and decoder
 *
 * A raw DEFLATE stream (no zlib or gzip wrapper: the sizes travel beside the data, e.g. in the DMFC header) is
 * decoded and encoded memory to memory. Both directions live in this library, so the host tools and the target see
 * the very same implementation:
 *
 *  - the decoder (dmcomp_inflate) is small, needs no heap and no libc beyond <stddef.h>/<stdint.h>/<string.h>; its
 *    state is a few kB on the stack (see DMCOMP_LIT_FAST_BITS / DMCOMP_DIST_FAST_BITS) and the output buffer is the
 *    window - nothing is copied around;
 *  - the encoder (dmcomp_deflate) is meant for the host: it allocates ~330 kB for the hash chains. Build the library
 *    with DMCOMP_NO_ENCODER (CMake: -DDMCOMP_BUILD_ENCODER=OFF) to leave it out.
 *
 * The streams are plain DEFLATE: zlib/gzip tools read what dmcomp_deflate writes (inflateInit2 with -15) and
 * dmcomp_inflate reads what they write.
 */
#ifndef DMCOMP_H
#define DMCOMP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Version of the library */
#define DMCOMP_VERSION_MAJOR    0
#define DMCOMP_VERSION_MINOR    1

/** Results: 0 is success, errors are negative */
typedef enum
{
    DMCOMP_OK               =  0,   //!< Success
    DMCOMP_ERR_PARAM        = -1,   //!< Invalid argument (NULL pointer, size too big for the coder)
    DMCOMP_ERR_DATA         = -2,   //!< The compressed data is not valid DEFLATE
    DMCOMP_ERR_TRUNCATED    = -3,   //!< The compressed data ends before the stream does
    DMCOMP_ERR_OVERFLOW     = -4,   //!< The output buffer is too small
    DMCOMP_ERR_MEMORY       = -5,   //!< The encoder could not allocate its work memory
} dmcomp_result_t;

/**
 * @brief Decompress a raw DEFLATE stream (the tables on the stack: ~3 kB, see dmcomp_inflate_ws() for the other way)
 *
 * Every block type is accepted (stored, fixed and dynamic Huffman codes). The data after the final block is ignored.
 * Corrupted data is never trusted: nothing is read beyond @p src + @p src_size and nothing is written beyond
 * @p dst + @p dst_capacity; on an error the content of @p dst is undefined.
 *
 * @param src           Compressed data
 * @param src_size      Size of the compressed data
 * @param dst           Output buffer (the whole decompressed data is kept in it - it is the window)
 * @param dst_capacity  Size of the output buffer
 * @param out_size      Receives the number of bytes written (may not be NULL)
 *
 * @return DMCOMP_OK or an error (DMCOMP_ERR_DATA, DMCOMP_ERR_TRUNCATED, DMCOMP_ERR_OVERFLOW, DMCOMP_ERR_PARAM)
 */
int dmcomp_inflate( const void* src, size_t src_size, void* dst, size_t dst_capacity, size_t* out_size );

/**
 * @brief Size of the work memory of dmcomp_inflate_ws()
 *
 * The tables of the Huffman codes: ~2.9 kB (see DMCOMP_LIT_FAST_BITS and DMCOMP_DIST_FAST_BITS in dmcomp_inflate.c).
 */
size_t dmcomp_inflate_workspace_size( void );

/**
 * @brief dmcomp_inflate() with the work memory given by the caller
 *
 * The same decoder, but its tables are not on the stack: for a thread with a small one (the decoding may be called from
 * anywhere a module is loaded), give it memory from the heap, or a buffer of your own that is not used at the time.
 *
 * @param workspace     dmcomp_inflate_workspace_size() bytes, aligned to 2 bytes (what malloc returns is); not used after the call
 *
 * Other parameters and results: see dmcomp_inflate(). DMCOMP_ERR_PARAM also for a NULL or odd workspace.
 */
int dmcomp_inflate_ws( const void* src, size_t src_size, void* dst, size_t dst_capacity, size_t* out_size,
                       void* workspace );

/** Name of a result, for logs. Never NULL. */
const char* dmcomp_strerror( int result );

#ifndef DMCOMP_NO_ENCODER

/** Level of compression that dmcomp_deflate uses for a level out of range */
#define DMCOMP_LEVEL_DEFAULT    6

/**
 * @brief Upper bound of the size of the compressed data
 *
 * dmcomp_deflate never writes more than this (incompressible data is stored), so a buffer of this size cannot overflow.
 */
size_t dmcomp_deflate_bound( size_t src_size );

/**
 * @brief Compress data to a raw DEFLATE stream
 *
 * Distances up to 32767 bytes are used. The result is the smallest of stored, fixed and dynamic Huffman codes for
 * every block.
 *
 * @param src           Data to compress
 * @param src_size      Size of the data (up to ~4 GB)
 * @param dst           Output buffer
 * @param dst_capacity  Size of the output buffer (dmcomp_deflate_bound() is always enough)
 * @param out_size      Receives the size of the compressed data (may not be NULL)
 * @param level         0: stored, 1 (fastest) .. 9 (smallest); out of range: DMCOMP_LEVEL_DEFAULT (negative) or 9
 *
 * @return DMCOMP_OK or an error (DMCOMP_ERR_OVERFLOW, DMCOMP_ERR_MEMORY, DMCOMP_ERR_PARAM)
 */
int dmcomp_deflate( const void* src, size_t src_size, void* dst, size_t dst_capacity, size_t* out_size, int level );

#endif /* DMCOMP_NO_ENCODER */

#ifdef __cplusplus
}
#endif

#endif /* DMCOMP_H */
