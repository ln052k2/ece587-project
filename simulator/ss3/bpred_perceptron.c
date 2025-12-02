/* bpred_perceptron.c - perceptron branch predictor implementation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host.h"
#include "misc.h"
#include "machine.h"
#include "bpred_perceptron.h"

/* Create and initialize a perceptron predictor */
struct bpred_perceptron_t *
bpred_perceptron_create(unsigned int num_perceptrons,
                        unsigned int weight_bits,
                        unsigned int history_length)
{
  struct bpred_perceptron_t *pred_perc;
  int i, h;

  /* Validate parameters */
  if (!num_perceptrons)
    fatal("perceptron: number of perceptrons `%d' must be positive", num_perceptrons);
  if (!history_length)
    fatal("perceptron: history length `%d' must be positive", history_length);

  /* Clamp to internal array limits */
  if (num_perceptrons > MAX_PERC) {
    fprintf(stderr, "Warning: perceptron count %d exceeds MAX_PERC (%d), clamping\n",
            num_perceptrons, MAX_PERC);
    num_perceptrons = MAX_PERC;
  }
  if (history_length > MAX_HIST) {
    fprintf(stderr, "Warning: history length %d exceeds MAX_HIST (%d), clamping\n",
            history_length, MAX_HIST);
    history_length = MAX_HIST;
  }

  /* Allocate predictor structure */
  if (!(pred_perc = calloc(1, sizeof(struct bpred_perceptron_t))))
    fatal("out of virtual memory");

  /* Initialize configuration */
  pred_perc->weight_i = num_perceptrons;
  pred_perc->weight_bits = weight_bits;
  pred_perc->history = history_length;
  pred_perc->lookup_out = 0;
  pred_perc->i = 0;

  /* Calculate maximum weight value based on weight_bits
   * For signed integers with n bits: range is -(2^(n-1)) to +(2^(n-1) - 1)
   */
  pred_perc->max_weight = (1 << (pred_perc->weight_bits - 1)) - 1;

  /* Initialize global branch history register
   * Index 0: bias (always +1, never changes)
   * Indices 1 to history: actual branch history (initialized to +1 = taken)
   */
  pred_perc->mask_table[0] = 1;  /* Bias is always +1 */
  for (h = 1; h <= pred_perc->history; h++)
    pred_perc->mask_table[h] = 1;  /* Initialize history to taken */

  /* Weight table is already zeroed by calloc */
  /* This gives all perceptrons a neutral starting point */

  return pred_perc;
}

/* Print perceptron predictor configuration */
void
bpred_perceptron_config(struct bpred_perceptron_t *pred_perc,
                        char name[],
                        FILE *stream)
{
  fprintf(stream,
          "pred_dir: %s: perceptron: %d entries, %d weight_bits, history=%d\n",
          name,
          pred_perc->weight_i,
          pred_perc->weight_bits,
          pred_perc->history);
}

/* Look up a branch prediction using the perceptron predictor */
char *
bpred_perceptron_lookup(struct bpred_perceptron_t *pred_perc,
                        md_addr_t baddr)
{
  static int lookup_count = 0;
  int idx, h;
  int sum = 0;
  int hist_len;

  /* DEBUG - first few lookups */
  if (lookup_count < 3) {
    fprintf(stderr, "LOOKUP #%d called, pred_perc=%p\n", lookup_count, (void*)pred_perc);
    lookup_count++;
  }

  /* Check for NULL */
  if (!pred_perc) {
    fprintf(stderr, "ERROR: pred_perc is NULL in lookup!\n");
    abort();
  }

  hist_len = pred_perc->history;
  
  /* DEBUG - first few lookups */
  if (lookup_count <= 3) {
    fprintf(stderr, "  baddr=0x%lx, hist_len=%d, weight_i=%d\n", 
            (unsigned long)baddr, hist_len, pred_perc->weight_i);
  }

  /* Hash the branch address to select a perceptron
   * Use multiple address bits to reduce aliasing
   */
  idx = (((baddr >> 2) ^ (baddr >> 13) ^ (baddr >> 17))
         & (pred_perc->weight_i - 1));
  
  /* Store index for use in update */
  pred_perc->i = idx;

  /* Compute perceptron output: y = w_0 + sum(w_i * x_i)
   * This is the dot product of weights and history
   */
  
  /* Add bias weight (index 0, always multiplied by +1) */
  sum = pred_perc->weight_table[idx][0];

  /* Add contributions from history bits
   * x_i is either +1 (taken) or -1 (not taken)
   * w_i is the learned weight for that history position
   */
  for (h = 1; h <= hist_len && h <= MAX_HIST; h++)
  {
    int x = pred_perc->mask_table[h];
    
    /* Convert 0 to -1 for bipolar representation (should not happen) */
    if (x == 0) x = -1;
    
    /* Accumulate: sum += w_i * x_i */
    sum += pred_perc->weight_table[idx][h] * x;
  }

  /* Store output for use in training */
  pred_perc->lookup_out = sum;

  /* Return pointer to bias weight for interface compatibility
   * The actual prediction is: taken if sum >= 0, not taken if sum < 0
   */
  return (char *)&pred_perc->weight_table[idx][0];
}

