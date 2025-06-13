set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/p0033.osil"
write problem temp/p0033.osil.pip
presolve
write transproblem temp/p0033.osil_trans.pip
set heur emph def
read temp/p0033.osil_trans.pip
optimize
validatesolve "3089" "3089"
read temp/p0033.osil.pip
optimize
validatesolve "3089" "3089"
quit
