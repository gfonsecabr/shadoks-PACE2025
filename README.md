# Hitting Set and Dominating Set Solver for PACE 2025

Team Shadoks code for Hitting Set and Dominating Set for the PACE 2025 competition

## Running the Solver

There are three static linked executables:

 - ``exact``: the exact hitting set and dominating set solver that guarantees optimality of the solution produced or runs forever if it cannot find an optimal solution.
 - ``heuristic``: the heuristic hitting set and dominating set solver runs for 5 minutes and outputs the best solution found
 - ``anytime``: this is a modified verision of the MAXSAT anytime solver TT-Open-WBO-Inc. The executable must be in the same directory of the other ones and it will be called automatically by the hitting/dominating set solvers.

The solver takes 2 optional command line arguments:

 - ``inputfile``: Name of the input file. If not given, the input is taken from the standard input.
 - ``outputfile``: Name of the output file. If not given, the solution is written to the standard output.

The behavior of the solvers is not deterministic because of the use of time limits throughout the solvers. If teams are allowed 3 versions, please run our solvers 3 times to make the comparison more fair.

## Compiling

Please refer to each sub directories but the compilation principles are the same. The directory src contains the sources. The compilation can be done as follows in the src directory.

```bash
cmake .
make
```
There are two executables: shadoks and tt or anytime depending on the sub challenge. Those two executables are mandatory to be in the same directory for the execution. A copy after compilation was placed under the bin directory.

There are no external dependencies, all dependencies are already included and compiled with shadoks. GLPK 5.0 is included as a compiled library, and the source code is available at [[https://www.gnu.org/software/glpk/#downloading]]. A modified version of EvalMaxSAT is included, and is compiled together with our main solvers.

## LICENSE
Copyright 2025 Guilherme D. da Fonseca, Fabien Feschet, and Yan Gerard, the Shadoks team.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

