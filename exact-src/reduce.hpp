#pragma once
#include <vector>
#include <cassert>
#include <glpk.h>
#include "hypergraph.hpp"
#include "maxsat.hpp"
#include "tools.hpp"
#include "mip.hpp"
#include "greedy.hpp"

#include <iostream>

inline std::unordered_set<int> solveSAT(const HyperGraph &hg, double timeLimit, std::string solver) {
  if(hg.countEdges()==0)
    return {};

  std::string satstr = hg.stringSAT();
  /*
  std::ofstream f("model.sat");
  f << satstr;
  */
  int timeout = timeLimit - elapsed();
  std::unordered_set<int> sol;

  std::vector<std::string> cmd ={std::to_string(timeout), solver};
  
  std::string result = runExternal("timeout", cmd, satstr);
  std::istringstream issResult(result);

  std::string line;
  std::string res;
  int cost = -1;
  while (std::getline(issResult, line)) {
    std::istringstream iss(line);
    char action;
    if(!(iss >> action)) {
      continue;
    }
    switch (action) {
      case 'c':
      {
        break;
      }

      case 'o':
      {
        if(!(iss >> cost)) {
          std::cout << cost << ' ';
          assert(false);
        }
        break;
      }

      case 's':
      {
        if(!(iss >> res)) {
          assert(false);
        }
        break;
      }

      case 'v':
      {
        if(!(iss >> res)) {
          assert(false);
        }

        int res2;
        if(!(iss >> res2)) {
          if(res.compare("-1") ==  0) {
            res = "0"; // special case of a single variable
          }

          sol.clear();
          for(unsigned int i=0; i<res.size(); i++) {
            if(res[i] == '1')
              sol.insert(i);
          }
        } else {
          // Old format
          assert(false);
        }

        break;
      }

      default:
        assert(false);
    }
  }

  return sol;
}

class Reduce {
  HyperGraph &hg;
  std::vector<char> brokenEdges, brokenEdgesContaining;

public:
  Reduce(HyperGraph &_hg)
  : hg(_hg)
  , brokenEdges(hg.countEdges(),false)
  , brokenEdgesContaining(hg.countVertices(), false) {
  }

  // Reduce as much as possible
  std::vector<int> reduce(double tlimit, bool verbose = true) {
    std::unordered_set<int> empty;
    return reduce(tlimit, verbose, empty);
  }

  // Reduce as much as possible, updating sol accordingly
  std::vector<int> reduce(double tlimit, bool verbose, std::unordered_set<int> &sol) {
    const static char redchar[3] = {'e','v','f'};
    double t0 = elapsed();
    std::vector<int> fixedLabels;
    std::unordered_set<int> fixed;
    int improvement[3] = {1,1,1};
    if(verbose)
      std::cout << "c Reducing: " << std::flush;
    for(int i = 1; i < 4 || improvement[i%3] == -1 || improvement[(i-2)%3] != 0 || improvement[(i-1)%3] != 0; i++) {
      double t = elapsed();
      if(i % 3 == 0) {
        if(t >= tlimit)
          break;
        if(i > 6 && improvement[(i+1)%3] != 0 && improvement[(i+2)%3] != 0 && improvement[i%3] <= 0 && i % 120 != 0 )
          improvement[i%3] = -1;
        else
          improvement[i%3] = trimEdges(tlimit);
      }
      else if(i % 3 == 1) {
        if(sol.empty())
          improvement[i%3] = trimVertices();
        else
          improvement[i%3] = trimVertices(sol);
      }
      else {
        auto p = fixVertices(fixedLabels, fixed);
        improvement[i%3] = p.first;
      }
      if(verbose && improvement[i%3] > 0)
        std::cout << redchar[i%3] << std::flush;
    }

    for(int v : fixed)
      sol.erase(v);

    if(verbose)
      std::cout << std::endl << "c Fixed vertices: " << fixedLabels.size() << std::endl;

    removeEmpty(sol);

    if(verbose) {
      double tred = elapsed();
      std::cout << "c Reduction time: " << tred - t0 << std::endl;
    }
    return fixedLabels;
  }


protected:
  // Remove vertices that are not maximal and rename them in sol
  int trimVertices(std::unordered_set<int> &sol) {
    int ret = 0;

    for(int i = 0; i < hg.countVertices(); i++) {
      if(!hg.edgesContaining[i].empty()) {
        for(int j : hg.edges[hg.edgesContaining[i].back()]) {
          if(i != j && isSubSet(hg.edgesContaining[i],hg.edgesContaining[j])) {
            // When equal vectors, keep first
            if(hg.edgesContaining[i].size() < hg.edgesContaining[j].size() || i > j) {
              // Remove vertex i because of j
              if(sol.contains(i)) {
                sol.erase(i);
                sol.insert(j);
              }
              for(int e :hg.edgesContaining[i])
                brokenEdges[e] = true;
              hg.edgesContaining[i].clear();
              ret++;
              break;
            }
          }
        }
      }
    }

    repairEdges();

    return ret;
  }

