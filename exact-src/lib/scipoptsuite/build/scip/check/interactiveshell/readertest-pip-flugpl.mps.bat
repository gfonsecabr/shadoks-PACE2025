set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/flugpl.mps"
write problem temp/flugpl.mps.pip
presolve
write transproblem temp/flugpl.mps_trans.pip
set heur emph def
read temp/flugpl.mps_trans.pip
optimize
validatesolve "1201500" "1201500"
read temp/flugpl.mps.pip
optimize
validatesolve "1201500" "1201500"
quit
