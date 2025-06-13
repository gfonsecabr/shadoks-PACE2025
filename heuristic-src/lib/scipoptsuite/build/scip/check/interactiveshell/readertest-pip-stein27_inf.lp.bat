set heur emph off
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MIP/stein27_inf.lp"
write problem temp/stein27_inf.lp.pip
presolve
write transproblem temp/stein27_inf.lp_trans.pip
set heur emph def
read temp/stein27_inf.lp_trans.pip
optimize
validatesolve "+infinity" "+infinity"
read temp/stein27_inf.lp.pip
optimize
validatesolve "+infinity" "+infinity"
quit
