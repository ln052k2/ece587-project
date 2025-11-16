#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// 2-bit counter defines
#define COUNTER_STRONGLY_NOT_TAKEN 0
#define COUNTER_WEAKLY_NOT_TAKEN   1
#define COUNTER_WEAKLY_TAKEN       2
#define COUNTER_STRONGLY_TAKEN     3

// Structure for global predictor
typedef struct {
    uint32_t history;          // global history register
    uint32_t history_bits;     // number of bits used in history
    uint32_t pht_size;         // number of entries in PHT
    uint8_t *pht;              // array of 2-bit saturating counters
} global_pred_t;

// Helper functions
global_pred_t *global_create(uint32_t history_bits);
void global_clear(global_pred_t *gp);
void global_destroy(global_pred_t *gp);

// Interface with top level tournament predictor
uint8_t global_predict(global_pred_t *gp, uint32_t pc);
void global_update(global_pred_t *gp, uint32_t pc, uint8_t taken);

#endif