  // Remove vertices that are not maximal
  int trimVertices() {
    int ret = 0;

    for(int i = 0; i < hg.countVertices(); i++) {
      if(!hg.edgesContaining[i].empty()) {
        for(int j : hg.edges[hg.edgesContaining[i].back()]) {
          if(i != j && isSubSet(hg.edgesContaining[i],hg.edgesContaining[j])) {
            // When equal vectors, keep first
            if(hg.edgesContaining[i].size() < hg.edgesContaining[j].size() || i > j) {
              // Remove vertex i
              for(int e :hg.edgesContaining[i])
                brokenEdges[e] = true;
              hg.edgesContaining[i].clear();
              ret++;
              break;
            }
          }
        }
      }
    }

    repairEdges();

    return ret;
  }

  // Remove hyperedges that are not minimal
  int trimEdges(double tmax) {
    int ret = 0;

    // Remove equal edges of size 2
    std::unordered_set<std::pair<int,int>> biEdges;
    bool hasLargeEdges = false;
    for(int i = 0; i < hg.countEdges(); i++) {
      if(hg.edges[i].size() == 2) {
        if(!biEdges.emplace(hg.edges[i][0],hg.edges[i][1]).second) {
          for(int v : hg.edges[i])
            brokenEdgesContaining[v] = true;
          hg.edges[i].clear();
          ret++;
        }
      }
      else
        hasLargeEdges = true;
    }

    if(hasLargeEdges) {
      // Remove remaining edges that are not minimal
      for(int i = 0; i < hg.countEdges(); i++) {
        if(elapsed() > tmax)
          break;
        if(!hg.edges[i].empty()) {
          for(int j : hg.edgesContaining[hg.edges[i].back()]) {
            if(hg.edges[i].size() == hg.edges[j].size() && hg.edges[i].size() == 2)
              continue; // Treated above
              // When equal vectors, keep first
            if( !hg.edges[j].empty() &&
                (hg.edges[i].size() < hg.edges[j].size() || i < j) &&
                isSubSet(hg.edges[i],hg.edges[j])
               ) {
              // Remove edge j
              for(int v : hg.edges[j])
                brokenEdgesContaining[v] = true;
              hg.edges[j].clear();
              ret++;
            }
          }
        }
      }
    }

    repairEdgesContaining();

    return ret;
  }

  // Called by fixVertices to remove a vertex and all its hyperedges
  int removeVerticesWithEdges(const std::unordered_set<int> &toDelete) {
    int ret = 0;
    if(toDelete.empty())
      return ret;
    for(int vi : toDelete) {
      for(int ei : hg.edgesContaining[vi]) {
        for(int v : hg.edges[ei])
          brokenEdgesContaining[v] = true;
        hg.edges[ei].clear();
        ret++;
      }
      hg.edgesContaining[vi].clear();
    }

    repairEdgesContaining();

    return ret;
  }

  // Fix vertices given by toFix to the solution (in fixedLabels)
  std::pair<int,int> fixGivenVertices(std::vector<int> &fixedLabels, const std::unordered_set<int> &toFix) {
    for(auto l : hg.getLabels(toFix))
      fixedLabels.push_back(l);

    return std::make_pair((int)toFix.size(), removeVerticesWithEdges(toFix));
  }

  // Fix vertices that are in a singleton hyperedge to the solution (in fixedLabels)
  std::pair<int,int> fixVertices(std::vector<int> &fixedLabels, std::unordered_set<int> &fixed) {
    std::unordered_set<int> toFix;
    for(int ei = 0; ei < hg.countEdges(); ei++)
      if(hg.edges[ei].size() == 1) {
        toFix.insert(hg.edges[ei][0]);
        fixed.insert(hg.edges[ei][0]);
      }

    return fixGivenVertices(fixedLabels, toFix);
  }

