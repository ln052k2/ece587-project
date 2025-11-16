/*
 * @file: tournament.c
 *
 * @description: Top-level tournament branch predictor for SimpleScalar.
 * Implements Alpha-21264 style local/global hybrid predictor with a selector.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tournament.h"
#include "predictor_local.h"
#include "predictor_global.h"
#include "predictor_choice.h"
#include "btb.h"

// Create tournament predictor
bpred_tournament_t *bpred_tournament_create(
    int local_history_bits,
    int local_table_entries,
    int global_history_bits,
    int choice_table_entries,
    int btb_entries
) {
    // Allocate top level tournament predictor
    bpred_tournament_t *pred = (bpred_tournament_t *)calloc(1, sizeof(bpred_tournament_t));
    if (!pred) { 
        fprintf(stderr, "Error allocating tournament predictor\n");
        exit(1);
    }

    // Initialize components (sub-predictors) of the tournament predictor
    pred->local = local_pred_create(local_history_bits, local_table_entries);
    pred->global = global_pred_create(global_history_bits);
    pred->choice = choice_pred_create(choice_table_entries);

    // Create BTB
    pred->btb = btb_create(btb_entries);
    
    return pred;
}

// Predictor runction is called before executing a branch
bpred_outcome_t bpred_tournament_lookup(
    bpred_tournament_t *pred,
    md_addr_t pc
) {
    bpred_outcome_t out = {0};

    // Predict direction with local + global predictors
    int local_pred   = local_pred_lookup(pred->local, pc);
    int global_pred  = global_pred_lookup(pred->global);
    int selector     = choice_pred_lookup(pred->choice);

    // Selector chooses between the two predictions
    int final_pred = (selector ? global_pred : local_pred);
    out.direction_prediction = final_pred;

    // Get target address from BTB if branch is taken
    if (final_pred)
        out.target_prediction = btb_lookup(pred->btb, pc);
    else
        out.target_prediction = 0;

    // Save internal predictor results
    out.local_prediction = local_pred;
    out.global_prediction = global_pred;
    out.selector_choice = selector;

    return out;
}


// Update function is called after branch is resolved
void bpred_tournament_update(
    bpred_tournament_t *pred,
    md_addr_t pc,
    int taken,
    md_addr_t target,
    bpred_outcome_t outcome
) {
    if (taken) {
        btb_update(pred->btb, pc, target);
    }

    // Update each predictor component
    local_pred_update(pred->local, pc, taken);
    global_pred_update(pred->global, taken);

    // Update choice predictor with outcomes of each sub-predictor
    choice_pred_update(pred->choice,
                       outcome.local_prediction,
                       outcome.global_prediction,
                       taken);
}


// Reset predictor state(s)
void bpred_tournament_reset(bpred_tournament_t *pred)
{
    local_pred_reset(pred->local);
    global_pred_reset(pred->global);
    choice_pred_reset(pred->choice);
    btb_reset(pred->btb);
}

// Deallocate tournament components 
void bpred_tournament_clear(bpred_tournament_t *pred)
{
    if (!pred) return;

    local_pred_clear(pred->local);
    global_pred_clear(pred->global);
    choice_pred_clear(pred->choice);
    btb_clear(pred->btb);

    free(pred);
}