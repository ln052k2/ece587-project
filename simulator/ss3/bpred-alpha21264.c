/* bpred-alpha21264.c - Alpha 21264 tournament branch predictor */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "bpred-alpha21264.h"

/* create an Alpha 21264 tournament predictor */
struct bpred_alpha21264_t *         /* Alpha 21264 predictor instance */
bpred_alpha21264_create(
  unsigned int local_size,          /* local history table size */
  unsigned int pred_table_size,     /* local/global predictor table size */
  unsigned int hist_width,          /* history register width */
  unsigned int choice_size)         /* choice predictor table size */
{
  struct bpred_alpha21264_t *pred;
  unsigned int i;
  int flipflop;
  
  /* allocate predictor structure */
  pred = (struct bpred_alpha21264_t *)calloc(1, sizeof(struct bpred_alpha21264_t));
  if (!pred)
    fatal("out of virtual memory");
  
  /* validate parameters */
  if (!local_size || (local_size & (local_size - 1)) != 0)
    fatal("Alpha 21264: local history table size must be non-zero and power of 2");
  
  if (!pred_table_size || (pred_table_size & (pred_table_size - 1)) != 0)
    fatal("Alpha 21264: predictor table size must be non-zero and power of 2");
  
  if (!hist_width || hist_width > 30)
    fatal("Alpha 21264: history width must be between 1 and 30 bits");
  
  if (!choice_size || (choice_size & (choice_size - 1)) != 0)
    fatal("Alpha 21264: choice table size must be non-zero and power of 2");
  
  /* initialize local predictor components */
  pred->local_size = local_size;
  pred->local_hist_width = hist_width;
  pred->local_pred_size = pred_table_size;
  
  /* allocate local history table */
  pred->local_hist = (unsigned int *)calloc(local_size, sizeof(unsigned int));
  if (!pred->local_hist)
    fatal("out of virtual memory");
  
  /* allocate local predictor table */
  pred->local_pred = (unsigned char *)calloc(pred_table_size, sizeof(unsigned char));
  if (!pred->local_pred)
    fatal("out of virtual memory");
  
  /* initialize global predictor components */
  pred->global_hist_width = hist_width;
  pred->global_hist = 0;
  pred->global_pred_size = pred_table_size;
  
  /* allocate global predictor table */
  pred->global_pred = (unsigned char *)calloc(pred_table_size, sizeof(unsigned char));
  if (!pred->global_pred)
    fatal("out of virtual memory");
  
  /* initialize choice predictor components */
  pred->choice_size = choice_size;
  
  /* allocate choice predictor table */
  pred->choice = (unsigned char *)calloc(choice_size, sizeof(unsigned char));
  if (!pred->choice)
    fatal("out of virtual memory");
  
  /* initialize all predictor counters to weakly predict one way or the other */
  flipflop = 1;
  for (i = 0; i < pred_table_size; i++)
  {
    pred->local_pred[i] = flipflop;
    pred->global_pred[i] = flipflop;
    flipflop = 3 - flipflop;  /* alternate between 1 and 2 */
  }
  
  /* initialize choice predictor to weakly prefer global (2) or local (1) */
  flipflop = 2;
  for (i = 0; i < choice_size; i++)
  {
    pred->choice[i] = flipflop;
    flipflop = 3 - flipflop;  /* alternate between 2 and 1 */
  }
  
  return pred;
}

/* probe the Alpha 21264 predictor for a prediction */
unsigned char *                     /* pointer to pred counter */
bpred_alpha21264_lookup(
  struct bpred_alpha21264_t *pred,  /* predictor instance */
  md_addr_t baddr)                  /* branch address */
{
  unsigned int local_idx;
  unsigned int local_hist_val;
  unsigned int local_pred_idx;
  unsigned int global_pred_idx;
  unsigned int choice_idx;
  unsigned char choice_pred;
  
  if (!pred)
    panic("Alpha 21264 predictor is NULL");
  
  /* compute local history table index from branch address */
  local_idx = (baddr >> MD_BR_SHIFT) & (pred->local_size - 1);
  
  /* get local history value for this branch */
  local_hist_val = pred->local_hist[local_idx];
  
  /* index into local predictor table using local history */
  local_pred_idx = local_hist_val & (pred->local_pred_size - 1);
  
  /* index into global predictor table using global history */
  global_pred_idx = pred->global_hist & (pred->global_pred_size - 1);
  
  /* index into choice predictor using global history */
  choice_idx = pred->global_hist & (pred->choice_size - 1);
  
  /* get choice predictor value */
  choice_pred = pred->choice[choice_idx];
  
  /* choice predictor selects between local and global:
   * >= 2 means use global predictor
   * < 2 means use local predictor */
  if (choice_pred >= 2)
    return &pred->global_pred[global_pred_idx];
  else
    return &pred->local_pred[local_pred_idx];
}

