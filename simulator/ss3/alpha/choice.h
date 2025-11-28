#ifndef CHOICE_H
#define CHOICE_H

// Includes
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "machine.h"

// Choice (chooser / meta) predictor
typedef struct selector_t {
    uint32_t entries;         // number of chooser entries (power of two)
    uint32_t mask;            // mask for index
    unsigned char *ctr;       // 2-bit counters
} selector_t;

// Create / reset / clear
selector_t *choice_create(uint32_t entries);
void choice_reset(selector_t *s);
void choice_clear(selector_t *s);

// Lookup / counter accessor
int choice_lookup(selector_t *s, uint32_t idx);
unsigned char *choice_get_counter(selector_t *s, uint32_t idx);

#endif // CHOICE_H
