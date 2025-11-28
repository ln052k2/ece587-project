#ifndef LOCAL_H
#define LOCAL_H

// Includes
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "machine.h"

// Local predictor type
typedef struct local_pred_t {
    uint32_t history_bits;   // bits per local history entry (k)
    uint32_t lht_entries;    // number of local history table entries (power of two)
    uint32_t lpht_entries;   // number of local PHT entries (power of two)
    uint32_t lht_mask;       // mask for LHT index
    uint32_t lpht_mask;      // mask for LPHT index

    // Storage
    uint32_t *lht;           // local histories (store low bits)
    unsigned char *lpht;     // local pattern history table (2-bit counters)
} local_pred_t;

// Create / reset / clear
local_pred_t *local_create(uint32_t history_bits, uint32_t lht_entries);
void local_reset(local_pred_t *lp);
void local_clear(local_pred_t *lp);

// Lookup / history-update / counter pointer
int local_lookup(local_pred_t *lp, md_addr_t pc);
unsigned char *local_get_counter(local_pred_t *lp, md_addr_t pc);
void local_update_history(local_pred_t *lp, md_addr_t pc, int taken);

#endif // LOCAL_H