/* update the Alpha 21264 predictor */
void
bpred_alpha21264_update(
  struct bpred_alpha21264_t *pred,  /* predictor instance */
  md_addr_t baddr,                  /* branch address */
  int taken)                        /* actual branch outcome */
{
  unsigned int local_idx;
  unsigned int local_hist_val;
  unsigned int local_pred_idx;
  unsigned int global_pred_idx;
  unsigned int choice_idx;
  unsigned char local_pred;
  unsigned char global_pred;
  int local_correct;
  int global_correct;
  
  if (!pred)
    panic("Alpha 21264 predictor is NULL");
  
  /* compute indices (same as in lookup) */
  local_idx = (baddr >> MD_BR_SHIFT) & (pred->local_size - 1);
  local_hist_val = pred->local_hist[local_idx];
  local_pred_idx = local_hist_val & (pred->local_pred_size - 1);
  global_pred_idx = pred->global_hist & (pred->global_pred_size - 1);
  choice_idx = pred->global_hist & (pred->choice_size - 1);
  
  /* get current predictions */
  local_pred = pred->local_pred[local_pred_idx];
  global_pred = pred->global_pred[global_pred_idx];
  
  /* determine which predictor was correct */
  local_correct = ((local_pred >= 2) == !!taken);
  global_correct = ((global_pred >= 2) == !!taken);
  
  /* update local predictor (2-bit saturating counter) */
  if (taken)
  {
    if (pred->local_pred[local_pred_idx] < 3)
      pred->local_pred[local_pred_idx]++;
  }
  else
  {
    if (pred->local_pred[local_pred_idx] > 0)
      pred->local_pred[local_pred_idx]--;
  }
  
  /* update global predictor (2-bit saturating counter) */
  if (taken)
  {
    if (pred->global_pred[global_pred_idx] < 3)
      pred->global_pred[global_pred_idx]++;
  }
  else
  {
    if (pred->global_pred[global_pred_idx] > 0)
      pred->global_pred[global_pred_idx]--;
  }
  
  /* update choice predictor only if predictors disagreed */
  if (local_correct != global_correct)
  {
    if (global_correct)
    {
      /* global was correct, increment toward global (3) */
      if (pred->choice[choice_idx] < 3)
        pred->choice[choice_idx]++;
    }
    else
    {
      /* local was correct, decrement toward local (0) */
      if (pred->choice[choice_idx] > 0)
        pred->choice[choice_idx]--;
    }
  }
  
  /* update local history for this branch */
  pred->local_hist[local_idx] = 
    ((local_hist_val << 1) | (!!taken)) & 
    ((1 << pred->local_hist_width) - 1);
  
  /* update global history register */
  pred->global_hist = 
    ((pred->global_hist << 1) | (!!taken)) & 
    ((1 << pred->global_hist_width) - 1);
}

/* print Alpha 21264 predictor configuration */
void
bpred_alpha21264_config(
  struct bpred_alpha21264_t *pred,  /* predictor instance */
  char name[],                      /* predictor name */
  FILE *stream)                     /* output stream */
{
  if (!stream)
    stream = stderr;
  
  fprintf(stream,
    "pred_dir: %s: Alpha 21264 tournament predictor\n", name);
  fprintf(stream,
    "  local hist table: %d entries, %d bits/entry\n",
    pred->local_size, pred->local_hist_width);
  fprintf(stream,
    "  local pred table: %d entries, 2-bit counters\n",
    pred->local_pred_size);
  fprintf(stream,
    "  global pred table: %d entries, 2-bit counters\n",
    pred->global_pred_size);
  fprintf(stream,
    "  global history: %d bits\n",
    pred->global_hist_width);
  fprintf(stream,
    "  choice table: %d entries, 2-bit counters\n",
    pred->choice_size);
}

