set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/p0548.mps"
write problem temp/p0548.mps.cip
presolve
write transproblem temp/p0548.mps_trans.cip
set heur emph def
read temp/p0548.mps_trans.cip
optimize
validatesolve "8691" "8691"
read temp/p0548.mps.cip
optimize
validatesolve "8691" "8691"
quit
