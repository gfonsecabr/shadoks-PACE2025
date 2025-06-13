set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/enigma.mps"
write problem temp/enigma.mps.cip
presolve
write transproblem temp/enigma.mps_trans.cip
set heur emph def
read temp/enigma.mps_trans.cip
optimize
validatesolve "0" "0"
read temp/enigma.mps.cip
optimize
validatesolve "0" "0"
quit
