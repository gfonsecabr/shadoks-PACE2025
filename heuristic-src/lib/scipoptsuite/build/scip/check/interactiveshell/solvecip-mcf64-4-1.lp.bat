read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/Indicator/mcf64-4-1.lp"
write problem temp/mcf64-4-1.lp.cip
read temp/mcf64-4-1.lp.cip
optimize
validatesolve "10" "10"
quit
