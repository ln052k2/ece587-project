#include "global.h"


// Creates a global predictor
global_pred_t *global_create(uint32_t history_bits) {
    global_pred_t *gp = (global_pred_t*)malloc(sizeof(global_pred_t));
    if (!gp) return NULL;

    gp->history_bits = history_bits;
    gp->history = 0;

    // Allocate the pattern history table
    gp->pht_size = 1u << history_bits;
    gp->pht = (uint8_t*)malloc(gp->pht_size * sizeof(uint8_t));

    // Initialize counters to weakly taken
    for (uint32_t i = 0; i < gp->pht_size; i++)
        gp->pht[i] = COUNTER_WEAKLY_TAKEN;

    return gp;
}

// Predict taken/not-taken using global history
uint8_t global_predict(global_pred_t *gp, uint32_t pc) {
    uint32_t idx = global_index(gp, pc);
    uint8_t counter = gp->pht[idx];

    // Return 1 for taken, 0 for not taken
    return (counter >= COUNTER_WEAKLY_TAKEN);
}

// Update predictor based on actual outcome
void global_update(global_pred_t *gp, uint32_t pc, uint8_t taken) {
    uint32_t idx = global_index(gp, pc);

    // Update saturating counter
    if (taken)
        gp->pht[idx] = counter_inc(gp->pht[idx]);
    else
        gp->pht[idx] = counter_dec(gp->pht[idx]);

    // Shift in the new history bit
    gp->history = ((gp->history << 1) | (taken & 1));

    // Mask to maintain correct history width
    uint32_t mask = (1u << gp->history_bits) - 1;
    gp->history &= mask;
}


// Reset global predictor state
void global_reset(global_pred_t *gp) {
    if (!gp) return;

    gp->history = 0;

    // Reset all counters to weakly taken
    for (uint32_t i = 0; i < gp->pht_size; i++)
        gp->pht[i] = COUNTER_WEAKLY_TAKEN;
}

// Free global predictor memory
void global_clear(global_pred_t *gp) {
    if (!gp) return;
    free(gp->pht);
    free(gp);
}

/***********************************************************
* HELPER FUNCTIONS
*************************************************************/

// Compute index into PHT based on global history 
static inline uint32_t global_index(global_pred_t *gp, uint32_t pc) {
    uint32_t mask = gp->pht_size - 1;
    return gp->history & mask;
}

// Increment 2-bit saturating counter
static inline uint8_t counter_inc(uint8_t c) {
    if (c < COUNTER_STRONGLY_TAKEN) c++;
    return c;
}

// Decrement 2-bit saturating counter
static inline uint8_t counter_dec(uint8_t c) {
    if (c > COUNTER_STRONGLY_NOT_TAKEN) c--;
    return c;
}