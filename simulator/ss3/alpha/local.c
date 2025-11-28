// Local predictor: LHT indexed by PC -> LPHT (2-bit counters)

#include "local.h"

// Helper: safe calloc
static void *xcalloc_local(size_t n, size_t s) {
    void *p = calloc(n, s);
    if (!p) fatal("local: out of memory");
    return p;
}

// Create local predictor
local_pred_t *local_create(uint32_t history_bits, uint32_t lht_entries) {
    local_pred_t *lp = (local_pred_t*)xcalloc_local(1, sizeof(local_pred_t));
    lp->history_bits = history_bits;
    lp->lht_entries = lht_entries;
    lp->lpht_entries = 1u << history_bits;
    lp->lht_mask = lht_entries - 1;
    lp->lpht_mask = lp->lpht_entries - 1;

    lp->lht = (uint32_t*)xcalloc_local(lp->lht_entries, sizeof(uint32_t));
    lp->lpht = (unsigned char*)xcalloc_local(lp->lpht_entries, sizeof(unsigned char));

    // Initialize LPHT counters to weakly not-taken (1)
    for (uint32_t i = 0; i < lp->lpht_entries; ++i) lp->lpht[i] = 1;
    return lp;
}

// Reset local predictor
void local_reset(local_pred_t *lp) {
    if (!lp) return;
    memset(lp->lht, 0, lp->lht_entries * sizeof(uint32_t));
    for (uint32_t i = 0; i < lp->lpht_entries; ++i) lp->lpht[i] = 1;
}

// Clear local predictor
void local_clear(local_pred_t *lp) {
    if (!lp) return;
    free(lp->lht);
    free(lp->lpht);
    free(lp);
}

// Compute LHT index from PC
static inline uint32_t local_lht_index(local_pred_t *lp, md_addr_t pc) {
    return (pc >> MD_BR_SHIFT) & lp->lht_mask;
}

// Get LPHT index (from local history)
static inline uint32_t local_lpht_index(local_pred_t *lp, uint32_t local_hist) {
    return local_hist & lp->lpht_mask;
}

// Lookup local prediction (0/1)
int local_lookup(local_pred_t *lp, md_addr_t pc) {
    uint32_t li = local_lht_index(lp, pc);
    uint32_t hist = lp->lht[li] & ((1u << lp->history_bits) - 1);
    unsigned char ctr = lp->lpht[local_lpht_index(lp, hist)];
    return (ctr >= 2) ? 1 : 0;
}

// Return pointer to the underlying 2-bit counter (0..3)
unsigned char *local_get_counter(local_pred_t *lp, md_addr_t pc) {
    uint32_t li = local_lht_index(lp, pc);
    uint32_t hist = lp->lht[li] & ((1u << lp->history_bits) - 1);
    uint32_t idx = local_lpht_index(lp, hist);
    return &lp->lpht[idx];
}

// Update local history only (do not change counters)
void local_update_history(local_pred_t *lp, md_addr_t pc, int taken) {
    uint32_t li = local_lht_index(lp, pc);
    uint32_t mask = (1u << lp->history_bits) - 1;
    lp->lht[li] = ((lp->lht[li] << 1) | (taken ? 1u : 0u)) & mask;
}