/* Update the perceptron predictor after branch resolution */
void
bpred_perceptron_update(struct bpred_perceptron_t *pred_perc,
                        md_addr_t baddr,
                        int taken)
{
  int idx;
  int t;           /* Actual outcome: +1 if taken, -1 if not taken */
  int y;           /* Perceptron output from lookup */
  int abs_y;       /* Absolute value of y */
  int theta;       /* Training threshold */
  int hist;        /* History length */
  int MAXW;        /* Maximum weight value */
  int i;

  /* Get predictor parameters */
  hist = pred_perc->history;
  MAXW = pred_perc->max_weight;
  
  /* Clamp history to maximum */
  if (hist > MAX_HIST)
    hist = MAX_HIST;

  /* Compute perceptron index (same hash as lookup) */
  idx = (((baddr >> 2) ^ (baddr >> 13) ^ (baddr >> 17))
         & (pred_perc->weight_i - 1));

  /* Get actual outcome and prediction */
  t = taken ? +1 : -1;
  y = pred_perc->lookup_out;
  abs_y = (y < 0) ? -y : y;

  /* Calculate training threshold using formula from paper:
   * theta = floor(1.93 * history_length) + 14
   * Extra stabilization for large histories
   */
  theta = (int)(1.93 * hist) + 14;
  if (hist > 32)
    theta += hist / 4;

  /* Training algorithm from paper (Section 3.3):
   * Train if prediction was wrong OR confidence was low
   * if (sign(y) != t) OR (|y| <= theta) then
   *     for each weight w_i:
   *         w_i := w_i + t * x_i
   */
  if ((t * y) <= 0 || abs_y <= theta)
  {
    /* Update bias weight (w_0, always multiplied by x_0 = +1) */
    int w0 = pred_perc->weight_table[idx][0];
    w0 += t;  /* Equivalent to: w0 += t * 1 */
    
    /* Clamp to prevent overflow */
    if (w0 > MAXW)  w0 = MAXW;
    if (w0 < -MAXW) w0 = -MAXW;
    
    pred_perc->weight_table[idx][0] = w0;

    /* Update history weights (w_1 through w_history) */
    for (i = 1; i <= hist && i <= MAX_HIST; i++)
    {
      int x = pred_perc->mask_table[i];
      int w = pred_perc->weight_table[idx][i];
      
      /* Convert 0 to -1 for bipolar representation (should not happen) */
      if (x == 0) x = -1;
      
      /* Perceptron learning rule: w_i := w_i + t * x_i
       * This can also be written as:
       * - If t and x have same sign (agree): increment weight
       * - If t and x have different sign (disagree): decrement weight
       */
      w += t * x;
      
      /* Clamp to prevent overflow */
      if (w > MAXW)  w = MAXW;
      if (w < -MAXW) w = -MAXW;
      
      pred_perc->weight_table[idx][i] = w;
    }
  }

  /* Update global branch history register
   * Shift history: newest goes to position 1, older bits shift toward end
   * Position 0 remains as bias (+1) and never changes
   */
  for (i = hist; i > 1 && i <= MAX_HIST; i--)
    pred_perc->mask_table[i] = pred_perc->mask_table[i - 1];

  /* Insert newest outcome at position 1 */
  pred_perc->mask_table[1] = taken ? +1 : -1;
}