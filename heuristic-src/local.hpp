#pragma once
#include "hypergraph.hpp"
#include "tools.hpp"
#include "mip.hpp"
#include "reduce.hpp"
#include "maxsat.hpp"
#include "greedy.hpp"
#include <algorithm>
#include <unordered_set>
#include <cassert>

class Local {
protected:
  const HyperGraph *hg;
  std::unordered_set<int> solution;
  std::vector<char> solutionVC;
  std::uniform_int_distribution<> randomEdge;
  std::uniform_int_distribution<> randomVertex;
  std::vector<int> hitCount;
  std::unordered_set<int> unhit;
  std::vector<int> randomVertexPool;
public:
  std::vector<int> testedEdges;
  Local(const HyperGraph &_hg, const std::unordered_set<int> &_solution)
  : hg(&_hg), solution(_solution)
  , solutionVC(hg->countVertices())
  , randomEdge(0,std::max(0,hg->countEdges()-1))
  , randomVertex(0,std::max(0,hg->countVertices()-1))
  , hitCount(hg->countEdges())
  , testedEdges(hg->countEdges()) {
    for(int v : solution) {
      solutionVC[v] = 1;
      for(int e : hg->edgesContaining[v])
        hitCount[e]++;
    }
    for(int e = 0; e < hg->countEdges(); e++) {
      if(hitCount[e] == 0)
        unhit.insert(e);
    }
  }

  int localStep(int limit = 50, double timeout = .5) {
    if(hg->countEdges() == 0)
      return 0;

    static std::bernoulli_distribution bern(0.5);
    std::vector<int> removed = bern(rgen) ? randomRemovals<true>(limit)
                                          : randomRemovals<false>(limit);

    for(int e : unhit)
      testedEdges[e]++;
    HyperGraph sub = hg->edgeSubGraph(unhit);
    sub.shuffle();

    Reduce reducer(sub);
    std::vector<int> fixed = reducer.reduce(elapsed() + .2, false);
    for(int v : fixed)
      insertSolution(v);

    std::vector<int> toInsert;

    if(sub.countEdges()) {
      Greedy subGreedy(sub);
      int previousSubSolSize = (int)removed.size() - (int)fixed.size();
      subGreedy.setBound(previousSubSolSize);
      subGreedy.solveGreedy();
      subGreedy.trim();

      // If greedy solution is better or optimal accept
      // Notice that a greedy solution of at most 2 is always optimal
      if(subGreedy.getSolution().size() < previousSubSolSize
        || subGreedy.getSolution().size() <= 2) {
        toInsert = subGreedy.getSolutionLabels();
      }
      else {
        Mip subSolver(sub);
        if(!subSolver.solveExact(timeout)) {
          for(int v : fixed)
            removeSolution(v);
          for(int v : removed)
            insertSolution(v);
          return -1;
        }
        toInsert = subSolver.getSolutionLabels();
      }

      for(int v : toInsert)
        insertSolution(v);
    }

    return (int) removed.size() - (int) toInsert.size() - (int) fixed.size();
  }

  int localStepSAT(int limit, double timeout = 3) {
    if(hg->countEdges() == 0)
      return 0;

    static std::bernoulli_distribution bern(0.5);
    std::vector<int> removed = bern(rgen) ? randomRemovals<false>(limit)
                                          : randomRemovals<true>(limit);

    for(int e : unhit)
      testedEdges[e]++;
    HyperGraph sub = hg->edgeSubGraph(unhit);
    sub.shuffle();

    // Get previous solution for sub
    std::unordered_map<int,int> newIndex;
    for(int i = 0; i < (int) sub.labels.size(); i++)
      newIndex[sub.labels[i]] = i;

    std::unordered_set<int> previousSubSol;
    for(int v : removed) {
      if(newIndex.contains(v))
        previousSubSol.insert(newIndex.at(v));
    }

    Reduce reducer(sub);
    std::vector<int> fixed = reducer.reduce(elapsed() + .5, false, previousSubSol);
    for(int v : fixed)
      insertSolution(v);

    std::vector<int> toInsert;

    if(sub.countEdges() != 0) {
      MaxSAT subSolver(sub);

      double sciptime = previousSubSol.empty() ? timeout/4.0 : 0.0;
      if(!subSolver.solveExact(timeout, sciptime , previousSubSol)) {
        for(int v : fixed)
          removeSolution(v);
        for(int v : removed)
          insertSolution(v);
        return -1;
      }

      toInsert = subSolver.getSolutionLabels();

      for(int v : toInsert)
        insertSolution(v);
    }

    return (int) removed.size() - (int) toInsert.size() - (int) fixed.size();
  }

