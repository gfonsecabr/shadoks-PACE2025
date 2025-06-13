#pragma once
#include "hypergraph.hpp"
#include <cassert>
#include "EvalMaxSAT.h"
#include "mcqd.h"
#include "ArraySet.h"
#include "Tools.h"
#include "CliqueTools.h"
#include "MISS.h"

class MaxSAT {
protected:
  const HyperGraph &hg;
  std::vector<int> solution;
  std::vector<int> subopt;
public:
  MaxSAT(const HyperGraph &_hg, int _maxValue = -1)
  : hg(_hg) {
  }

  bool solveExact(double maxTime, double scipTime=0.0, const std::unordered_set<int> &sol = {}, bool limitTime = true) {
    int m =  hg.countEdges(), n = hg.countVertices();
    if(m == 0)
      return true;
    EvalMaxSAT solver;

    std::vector<int> vars(n);
    int maxVar = 0;
    for(int &var : vars) {
      var = solver.newVar();
      solver.addClause({-var}, 1);
      maxVar = std::max(var, maxVar);
    }

    for(int e = 0; e < m; e++) {
      std::vector<int> clause;
      for(int v : hg.edges[e]) {
        clause.push_back(vars[v]);
      }
      solver.addClause(clause);
    }

    solver.setTargetComputationTime(maxTime, scipTime);
    if(maxTime < 100) { // Quick solver for componnent reduction
      solver.setBoundRefTime(0.0, 0.0);
      solver.unactivateMultiSolveStrategy();
    }
    else {
      if(hg.countVertices() <= 10000) {
        solver.setBoundRefTime(.3, 150.0);
      }
      else { // Large instances
        solver.setBoundRefTime(.3, 50.0);
      }
    }
    solver.disableOptimize();

    std::vector<bool> bsol;
    if(!sol.empty()) {
      bsol.resize(maxVar+1);
      for(int v : sol) {
        bsol[vars[v]] = true;
      }

      solver.setSolution(bsol, sol.size());
    }

    if(!limitTime)
      solver.printInfo();

    int opt = limitTime ? solver.solveWithTimeout(maxTime) : solver.solve();

    solution.clear();
    subopt.clear();

    auto &outsol = opt == 1 ? solution :subopt;

    if(solver.getCost() < std::numeric_limits<t_weight>::max()) {
      for(int v = 0; v < hg.countVertices(); v++) {
        if(solver.getValue(vars[v]))
          outsol.push_back(v);
      }
    }

    return opt == 1;

  }

  bool solveVC() {
    if(hg.maxEdgeDegree() > 2)
      return false;
    int n = hg.countVertices();
    
    std::vector< std::vector <char> > matrix;
    matrix.resize(n);
    
    for(int v = 0; v < n; v++) {
        matrix[v].resize(n);
    }

    for(int e = 0; e < hg.countEdges(); e++) {
      int u = hg.edges[e].at(0);
      int v = hg.edges[e].at(1);
      matrix[u][v] = 1;
      matrix[v][u] = 1;
    }

    // Run algorithm
    std::unique_ptr<Algorithm> pAlgorithm(new MISS(matrix)); // Independent set
    
    std::list<std::list<int>> iset;

    auto isetCount = pAlgorithm->Run(iset);

    int size = 0;
    for (auto l: iset)
       size += l.size();

    // Output size
    std::cout << "c Found independent set of size " << size << std::endl;
    if(isetCount == 0)
      return false;

    // Put element in solution
    std::unordered_set<int> isset;
    for(auto component : iset)
        for(auto is : component)
            isset.insert(is);
    solution.clear();
    for(int v = 0; v < n; v++)
      if(!isset.contains(v))
        solution.push_back(v);    

    return true;
  }

  const std::vector<int> &getSolution() const {
    return solution;
  }

  const std::vector<int> &getSubOptSolution() const {
    return subopt;
  }

  const std::vector<int> getSolutionLabels() const {
    return hg.getLabels(solution);
  }
};
