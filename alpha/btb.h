#ifndef BTB_H
#define BTB_H

// Includes
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "machine.h"

// Simple BTB entry
typedef struct btb_entry_t {
    md_addr_t tag;
    md_addr_t target;
    unsigned valid;
} btb_entry_t;

// BTB type (set-associative)
typedef struct btb_t {
    uint32_t sets;
    uint32_t assoc;
    btb_entry_t *entries;   // size = sets * assoc
} btb_t;

// Create / reset / clear
btb_t *btb_create(uint32_t sets, uint32_t assoc);
void btb_reset(btb_t *b);
void btb_clear(btb_t *b);

// Lookup / update
md_addr_t btb_lookup(btb_t *b, md_addr_t pc);
void btb_update(btb_t *b, md_addr_t pc, md_addr_t target);

#endif // BTB_H
