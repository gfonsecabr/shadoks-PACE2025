# Hitting Set and Dominating Set Solver for PACE 2025

Team Shadoks code for Hitting Set and Dominating Set for the PACE 2025 competition

## Running the Solver

There are three static linked executables in the root directory:

 - ``exact``: the exact hitting set and dominating set solver that guarantees optimality of the solution produced or runs forever if it cannot find an optimal solution.
 - ``heuristic``: the heuristic hitting set and dominating set solver runs for 5 minutes and outputs the best solution found
 - ``anytime``: this is a modified verision of the MAXSAT anytime solver TT-Open-WBO-Inc. The executable must be in the same directory of the other ones and it will be called automatically by the hitting/dominating set solvers.

The solver takes 2 optional command line arguments:

 - ``inputfile``: Name of the input file. If not given, the input is taken from the standard input.
 - ``outputfile``: Name of the output file. If not given, the solution is written to the standard output.

The behavior of the solvers is not deterministic because of the use of time limits throughout the solvers. If teams are allowed 3 versions, please run our solvers 3 times to make the comparison more fair.

## Compiling

The directory ``heuristic-src`` contains the source code of ``heuristic`` and the directory ``exact-src`` contains the source code of ``excact``. The compilacan be done as follows in the src directory.

```bash
cmake .
make
```

The directory ``tt-open-wbo-in`` contains the source of ``anytime``. Please see the compilation instruction in the ``README.md`` file of the directory for compilation instructions.

There are no external dependencies, all dependencies are already included and compiled with shadoks. GLPK 5.0 is included as a compiled library, and the unmodified source code is available at https://www.gnu.org/software/glpk/#downloading . Modified versions of EvalMaxSAT and open-mcs are included and compiled together with our main solvers.

## Algorithms

The solvers are for hitting set. Dominating set instances are reduced to hitting set.

The **heuristic** solver does essentially the following:
 1) Apply three reduction rules: (i) If an edge e is a superset of another edge, remove e. (ii) If a vertex v hits a subset of another vertex, remove v. (iii) If an edge has a single vertex v, add v to the solution and remove all edges that contain v. 
 2) Try to solve each connected component exactly with MIP or MAXSAT solvers, from small to large, aborting if too hard.
 3) Reduce the problem to MAXSAT and run an anytime MAXSAT heuristic solver for around a minute.
 4) Local search: Remove some vertices of the solution, get the subhypergraph of uncovered edges and solve it with either an exact MAXSAT solver or GLPK MIP solver. Repeat this step until out of time.

The **exact** solver does essentially the following:
 1) Apply the same reduction rules of the heuristic solver.
 2) Try to solve each connected component exactly with MIP or MAXSAT solvers, from small to large, aborting if too hard, but with a larger timeout than in the heuristic version.
 3) If all hyperedges have degree 2, the problem is vertex cover. Solve it using an exact Independent Set solver.
 4) Otherwise, find a good heuristic solution (as steps 3 and 4 above) and give it as an initial solution to the MAXSAT solver EvalMaxSAT.

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

