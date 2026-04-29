/*****************************************************
 * ROCK POOL RUNTIME
 * Two-pool allocator implementing ADR-0003.
 *  - bump:      stack-disciplined save/restore per scope.
 *  - longlived: refcount-managed, size-class freelists,
 *               reclaim() coalescing.
 *****************************************************/

#ifndef ROCK_POOLS_H
#define ROCK_POOLS_H

#include <stddef.h>
#include <stdint.h>

/* Universal block header for refcount-managed allocations.
 * Sits immediately before the payload. Per ADR-0003 §8.5. */
typedef struct rock_block_header {
  uint16_t size;     /* payload size in bytes (excluding this header) */
  uint16_t refcount; /* ROCK_RC_STATIC = static; ROCK_RC_FREE = free; else live count */
} rock_block_header;

#define ROCK_RC_FREE    0xFFFEu
#define ROCK_RC_STATIC  0xFFFFu

/* Initialise the pools. Must be called before any other pool API.
 * On allocation failure to obtain backing memory, prints diagnostic and exits. */
void rock_pools_init(size_t bump_capacity, size_t longlived_capacity);

/* Tear down the pools. Frees backing memory. */
void rock_pools_deinit(void);

/* ---- Bump pool ---- */

/* Allocate `bytes` raw bytes from the bump pool. On OOM, invokes the OOM
 * handler (which by default prints a diagnostic and exits). */
void *rock_bump_alloc(size_t bytes);

/* Bump save/restore for stack-disciplined region scopes. */
typedef size_t rock_bump_mark;

rock_bump_mark rock_bump_save(void);
void rock_bump_restore(rock_bump_mark mark);

/* ---- Longlived pool ---- */

/* Allocate a refcount-managed block. Returns a pointer to the payload.
 * The block header is at `((rock_block_header *)payload) - 1` with
 * size set to the rounded-up class size and refcount = 1. */
void *rock_longlived_alloc(size_t payload_size);

/* Free a longlived block. Marks it free and pushes onto the matching
 * size-class freelist. Static blocks (refcount == ROCK_RC_STATIC) are
 * silently ignored. Double-free is a fatal diagnostic. */
void rock_longlived_free(void *payload);

/* Coalesce adjacent free blocks. Linear scan from base to high-water mark.
 * O(N * F) where N is the block count and F is the freelist length per
 * removal — acceptable for occasional reclaim() calls on a bounded pool. */
void rock_longlived_reclaim(void);

/* ---- Diagnostics ---- */

size_t rock_bump_used(void);
size_t rock_bump_capacity(void);
size_t rock_longlived_used(void);
size_t rock_longlived_capacity(void);
size_t rock_longlived_free_bytes(void);

/* OOM handler signature. Default implementation prints a diagnostic to
 * stderr and exits(1). Tests may install a non-exiting handler. */
typedef void (*rock_oom_handler_fn)(const char *pool_name,
                                     size_t requested,
                                     size_t available);
void rock_set_oom_handler(rock_oom_handler_fn handler);

#endif /* ROCK_POOLS_H */
