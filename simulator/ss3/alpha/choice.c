// Choice predictor (chooser): small table of 2-bit counters indexed by GHR

#include "choice.h"

// Helper: safe calloc
static void *xcalloc_choice(size_t n, size_t s) {
    void *p = calloc(n, s);
    if (!p) fatal("choice: out of memory");
    return p;
}

// Create chooser
selector_t *choice_create(uint32_t entries) {
    selector_t *s = (selector_t*)xcalloc_choice(1, sizeof(selector_t));
    s->entries = entries;
    s->mask = entries - 1;
    s->ctr = (unsigned char*)xcalloc_choice(entries, sizeof(unsigned char));
    // Initialize to weakly prefer local (01)
    for (uint32_t i = 0; i < entries; ++i) s->ctr[i] = 1;
    return s;
}

// Reset chooser
void choice_reset(selector_t *s) {
    if (!s) return;
    for (uint32_t i = 0; i < s->entries; ++i) s->ctr[i] = 1;
}

// Clear chooser
void choice_clear(selector_t *s) {
    if (!s) return;
    free(s->ctr);
    free(s);
}

// Lookup chooser decision (0=local,1=global)
int choice_lookup(selector_t *s, uint32_t idx) {
    unsigned char c = s->ctr[idx & s->mask];
    return (c >= 2) ? 1 : 0;
}

// Return pointer to chooser counter
unsigned char *choice_get_counter(selector_t *s, uint32_t idx) {
    return &s->ctr[idx & s->mask];
}
