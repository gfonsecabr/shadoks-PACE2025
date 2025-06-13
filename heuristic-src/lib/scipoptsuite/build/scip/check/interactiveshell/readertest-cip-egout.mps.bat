set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/egout.mps"
write problem temp/egout.mps.cip
presolve
write transproblem temp/egout.mps_trans.cip
set heur emph def
read temp/egout.mps_trans.cip
optimize
validatesolve "568.1007" "568.1007"
read temp/egout.mps.cip
optimize
validatesolve "568.1007" "568.1007"
quit
