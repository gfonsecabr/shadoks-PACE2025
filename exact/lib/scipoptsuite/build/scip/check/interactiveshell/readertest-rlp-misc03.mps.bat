set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/misc03.mps"
write problem temp/misc03.mps.rlp
presolve
write transproblem temp/misc03.mps_trans.rlp
set heur emph def
read temp/misc03.mps_trans.rlp
optimize
validatesolve "3360" "3360"
read temp/misc03.mps.rlp
optimize
validatesolve "3360" "3360"
quit