  // Remove empty vertices and hyperedges, relabelling everything
  void removeEmpty(std::unordered_set<int> &sol) {
    if(!sol.empty()) {
      // -2 means not in the solution
      // -1 means not in the graph (should not be in the solution either)
      std::vector<int> solv(hg.countVertices(), -2);
      for(int v : sol)
        solv[v] = v;
      for(int i = 0; i < hg.countVertices(); i++) {
        if(hg.edgesContaining[i].empty()) {
          if(solv[i] != -2) {
            // Different fixed vertices are incompatible
            solv.clear();
            sol.clear();
            break;
          }
          solv[i] = -1;
        }
      }
      std::erase(solv, -1);

      for(int i = 0; i < (int) solv.size(); i++) {
        int v = solv[i];
        if(v != -2) {
          sol.erase(v);
          sol.insert(i);
        }
      }
    }

    for(int i = 0; i < hg.countVertices(); i++) {
      if(hg.edgesContaining[i].empty())
        hg.labels[i] = -1;
    }
    std::erase(hg.edgesContaining, std::vector<int>());
    std::erase(hg.labels, -1);

    std::vector<int> newNumber(hg.edges.size(), -1);
    int m = 0;
    for(std::vector<int> &evec : hg.edgesContaining) {
      for(int &e : evec) {
        if(newNumber[e] == -1) {
          newNumber[e] = m++;
        }
        e = newNumber[e];
      }
    }

    hg.buildEdges();
  }

  void repairEdgesContaining() {
    for(int i = 0; i < hg.countVertices(); i++)
      repairEdgesContaining(i);
  }

  void repairEdgesContaining(int i) {
    if(brokenEdgesContaining[i]) {
      std::erase_if(hg.edgesContaining[i], [this](int e){return this->hg.edges[e].empty();});
      brokenEdgesContaining[i] = false;
    }
  }

  void repairEdges() {
    for(int i = 0; i < hg.countEdges(); i++)
      repairEdges(i);
  }

  void repairEdges(int i) {
    if(brokenEdges[i]) {
      std::erase_if(hg.edges[i], [this](int v){return this->hg.edgesContaining[v].empty();});
      brokenEdges[i] = false;
    }
  }
};


template<class C>
int removeVerticesWithEdges(HyperGraph &hg, const C &toDelete) {
  int ret = 0;
  if(toDelete.empty())
    return ret;
  for(int vi : toDelete) {
    for(int ei : hg.edgesContaining[vi]) {
      hg.edges[ei].clear();
      ret++;
    }
  }
  std::erase(hg.edges, std::vector<int>());
  hg.buildEdgesContaining();

  //Renumber vertices in hg edges
  std::vector<int> newNumber(hg.countVertices());
  for(int i=0, j=0; i < hg.countVertices(); i++) {
    newNumber[i] = j;
    if(hg.edgesContaining[i].empty())
      hg.labels[i] = -1;
    else
      j++;
  }
  for(std::vector<int> &e : hg.edges)
    for(int &vi : e)
      vi = newNumber[vi];

  std::erase(hg.labels, -1);
  hg.buildEdgesContaining();
  return ret;
}

template<class T>
inline std::pair<int,int> fixGivenVertices(HyperGraph &hg,
                       std::vector<int> &fixedLabels,
                       const T &toFix) {
  for(auto l : hg.getLabels(toFix))
    fixedLabels.push_back(l);

  return std::make_pair((int)toFix.size(), removeVerticesWithEdges(hg, toFix));
}


inline int reduceComponents(HyperGraph &hg, std::vector<int> &fixedLabels, double timeLimit, bool verbose = true) {
  int ret = 0;
  std::vector<std::vector<int>> components = hg.components();
  std::unordered_set<int> sol;
  double t0 = elapsed();

  if(verbose)
    std::cout << "c Components: " << components.size() << " -> [" << std::flush;
  bool usemip = true;

  for(int i = components.size() - 1; i >= 0 && elapsed() < timeLimit; i--) {
    HyperGraph component = hg.componentSubGraph(components[i]);
    Mip mip(component);

    if(usemip && mip.solveExact(1)) {
      auto csol = mip.getSolutionLabels();
      sol.insert(csol.begin(), csol.end());
      ret++;
    }
    else {
      usemip = false;
      MaxSAT maxsat(component);
      // double t = elapsed();
      double timeout = 6 + 12*(i <= 3);
      if(maxsat.solveExact(timeout, 1 + (i <= 3))) {
          auto csol = maxsat.getSolutionLabels();
        sol.insert(csol.begin(), csol.end());
        ret++;
        std::cout << '+' << std::flush;
      }
      else if(verbose) {
        std::cout << " " << components[i].size() << std::flush;
      }
      // if(elapsed()-t > 1.2*timeout) {
      //   std::cout << "[" << timeout << "," <<elapsed()-t << "]"<<std::flush;
      // }
    }
  }

  fixGivenVertices(hg, fixedLabels, sol);
  if(verbose)
    std::cout << " ] = " << components.size()-ret << " components in " << elapsed() - t0 << std::endl;

  return ret;
}
