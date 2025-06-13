set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/MANN_a9.clq.lp"
write problem temp/MANN_a9.clq.lp.mps
presolve
write transproblem temp/MANN_a9.clq.lp_trans.mps
set heur emph def
read temp/MANN_a9.clq.lp_trans.mps
optimize
validatesolve "16" "16"
read temp/MANN_a9.clq.lp.mps
optimize
validatesolve "16" "16"
quit
