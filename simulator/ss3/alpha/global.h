#ifndef GLOBAL_H
#define GLOBAL_H

// Includes
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "machine.h"

// Global predictor type (GHR -> GPHT)
typedef struct global_pred_t {
    uint32_t history_bits;   // number of bits in GHR
    uint32_t gpht_entries;   // number of GPHT entries (power of two)
    uint32_t gpht_mask;      // mask for GPHT index

    uint32_t ghr;            // global history register (low bits)
    unsigned char *gpht;     // GPHT 2-bit counters
} global_pred_t;

// Create / reset / clear
global_pred_t *global_create(uint32_t history_bits);
void global_reset(global_pred_t *gp);
void global_clear(global_pred_t *gp);

// Lookup / history update / accessors
int global_lookup(global_pred_t *gp);
unsigned char *global_get_counter(global_pred_t *gp);
void global_update_history(global_pred_t *gp, int taken);
uint32_t global_get_ghr(global_pred_t *gp);

#endif // GLOBAL_H
