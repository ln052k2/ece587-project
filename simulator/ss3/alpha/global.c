// Global predictor: GHR indexed GPHT with 2-bit counters

#include "global.h"

// Helper: safe calloc
static void *xcalloc_global(size_t n, size_t s) {
    void *p = calloc(n, s);
    if (!p) fatal("global: out of memory");
    return p;
}

// Create global predictor
global_pred_t *global_create(uint32_t history_bits) {
    global_pred_t *gp = (global_pred_t*)xcalloc_global(1, sizeof(global_pred_t));
    gp->history_bits = history_bits;
    gp->gpht_entries = 1u << history_bits;
    gp->gpht_mask = gp->gpht_entries - 1;
    gp->ghr = 0;
    gp->gpht = (unsigned char*)xcalloc_global(gp->gpht_entries, sizeof(unsigned char));

    // Initialize GPHT to weakly not-taken (1)
    for (uint32_t i = 0; i < gp->gpht_entries; ++i) gp->gpht[i] = 1;
    return gp;
}

// Reset global predictor
void global_reset(global_pred_t *gp) {
    if (!gp) return;
    gp->ghr = 0;
    for (uint32_t i = 0; i < gp->gpht_entries; ++i) gp->gpht[i] = 1;
}

// Clear global predictor
void global_clear(global_pred_t *gp) {
    if (!gp) return;
    free(gp->gpht);
    free(gp);
}

// Lookup global prediction
int global_lookup(global_pred_t *gp) {
    unsigned char ctr = gp->gpht[gp->ghr & gp->gpht_mask];
    return (ctr >= 2) ? 1 : 0;
}

// Return pointer to the underlying GPHT counter
unsigned char *global_get_counter(global_pred_t *gp) {
    return &gp->gpht[gp->ghr & gp->gpht_mask];
}

// Update global history only
void global_update_history(global_pred_t *gp, int taken) {
    gp->ghr = ((gp->ghr << 1) | (taken ? 1u : 0u)) & gp->gpht_mask;
}

// Return current GHR
uint32_t global_get_ghr(global_pred_t *gp) {
    return gp->ghr;
}
