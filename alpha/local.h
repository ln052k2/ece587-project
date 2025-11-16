#ifndef LOCAL_H
#define LOCAL_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// 2-bit counter states
#define COUNTER_STRONGLY_NOT_TAKEN 0
#define COUNTER_WEAKLY_NOT_TAKEN   1
#define COUNTER_WEAKLY_TAKEN       2
#define COUNTER_STRONGLY_TAKEN     3

// Local predictor structure
typedef struct {
    uint32_t history_bits;      // Number of bits per LHT entry
    uint32_t lht_size;          // Number of local history entries
    uint32_t lpht_size;         // Number of LPHT counters
    uint16_t *lht;              // Local history table
    uint8_t  *lpht;             // Pattern table of 2-bit counters
} local_pred_t;

// Functions
local_pred_t *local_create(uint32_t history_bits, uint32_t lht_entries);
void local_reset(local_pred_t *lp);
void local_clear(local_pred_t *lp);

uint8_t local_predict(local_pred_t *lp, uint32_t pc);
void local_update(local_pred_t *lp, uint32_t pc, uint8_t taken);

#endif
