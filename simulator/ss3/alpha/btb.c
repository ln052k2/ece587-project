// Simple set-associative BTB implementation

#include "btb.h"

// Helper: safe calloc
static void *xcalloc_btb(size_t n, size_t s) {
    void *p = calloc(n, s);
    if (!p) fatal("btb: out of memory");
    return p;
}

// Create BTB
btb_t *btb_create(uint32_t sets, uint32_t assoc) {
    btb_t *b = (btb_t*)xcalloc_btb(1, sizeof(btb_t));
    b->sets = sets;
    b->assoc = assoc;
    b->entries = (btb_entry_t*)xcalloc_btb(sets * assoc, sizeof(btb_entry_t));
    return b;
}

// Reset BTB
void btb_reset(btb_t *b) {
    if (!b) return;
    memset(b->entries, 0, b->sets * b->assoc * sizeof(btb_entry_t));
}

// Clear BTB
void btb_clear(btb_t *b) {
    if (!b) return;
    free(b->entries);
    free(b);
}

// Compute set index
static inline uint32_t btb_set_index(btb_t *b, md_addr_t pc) {
    return (pc >> MD_BR_SHIFT) & (b->sets - 1);
}

// Lookup BTB, return target or 0
md_addr_t btb_lookup(btb_t *b, md_addr_t pc) {
    uint32_t set = btb_set_index(b, pc);
    uint32_t base = set * b->assoc;
    for (uint32_t i = 0; i < b->assoc; ++i) {
        btb_entry_t *e = &b->entries[base + i];
        if (e->valid && e->tag == (pc >> MD_BR_SHIFT))
            return e->target;
    }
    return 0;
}

// Update BTB: insert or update entry
void btb_update(btb_t *b, md_addr_t pc, md_addr_t target) {
    uint32_t set = btb_set_index(b, pc);
    uint32_t base = set * b->assoc;
    uint64_t tag = (pc >> MD_BR_SHIFT);

    // Search for existing entry
    for (uint32_t i = 0; i < b->assoc; ++i) {
        btb_entry_t *e = &b->entries[base + i];
        if (e->valid && e->tag == tag) {
            e->target = target;
            return;
        }
    }
    // Find invalid slot
    for (uint32_t i = 0; i < b->assoc; ++i) {
        btb_entry_t *e = &b->entries[base + i];
        if (!e->valid) {
            e->valid = 1;
            e->tag = tag;
            e->target = target;
            return;
        }
    }
    // Replace first (simple policy)
    btb_entry_t *e = &b->entries[base];
    e->valid = 1;
    e->tag = tag;
    e->target = target;
}
