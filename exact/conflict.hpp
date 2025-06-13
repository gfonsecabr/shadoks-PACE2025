#pragma once
#include "hypergraph.hpp"
#include "tools.hpp"
#include <algorithm>
#include <unordered_set>
#include <queue>
#include <cassert>

class Conflict {
protected:
  const HyperGraph &hg;
  std::unordered_set<int> solution;
  std::vector<int> hitCount;
  std::vector<double> baseCost;
  std::vector<char> solutionVC;
  std::unordered_set<int> unhit;
  std::priority_queue<std::pair<double,int>> removeHeap;
  std::priority_queue<std::pair<double,int>> insertHeap;
  const double expo = 1.2;
public:
  inline static size_t iterations = 0;
  Conflict(const HyperGraph &_hg, const std::unordered_set<int> &_solution)
  : hg(_hg), solution(_solution)
    , hitCount(hg.countEdges())
    , baseCost(hg.countEdges(), 1.0)
    , solutionVC(hg.countVertices())
  {
    for(int vi : solution)
      solutionVC[vi] = 1;

    for(int v : solution)
      for(int e : hg.edgesContaining[v])
        hitCount[e]++;

    for(int e = 0; e < hg.countEdges(); e++) {
      if(hitCount[e] == 0)
        unhit.insert(e);
    }
  }

  double insertGain(int vi) const {
    double ret = 0.0;
    for(int ei : hg.edgesContaining[vi]) {
      if(hitCount[ei] == 0)
        ret += pow(baseCost[ei],expo);
      else // I have no idea why dividing by the number of vertices is a good idea
        ret += pow(baseCost[ei],expo) / hg.countVertices();
    }

    return ret;
  }

  double removeGain(int vi) const {
    double ret = 0.0;
    bool redundant = true;
    for(int ei : hg.edgesContaining[vi]) {
      if(hitCount[ei] == 1) {
        ret -= pow(baseCost[ei],expo);
        redundant = false;
      }
      else // I have no idea why dividing by the number of vertices is a good idea
        ret -= pow(baseCost[ei],expo) / hg.countVertices();
    }

    // return ret;
    return redundant ? 0.0 : ret;
  }

  void removeWell() {
    if(removeHeap.empty())
      for(int vi : solution)
        removeHeap.push(std::make_pair(removeGain(vi),vi));

    int best = -1;
    while(true) {
      auto [oldGain, vi] = removeHeap.top();
      removeHeap.pop();
      if(!solutionVC[vi])
        continue;
      double newGain = removeGain(vi);
      if(newGain != oldGain) {
        removeHeap.push(std::make_pair(newGain, vi));
      }
      else {
        best = vi;
        break;
      }
    }

    removeSolution(best);
  }

  void insertWell() {
    static RandomizerWithSentinelShift<> randomBit;
    if(insertHeap.empty())
      for(int vi = 0; vi < hg.countVertices(); vi++)
        if(!solutionVC[vi])
          insertHeap.push(std::make_pair(insertGain(vi),vi));

    int best = -1;
    while(true) {
      auto [oldGain, vi] = insertHeap.top();
      insertHeap.pop();
      if(randomBit(rgen)) { // Some randomness helps... A little messy with heaps
        auto [oldGain2, vi2] = insertHeap.top();
        insertHeap.pop();
        insertHeap.push(std::make_pair(oldGain, vi));
        oldGain = oldGain2;
        vi = vi2;
      }
      if(solutionVC[vi])
        continue;
      double newGain = insertGain(vi);
      if(newGain != oldGain) {
        insertHeap.push(std::make_pair(newGain, vi));
      }
      else {
        best = vi;
        break;
      }
    }

    insertSolution(best);
  }

  bool conflictStep(double timeLimit) {
    if(hg.countEdges() == 0)
      return false;

    auto oldSolution = solution;
    double maxGain = *std::max_element(baseCost.begin(),baseCost.end());
    for(double &c : baseCost)
      c = 1.0 + c/maxGain/16;
    removeHeap = std::priority_queue<std::pair<double,int>>();
    insertHeap = std::priority_queue<std::pair<double,int>>();

    while(true) {
      iterations++;
      removeWell();

      for(int ei : unhit) {
        baseCost[ei]++;
        for(int u : hg.edges[ei]) // Elements whose gain got better need to be updated
          insertHeap.push(std::make_pair(insertGain(u),u));
      }

      if(unhit.empty())
        break;

      insertWell();
      if(elapsed() > timeLimit) {
        solution = oldSolution;
        return false;
      }

      if(unhit.empty()) {
        oldSolution = solution;
      }
   }

   return true;
  }

  const std::unordered_set<int> &getSolution() const {
    return solution;
  }

  const std::vector<int> getSolutionLabels() const {
    return hg.getLabels(solution);
  }

protected:
  bool removeSolution(int v) {
    assert(solution.contains(v));
    for(int e : hg.edgesContaining[v]) {
      if(--hitCount[e] == 0)
        unhit.insert(e);
    }

    for(int e : hg.edgesContaining[v]) {
      if(hitCount[e] == 0) {
        for(int u : hg.edges[e]) // Elements whose gain got better need to be updated
          insertHeap.push(std::make_pair(insertGain(u),u));
      }
    }

    solution.erase(v);
    solutionVC[v] = 0;
    return true;
  }

  void insertSolution(int v) {
    assert(!solution.contains(v));
    for(int e : hg.edgesContaining[v]) {
      if(hitCount[e]++ == 0)
        unhit.erase(e);
    }

    for(int e : hg.edgesContaining[v]) {
      if(hitCount[e] == 2) {
        for(int u : hg.edges[e]) // Elements whose gain got better need to be updated
          removeHeap.push(std::make_pair(removeGain(u),u));
      }
    }

    solution.insert(v);
    solutionVC[v] = 1;
    removeHeap.push(std::make_pair(removeGain(v),v));
  }

};