  const std::unordered_set<int> &getSolution() const {
    return solution;
  }

  const std::vector<int> getSolutionLabels() const {
    return hg->getLabels(solution);
  }

  int trim() {
    int ret = 0;
    std::vector<int> solv(solution.begin(),solution.end());
    std::shuffle(solv.begin(), solv.end(), rgen);
    for(int v : solv) {
      if(!useful(v)) {
        removeSolution(v);
        ret++;
      }
    }
    return ret;
  }

protected:

  bool removeSolution(int v) {
    if(!solutionVC[v])
      return false;
    for(int e : hg->edgesContaining[v]) {
      if(--hitCount[e] == 0)
        unhit.insert(e);
    }
    solution.erase(v);
    solutionVC[v] = 0;
    return true;
  }

  void insertSolution(int v) {
    if(solutionVC[v])
      return;
    for(int e : hg->edgesContaining[v]) {
      if(hitCount[e]++ == 0)
        unhit.erase(e);
    }
    solution.insert(v);
    solutionVC[v] = 1;
  }

  bool useful(int v) const {
    for(int e : hg->edgesContaining[v])
      if(hitCount[e] == 1)
        return true;

    return false;
  }



  std::vector<int> increasingDegreeEdges(std::vector<int> vec) const {
    std::vector<int> ret = vec;
    std::shuffle(ret.begin(), ret.end(), rgen);
    std::stable_sort(ret.begin(), ret.end(), [this](int a, int b){return hg->edges[a].size() < hg->edges[b].size();});
    return ret;
  }

  int getRandomVertex() {
    if(randomVertexPool.empty()) {
      randomVertexPool.insert(randomVertexPool.end(), solution.begin(), solution.end());
      std::shuffle(randomVertexPool.begin(), randomVertexPool.end(), rgen);
    }
    int ret = randomVertexPool.back();
    randomVertexPool.pop_back();
    return ret;
  }


  template<bool useQueue = true>
  std::vector<int> randomRemovals(int limit) {
    limit = std::min(limit, hg->countVertices());

    typename std::conditional<useQueue, std::list<int>, std::vector<int>>::type edgePool;
    std::list<int> vertexQueue;
    std::unordered_set<int> queuedVertices, pooledEdges;
    std::vector<int> removed;

    int rv = getRandomVertex();
    vertexQueue.push_back(rv);
    queuedVertices.insert(rv);

    while((int)unhit.size() < limit || (int)removed.size() < limit/2) {
      while(vertexQueue.empty()) { // Fill vertex queue
        if(edgePool.empty()) {
          return removed;  // Failed to reach limit. Small connected component!
        }
        int e;
        if constexpr(useQueue) {
          e = edgePool.front();
          edgePool.pop_front();
        }
        else {
          e = extractRandomElement(edgePool);
        }

        for(int v : randomPermutation(hg->edges[e])) {
          if(!queuedVertices.contains(v)) {
            queuedVertices.insert(v);
            vertexQueue.push_back(v);
          }
        }
      }

      int v = vertexQueue.front();
      vertexQueue.pop_front();

      // Add edges to queue
      for(int e : hg->edgesContaining[v]) {
        if(!pooledEdges.contains(e)) {
          pooledEdges.insert(e);
          edgePool.push_back(e);
        }
      }

      // Remove vertex from solution if it is the solution
      if(removeSolution(v)) {
        if(unhit.size() == hg->countEdges() && !removed.empty()) { // Got the whole graph! Undo and stop
          insertSolution(v);
          break;
        }
        removed.push_back(v);
      }
    }

    return removed;
  }


};
