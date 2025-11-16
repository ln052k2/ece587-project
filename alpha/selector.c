/*
 * @file: selector.c
 * 
 * @description: The choice predictor is a 2-bit saturating counter table (PHT) that chooses between
 * the local predictor and global predictor
 *      - 00 or 01 -> choose LOCAL predictor
 *      - 10 or 11 -> choose GLOBAL predictor
 * It only updates when the local and global predictors DISAGREE, and de/increments based on
 * whether the local or global predictor are correct, respectively.
 */

#include "predictor_choice.h"

choice_pred_t *choice_pred_create(int entries)
{
    choice_pred_t *p = calloc(1, sizeof(choice_pred_t));
    if (!p) { fprintf(stderr, "choice predictor alloc failed\n"); exit(1); }

    p->entries = entries;
    p->ctr = calloc(entries, sizeof(unsigned char));

    // Initialize to 01 (weakly local)
    for (int i = 0; i < entries; i++)
        p->ctr[i] = 1;

    return p;
}

// Lookup function
int choice_pred_lookup(choice_pred_t *p)
{
    // Single-entry selector
    unsigned char c = p->ctr[0];
    return (c >= 2) ? 1 : 0;
}

// Updates based on the outcomes of sub-predictors
void choice_pred_update(choice_pred_t *p,
                        int local_pred,
                        int global_pred,
                        int actual_outcome)
{
    int idx = 0;

    // No update if predictors agree
    if (local_pred == global_pred)
        return;

    unsigned char c = p->ctr[idx];

    // Increments if global is correct
    // Decrements if local is correct
    if (global_pred == actual_outcome) {
        if (c < 3) c++;
    } else if (local_pred == actual_outcome) {
        if (c > 0) c--;
    }

    p->ctr[idx] = c;
}

// Resets to initial state
void choice_pred_reset(choice_pred_t *p)
{
    for (int i = 0; i < p->entries; i++)
        p->ctr[i] = 1;
}

// Free choice predictor
void choice_pred_clear(choice_pred_t *p)
{
    if (!p) return;

    free(p->ctr);
    free(p);
}