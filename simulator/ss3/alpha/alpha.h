#ifndef ALPHA_H
#define ALPHA_H

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "host.h"
#include "machine.h"
#include "bpred.h"

#include "local.h"
#include "global.h"
#include "choice.h"
#include "btb.h"

// Alpha top-level type
typedef struct alpha_t {
    local_pred_t  *local;
    global_pred_t *global;
    selector_t    *choice;
    btb_t         *btb;
} alpha_t;

// Create / reset / clear
alpha_t *alpha_create(void);
void alpha_reset(alpha_t *a);
void alpha_clear(alpha_t *a);

// SimpleScalar-style lookup: MUST fill dir_update_ptr pointers
md_addr_t alpha_lookup(alpha_t *a,
                       md_addr_t baddr,
                       md_addr_t btarget,
                       enum md_opcode op,
                       int is_call,
                       int is_return,
                       struct bpred_update_t *dir_update_ptr,
                       int *stack_recover_idx);

// Update histories and BTB (do NOT change counters here)
void alpha_update(alpha_t *a,
                  md_addr_t baddr,
                  md_addr_t btarget,
                  int taken,
                  int pred_taken,
                  int correct,
                  enum md_opcode op,
                  struct bpred_update_t *dir_update_ptr);

#endif // ALPHA_H
