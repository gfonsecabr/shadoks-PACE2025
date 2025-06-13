read /home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip/check/instances/MIP/flugpl.mps
set reoptimization advanced maxsavednodes 100
set reoptimization advanced delay 0
set reoptimization advanced usecuts FALSE
set reoptimization advanced usepscost FALSE
set reoptimization advanced storevarhistory FALSE
set reoptimization advanced solvelp 3
set reoptimization advanced saveconsprop TRUE
set reoptimization advanced forceheurrestart 1
set load /home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip/check/coverage/settings/reopt-test-settings-varorderinterdiction-i.set
optimize
read /home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip/check/instances/MIP/flugpl_reopt/flugpl_1.diff
optimize
validatesolve -75 -75
read /home/florent/UQAM/MaxSAT2024/code/EvalMaxSAT/lib/scipoptsuite/scip/check/instances/MIP/flugpl_reopt/flugpl_2.diff
optimize
validatesolve 105 105
display reoptstatistic
quit
