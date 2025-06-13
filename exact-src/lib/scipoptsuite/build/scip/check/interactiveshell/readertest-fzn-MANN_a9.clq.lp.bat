set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/MANN_a9.clq.lp"
write problem temp/MANN_a9.clq.lp.fzn
presolve
write transproblem temp/MANN_a9.clq.lp_trans.fzn
set heur emph def
read temp/MANN_a9.clq.lp_trans.fzn
optimize
validatesolve "16" "16"
read temp/MANN_a9.clq.lp.fzn
optimize
validatesolve "16" "16"
quit
