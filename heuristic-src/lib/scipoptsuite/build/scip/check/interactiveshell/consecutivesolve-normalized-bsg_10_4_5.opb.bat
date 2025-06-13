set display verblevel 0
set timing enabled FALSE
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/PseudoBoolean/normalized-bsg_10_4_5.opb"
optimize
write statistics temp/normalized-bsg_10_4_5.opb_r1.stats
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/PseudoBoolean/normalized-bsg_10_4_5.opb"
optimize
write statistics temp/normalized-bsg_10_4_5.opb_r2.stats
quit
