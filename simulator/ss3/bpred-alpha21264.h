/* bpred-alpha21264.h - Alpha 21264 tournament branch predictor */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 */

#ifndef BPRED_ALPHA21264_H
#define BPRED_ALPHA21264_H

#include <stdio.h>
#include "host.h"
#include "misc.h"
#include "machine.h"

/*
 * This module implements the Alpha 21264 tournament branch predictor.
 * The predictor combines local and global branch history predictors
 * with a choice predictor to select between them.
 *
 * Components:
 *   - Local Predictor: Uses per-branch local history
 *   - Global Predictor: Uses global branch history 
 *   - Choice Predictor: Selects between local and global
 */

/* Alpha 21264 tournament predictor structure */
struct bpred_alpha21264_t {
  /* local predictor components */
  int local_size;                   /* local history table size */
  int local_hist_width;             /* local history register width */
  unsigned int *local_hist;         /* local history table */
  unsigned char *local_pred;        /* local predictor table */
  int local_pred_size;              /* local predictor table size */
  
  /* global predictor components */
  int global_hist_width;            /* global history register width */
  unsigned int global_hist;         /* global history register */
  unsigned char *global_pred;       /* global predictor table */
  int global_pred_size;             /* global predictor table size */
  
  /* choice predictor components */
  int choice_size;                  /* choice predictor table size */
  unsigned char *choice;            /* choice predictor table */
};

/* create an Alpha 21264 tournament predictor */
struct bpred_alpha21264_t *         /* Alpha 21264 predictor instance */
bpred_alpha21264_create(
  unsigned int local_size,          /* local history table size */
  unsigned int pred_table_size,     /* local/global predictor table size */
  unsigned int hist_width,          /* history register width */
  unsigned int choice_size);        /* choice predictor table size */

/* probe the Alpha 21264 predictor for a prediction */
unsigned char *                     /* pointer to pred counter */
bpred_alpha21264_lookup(
  struct bpred_alpha21264_t *pred,  /* predictor instance */
  md_addr_t baddr);                 /* branch address */

/* update the Alpha 21264 predictor */
void
bpred_alpha21264_update(
  struct bpred_alpha21264_t *pred,  /* predictor instance */
  md_addr_t baddr,                  /* branch address */
  int taken);                       /* actual branch outcome */

/* print Alpha 21264 predictor configuration */
void
bpred_alpha21264_config(
  struct bpred_alpha21264_t *pred,  /* predictor instance */
  char name[],                      /* predictor name */
  FILE *stream);                    /* output stream */

/* print Alpha 21264 predictor statistics */
void
bpred_alpha21264_stats(
  struct bpred_alpha21264_t *pred,  /* predictor instance */
  FILE *stream);                    /* output stream */

/* free Alpha 21264 predictor resources */
void
bpred_alpha21264_free(
  struct bpred_alpha21264_t *pred); /* predictor instance */

#endif /* BPRED_ALPHA21264_H */