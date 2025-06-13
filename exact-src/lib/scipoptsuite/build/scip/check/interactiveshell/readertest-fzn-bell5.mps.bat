set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/bell5.mps"
write problem temp/bell5.mps.fzn
presolve
write transproblem temp/bell5.mps_trans.fzn
set heur emph def
read temp/bell5.mps_trans.fzn
optimize
validatesolve "8966406.49" "8966406.49"
read temp/bell5.mps.fzn
optimize
validatesolve "8966406.49" "8966406.49"
quit
