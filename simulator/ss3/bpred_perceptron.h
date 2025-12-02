/* bpred_perceptron.h - perceptron branch predictor definitions */

#ifndef BPRED_PERCEPTRON_H
#define BPRED_PERCEPTRON_H

#include "host.h"
#include "machine.h"

/* Maximum limits for perceptron predictor */
#define MAX_PERC 8192      /* Maximum number of perceptrons */
#define MAX_HIST 256       /* Maximum history length */

/* Perceptron predictor configuration structure */
struct bpred_perceptron_t {
  int weight_i;            /* Number of perceptrons in table */
  int weight_bits;         /* Bits per weight (for max_weight calculation) */
  int history;             /* History length (number of previous branches) */
  int max_weight;          /* Maximum weight value (calculated from weight_bits) */
  
  /* State maintained during prediction */
  int lookup_out;          /* Output of most recent lookup (y value) */
  int i;                   /* Index of perceptron used in most recent lookup */
  
  /* Perceptron weights table: weight_table[perceptron_index][weight_index]
   * weight_index 0 = bias weight (always multiplied by +1)
   * weight_index 1..history = history weights
   */
  int weight_table[MAX_PERC][MAX_HIST + 1];
  
  /* Global branch history: mask_table[0] = bias (always +1)
   * mask_table[1..history] = actual history bits (±1)
   * Newest history is at index 1, oldest at index history
   */
  int mask_table[MAX_HIST + 1];
};

/* Function prototypes */

/* Create and initialize a perceptron predictor
 * Parameters:
 *   num_perceptrons - number of perceptrons in the table
 *   weight_bits - number of bits per weight (determines max_weight)
 *   history_length - number of previous branches to consider
 * Returns:
 *   pointer to initialized perceptron predictor structure
 */
struct bpred_perceptron_t *
bpred_perceptron_create(unsigned int num_perceptrons,
                        unsigned int weight_bits,
                        unsigned int history_length);

/* Print perceptron predictor configuration
 * Parameters:
 *   pred_perc - perceptron predictor instance
 *   name - name to display in output
 *   stream - output stream (typically stdout or stderr)
 */
void
bpred_perceptron_config(struct bpred_perceptron_t *pred_perc,
                        char name[],
                        FILE *stream);

/* Look up a branch prediction
 * Parameters:
 *   pred_perc - perceptron predictor instance
 *   baddr - branch address (used for hashing to select perceptron)
 * Returns:
 *   pointer to selected perceptron's bias weight (for interface compatibility)
 *   The actual prediction is based on the sign of pred_perc->lookup_out:
 *     negative = not taken, non-negative = taken
 */
char *
bpred_perceptron_lookup(struct bpred_perceptron_t *pred_perc,
                        md_addr_t baddr);

/* Update the perceptron predictor after branch resolution
 * Parameters:
 *   pred_perc - perceptron predictor instance
 *   baddr - branch address (used for hashing to select perceptron)
 *   taken - actual branch outcome (non-zero if taken)
 */
void
bpred_perceptron_update(struct bpred_perceptron_t *pred_perc,
                        md_addr_t baddr,
                        int taken);

#endif /* BPRED_PERCEPTRON_H */