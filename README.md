# Overview
This project adds two new branch predictors to the SimpleScalar simulator: the Alpha 21264 tournament predictor and perceptron branch predictor.

# Alpha 21264 Tournament Predictor Overview
The Alpha 21264 branch predictor uses 3 components:
1. Local predictor - predicts based on recent branch behavior
    * Tracks per-branch local history
    * Local history: 10 bits per entry (1024-entry table)
    * Local pattern table (PHT): 1K entries, each 3-bit saturating counter
    
2. Global predictor - tracks global correlation across branches
    * Single global history register (GHR): 12 bits
    * Global PHT: 4K entries (each entry is a 2 bit saturating counter)

3. Choice predictor (selector) - chooses between local or global predictor
    * 4K-entry table of 2-bit counters

ALSO: a 2K-entry BTB (branch target buffer) for target addresses

COMMAND:
```
./Run.pl   -db ./bench.db   -dir results/gcc1   -benchmark gcc   -sim <HOME_DIR>/simulator/ss3/sim-outorder   -args "-bpred alpha21264 -fastfwd 1000000 -max:inst 1000000" >& results/alpha.out
```

# Perceptron Branch Predictor
The perceptron branch predictor replaces the typical 2-bit saturating counters in gshare/2-level predictors with simple perceptrons. Perceptrons learn correlations across long global histories, which allows it to scale with long history lengths, unlike the typical 2-level predictor.

Each perceptron corresponds to one static branch, and stores a vector of signed integer weights, one per global history bit. Perceptrons are stored in a perceptron table, and indexed by PC just like in gshare. 

There is one global history register (GHR) for all branches, and every branch uses this same GHR as inputs to its perceptron. Each bit in the GHR is an encoded input for the branch's corresponding perceptron.
* GHR: (older)  T  N  N  T  T  …  (newest)
* x[i]: (older) +1  -1  -1  +1  +1  … (newest)

For history length N, each perceptron stores N+1 weights:
* w0 = bias weight
* w1...wN = correlation weights for history bits x1...xN

The prediction is made by computing the dot product between the global history bits (encoded +/- 1 depending on taken/not taken), and the weights in that branch's weight vector.
* y = w0 + sum(x[i] * w[i])

Based on this dot product, if y is under 0, then it is not taken. Otherwise, it predicts as taken. The magnitude of y is confidence. 

Weights w[i] are initialized to 0, and are learned over time. Training adjusts the weights of only the perceptron belonging to that branch. The update rule is 
* w[i] = w[i] + t*x[i]
* w0 = w0 + t

Where:
* t = +1 if actual outcome was taken
* t = -1 if actual outcome was not taken
* and x[i] are the global history bits
This means that weights increase when xᵢ and t agree; Likewise, weights decrease when xᵢ and t disagree.