/* print Alpha 21264 predictor statistics */
void
bpred_alpha21264_stats(
  struct bpred_alpha21264_t *pred,  /* predictor instance */
  FILE *stream)                     /* output stream */
{
  unsigned int i;
  unsigned int local_strong_taken = 0, local_weak_taken = 0;
  unsigned int local_weak_nottaken = 0, local_strong_nottaken = 0;
  unsigned int global_strong_taken = 0, global_weak_taken = 0;
  unsigned int global_weak_nottaken = 0, global_strong_nottaken = 0;
  unsigned int choice_strong_global = 0, choice_weak_global = 0;
  unsigned int choice_weak_local = 0, choice_strong_local = 0;
  
  if (!stream)
    stream = stderr;
  
  /* count predictor counter distributions */
  for (i = 0; i < pred->local_pred_size; i++)
  {
    switch (pred->local_pred[i])
    {
      case 0: local_strong_nottaken++; break;
      case 1: local_weak_nottaken++; break;
      case 2: local_weak_taken++; break;
      case 3: local_strong_taken++; break;
    }
  }
  
  for (i = 0; i < pred->global_pred_size; i++)
  {
    switch (pred->global_pred[i])
    {
      case 0: global_strong_nottaken++; break;
      case 1: global_weak_nottaken++; break;
      case 2: global_weak_taken++; break;
      case 3: global_strong_taken++; break;
    }
  }
  
  for (i = 0; i < pred->choice_size; i++)
  {
    switch (pred->choice[i])
    {
      case 0: choice_strong_local++; break;
      case 1: choice_weak_local++; break;
      case 2: choice_weak_global++; break;
      case 3: choice_strong_global++; break;
    }
  }
  
  fprintf(stream, "\n** Alpha 21264 Predictor Statistics **\n");
  fprintf(stream, "Local Predictor Counter Distribution:\n");
  fprintf(stream, "  Strongly Not-Taken (0): %u (%.2f%%)\n",
          local_strong_nottaken, 
          100.0 * local_strong_nottaken / pred->local_pred_size);
  fprintf(stream, "  Weakly Not-Taken   (1): %u (%.2f%%)\n",
          local_weak_nottaken,
          100.0 * local_weak_nottaken / pred->local_pred_size);
  fprintf(stream, "  Weakly Taken       (2): %u (%.2f%%)\n",
          local_weak_taken,
          100.0 * local_weak_taken / pred->local_pred_size);
  fprintf(stream, "  Strongly Taken     (3): %u (%.2f%%)\n",
          local_strong_taken,
          100.0 * local_strong_taken / pred->local_pred_size);
  
  fprintf(stream, "\nGlobal Predictor Counter Distribution:\n");
  fprintf(stream, "  Strongly Not-Taken (0): %u (%.2f%%)\n",
          global_strong_nottaken,
          100.0 * global_strong_nottaken / pred->global_pred_size);
  fprintf(stream, "  Weakly Not-Taken   (1): %u (%.2f%%)\n",
          global_weak_nottaken,
          100.0 * global_weak_nottaken / pred->global_pred_size);
  fprintf(stream, "  Weakly Taken       (2): %u (%.2f%%)\n",
          global_weak_taken,
          100.0 * global_weak_taken / pred->global_pred_size);
  fprintf(stream, "  Strongly Taken     (3): %u (%.2f%%)\n",
          global_strong_taken,
          100.0 * global_strong_taken / pred->global_pred_size);
  
  fprintf(stream, "\nChoice Predictor Counter Distribution:\n");
  fprintf(stream, "  Strongly Local  (0): %u (%.2f%%)\n",
          choice_strong_local,
          100.0 * choice_strong_local / pred->choice_size);
  fprintf(stream, "  Weakly Local    (1): %u (%.2f%%)\n",
          choice_weak_local,
          100.0 * choice_weak_local / pred->choice_size);
  fprintf(stream, "  Weakly Global   (2): %u (%.2f%%)\n",
          choice_weak_global,
          100.0 * choice_weak_global / pred->choice_size);
  fprintf(stream, "  Strongly Global (3): %u (%.2f%%)\n",
          choice_strong_global,
          100.0 * choice_strong_global / pred->choice_size);
  
  fprintf(stream, "\nCurrent Global History: 0x%x\n", pred->global_hist);
}

/* free Alpha 21264 predictor resources */
void
bpred_alpha21264_free(
  struct bpred_alpha21264_t *pred)  /* predictor instance */
{
  if (!pred)
    return;
  
  if (pred->local_hist)
    free(pred->local_hist);
  
  if (pred->local_pred)
    free(pred->local_pred);
  
  if (pred->global_pred)
    free(pred->global_pred);
  
  if (pred->choice)
    free(pred->choice);
  
  free(pred);
}