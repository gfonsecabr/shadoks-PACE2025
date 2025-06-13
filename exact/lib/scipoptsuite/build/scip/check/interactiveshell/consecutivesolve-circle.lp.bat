set display verblevel 0
set timing enabled FALSE
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MINLP/circle.lp"
optimize
write statistics temp/circle.lp_r1.stats
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/MINLP/circle.lp"
optimize
write statistics temp/circle.lp_r2.stats
quit
