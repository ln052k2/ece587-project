#ifndef PREDICTOR_CHOICE_H
#define PREDICTOR_CHOICE_H

#include <stdlib.h>
#include <stdio.h>

typedef struct {
    int entries;        // number of selector entries
    unsigned char *ctr; // 2 bit counters
} choice_pred_t;

// Create selector
choice_pred_t *choice_pred_create(int entries);

// Lookup selector choice
int choice_pred_lookup(choice_pred_t *p);

// Update selector
void choice_pred_update(choice_pred_t *p,
                        int local_pred,
                        int global_pred,
                        int actual_outcome);

// Reset selector state
void choice_pred_reset(choice_pred_t *p);

// Free selector memory
void choice_pred_clear(choice_pred_t *p);

#endif
