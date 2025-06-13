set display verblevel 0
set timing enabled FALSE
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/Cardinality/atm_5_25_1.cip"
optimize
write statistics temp/atm_5_25_1.cip_r1.stats
read "/home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip"/check/"instances/Cardinality/atm_5_25_1.cip"
optimize
write statistics temp/atm_5_25_1.cip_r2.stats
quit
