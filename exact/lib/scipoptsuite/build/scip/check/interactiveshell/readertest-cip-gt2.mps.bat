set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/gt2.mps"
write problem temp/gt2.mps.cip
presolve
write transproblem temp/gt2.mps_trans.cip
set heur emph def
read temp/gt2.mps_trans.cip
optimize
validatesolve "21166" "21166"
read temp/gt2.mps.cip
optimize
validatesolve "21166" "21166"
quit
