// Alpha top-level (Tournament) predictor glue
// This file composes local, global, choice, and BTB modules and implements
// the SimpleScalar-compatible lookup/update interface.

#include "alpha.h"

// Create Alpha predictor with sensible defaults
alpha_t *alpha_create(void) {
    alpha_t *a = (alpha_t*)calloc(1, sizeof(alpha_t));
    if (!a) fatal("alpha: alloc failed");

    // Local: 10-bit histories, 4k LHT entries
    a->local = local_create(10, 4096);

    // Global: 12-bit GHR
    a->global = global_create(12);

    // Choice: 4k entries (indexed by GHR)
    a->choice = choice_create(1u << 12);

    // BTB: 512 sets x 2-way (same defaults used earlier)
    a->btb = btb_create(512, 2);

    return a;
}

// Reset
void alpha_reset(alpha_t *a) {
    if (!a) return;
    local_reset(a->local);
    global_reset(a->global);
    choice_reset(a->choice);
    btb_reset(a->btb);
}

// Clear
void alpha_clear(alpha_t *a) {
    if (!a) return;
    btb_clear(a->btb);
    choice_clear(a->choice);
    global_clear(a->global);
    local_clear(a->local);
    free(a);
}

// SimpleScalar-compatible lookup
md_addr_t alpha_lookup(alpha_t *a,
                       md_addr_t baddr,
                       md_addr_t btarget,
                       enum md_opcode op,
                       int is_call,
                       int is_return,
                       struct bpred_update_t *u,
                       int *stack_recover_idx)
{
    // Validate args
    if (!a || !u) panic("alpha_lookup: null arguments");

    // If not control op, no prediction (return 0)
    if (!(MD_OP_FLAGS(op) & F_CTRL)) return 0;

    // Clear dir_update_t pointers
    u->pdir = NULL;
    u->pdir1 = NULL;
    u->pdir2 = NULL;
    u->pmeta = NULL;
    u->dir.ras = FALSE;

    // Compute local, global predictions and pointers
    int local_pred = local_lookup(a->local, baddr);
    unsigned char *local_ctr = local_get_counter(a->local, baddr);

    int global_pred = global_lookup(a->global);
    unsigned char *global_ctr = global_get_counter(a->global);

    // Chooser index uses GHR
    uint32_t ghr = global_get_ghr(a->global);
    unsigned char *choice_ctr = choice_get_counter(a->choice, ghr);

    // Choose which predictor to use
    int choose_global = ((*choice_ctr) >= 2);
    int final_pred = choose_global ? global_pred : local_pred;
    unsigned char *winner_ctr = choose_global ? global_ctr : local_ctr;

    // Fill update pointers so SimpleScalar will update counters
    // Note: bpred_update_t usually expects char*, cast accordingly
    u->pdir1 = (char *)global_ctr;
    u->pdir2 = (char *)local_ctr;
    u->pmeta = (char *)choice_ctr;
    u->pdir  = (char *)winner_ctr;

    // Populate dir flags used in some SimpleScalar versions
    u->dir.bimod = (*local_ctr >= 2);
    u->dir.twolev = (*global_ctr >= 2);
    u->dir.meta = (*choice_ctr >= 2);

    // Determine predicted target using BTB
    if (final_pred) {
        md_addr_t t = btb_lookup(a->btb, baddr);
        if (t) return t;         // BTB hit -> return target
        return 1;                // BTB miss but predicted taken -> non-zero
    } else {
        return 0;                // predicted not taken -> zero
    }
}

// alpha_update: update histories and BTB only (counters updated by SimpleScalar)
void alpha_update(alpha_t *a,
                  md_addr_t baddr,
                  md_addr_t btarget,
                  int taken,
                  int pred_taken,
                  int correct,
                  enum md_opcode op,
                  struct bpred_update_t *u)
{
    if (!a) return;

    // Update BTB on taken branches
    if (taken)
        btb_update(a->btb, baddr, btarget);

    // Update local history
    local_update_history(a->local, baddr, taken);

    // Update global history
    global_update_history(a->global, taken);

    // No explicit choice counter update here — SimpleScalar updates pmeta.
}
