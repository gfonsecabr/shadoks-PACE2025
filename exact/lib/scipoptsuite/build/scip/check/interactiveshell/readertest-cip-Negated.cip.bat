set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/Negated.cip"
write problem temp/Negated.cip.cip
presolve
write transproblem temp/Negated.cip_trans.cip
set heur emph def
read temp/Negated.cip_trans.cip
optimize
validatesolve "1" "1"
read temp/Negated.cip.cip
optimize
validatesolve "1" "1"
quit
