set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/p0548.mps"
write problem temp/p0548.mps.rlp
presolve
write transproblem temp/p0548.mps_trans.rlp
set heur emph def
read temp/p0548.mps_trans.rlp
optimize
validatesolve "8691" "8691"
read temp/p0548.mps.rlp
optimize
validatesolve "8691" "8691"
quit
