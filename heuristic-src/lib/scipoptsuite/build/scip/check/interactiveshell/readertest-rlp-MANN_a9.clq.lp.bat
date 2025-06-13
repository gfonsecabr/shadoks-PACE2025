set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/MANN_a9.clq.lp"
write problem temp/MANN_a9.clq.lp.rlp
presolve
write transproblem temp/MANN_a9.clq.lp_trans.rlp
set heur emph def
read temp/MANN_a9.clq.lp_trans.rlp
optimize
validatesolve "16" "16"
read temp/MANN_a9.clq.lp.rlp
optimize
validatesolve "16" "16"
quit
