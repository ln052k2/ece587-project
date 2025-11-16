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
