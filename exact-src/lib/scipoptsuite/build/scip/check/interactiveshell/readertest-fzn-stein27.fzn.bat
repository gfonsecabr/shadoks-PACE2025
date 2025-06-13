set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/stein27.fzn"
write problem temp/stein27.fzn.fzn
presolve
write transproblem temp/stein27.fzn_trans.fzn
set heur emph def
read temp/stein27.fzn_trans.fzn
optimize
validatesolve "18" "18"
read temp/stein27.fzn.fzn
optimize
validatesolve "18" "18"
quit
