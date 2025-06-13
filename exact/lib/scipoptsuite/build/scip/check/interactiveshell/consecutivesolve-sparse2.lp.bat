set display verblevel 0
set timing enabled FALSE
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/SOS/sparse2.lp"
optimize
write statistics temp/sparse2.lp_r1.stats
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/SOS/sparse2.lp"
optimize
write statistics temp/sparse2.lp_r2.stats
quit
