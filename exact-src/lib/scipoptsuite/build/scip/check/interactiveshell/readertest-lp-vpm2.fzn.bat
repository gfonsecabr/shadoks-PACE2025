set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/vpm2.fzn"
write problem temp/vpm2.fzn.lp
presolve
write transproblem temp/vpm2.fzn_trans.lp
set heur emph def
read temp/vpm2.fzn_trans.lp
optimize
validatesolve "13.75" "13.75"
read temp/vpm2.fzn.lp
optimize
validatesolve "13.75" "13.75"
quit
