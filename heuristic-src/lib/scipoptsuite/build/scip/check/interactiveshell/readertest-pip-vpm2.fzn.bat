set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/vpm2.fzn"
write problem temp/vpm2.fzn.pip
presolve
write transproblem temp/vpm2.fzn_trans.pip
set heur emph def
read temp/vpm2.fzn_trans.pip
optimize
validatesolve "13.75" "13.75"
read temp/vpm2.fzn.pip
optimize
validatesolve "13.75" "13.75"
quit
