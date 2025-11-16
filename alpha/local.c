#include "local.h"

// Creates the local branch predictor
local_pred_t *local_create(uint32_t history_bits, uint32_t lht_entries) {
    local_pred_t *lp = (local_pred_t*)malloc(sizeof(local_pred_t));
    if (!lp) return NULL;

    lp->history_bits = history_bits;
    lp->lht_size = lht_entries;

    // Allocate Local History Table (LHT)
    lp->lht = (uint16_t*)malloc(lht_entries * sizeof(uint16_t));
    memset(lp->lht, 0, lht_entries * sizeof(uint16_t));

    // Allocate Local Pattern History Table (LPHT)
    lp->lpht_size = 1u << history_bits;
    lp->lpht = (uint8_t*)malloc(lp->lpht_size * sizeof(uint8_t));

    // Initialize LPHT to weakly taken
    for (uint32_t i = 0; i < lp->lpht_size; i++)
        lp->lpht[i] = COUNTER_WEAKLY_TAKEN;

    return lp;
}

// Reset predictor contents
void local_reset(local_pred_t *lp) {
    if (!lp) return;

    // Reset LHT entries
    memset(lp->lht, 0, lp->lht_size * sizeof(uint16_t));

    // Reset LPHT counters to weakly taken
    for (uint32_t i = 0; i < lp->lpht_size; i++)
        lp->lpht[i] = COUNTER_WEAKLY_TAKEN;
}

// Clear predictor memory
void local_clear(local_pred_t *lp) {
    if (!lp) return;
    free(lp->lht);
    free(lp->lpht);
    free(lp);
}

// Predict taken / not taken
uint8_t local_predict(local_pred_t *lp, uint32_t pc) {
    uint32_t lht_idx = local_lht_index(lp, pc);
    uint32_t history = lp->lht[lht_idx];

    uint32_t lpht_idx = history & (lp->lpht_size - 1);
    uint8_t counter = lp->lpht[lpht_idx];

    // Return 1 for taken, 0 for not taken
    return (counter >= COUNTER_WEAKLY_TAKEN);
}

// Update the predictor based on actual outcome
void local_update(local_pred_t *lp, uint32_t pc, uint8_t taken) {
    uint32_t lht_idx = local_lht_index(lp, pc);
    uint32_t history = lp->lht[lht_idx];

    uint32_t lpht_idx = history & (lp->lpht_size - 1);

    // Update counter
    if (taken)
        lp->lpht[lpht_idx] = counter_inc(lp->lpht[lpht_idx]);
    else
        lp->lpht[lpht_idx] = counter_dec(lp->lpht[lpht_idx]);

    // Update local history: shift in outcome
    history = ((history << 1) | (taken & 1));

    // Mask to limit history size
    uint32_t mask = (1u << lp->history_bits) - 1;
    lp->lht[lht_idx] = (history & mask);
}

/***********************************************************
* HELPER FUNCTIONS
*************************************************************/

// Compute LHT index using PC
static inline uint32_t local_lht_index(local_pred_t *lp, uint32_t pc) {
    return (pc >> 2) & (lp->lht_size - 1);
}

// Increment 2-bit counter
static inline uint8_t counter_inc(uint8_t c) {
    if (c < COUNTER_STRONGLY_TAKEN) c++;
    return c;
}

// Decrement 2-bit counter
static inline uint8_t counter_dec(uint8_t c) {
    if (c > COUNTER_STRONGLY_NOT_TAKEN) c--;
    return c;
}