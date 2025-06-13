#pragma once
#include "hypergraph.hpp"
#include "tools.hpp"
#include <queue>
#include <unordered_set>
#include <cassert>

class Greedy {
protected:
  const HyperGraph &hg;
  std::unordered_set<int> solution;
  std::unordered_map<int,int> hit;
  std::priority_queue<std::tuple<double,int,int>> heap;
  double timeLimit = 0.0;
  double bound = 0.0;
  bool fast = false;
public:
  Greedy(const HyperGraph &_hg) : hg(_hg) {
  }

  void setBound(double _bound) {
    bound = _bound;
  }

  void setTimeLimit(double _timeLimit) {
    timeLimit = _timeLimit;
  }

  void solveGreedy() {
    static std::uniform_int_distribution<> distrib(0,65535);

    for(int v = 0; v < hg.countVertices(); v++) {
      auto p = std::make_tuple(countScore(v),distrib(rgen), v);
      heap.push(p);
    }

    while(hit.size() < (unsigned) hg.countEdges()) {
      int v = chooseVertex();
      if(v != -1)
        insertSolution(v);
      // std::cout << v << std::endl;
    }
  }

  void trim() {
    std::vector<int> solv(solution.begin(),solution.end());
    std::shuffle(solv.begin(), solv.end(), rgen);
    for(int v : solv) {
      if(!useful(v))
        removeSolution(v);
    }
  }

  std::unordered_set<int> &getSolution()  {
    return solution;
  }

  const std::vector<int> getSolutionLabels() const {
    return hg.getLabels(solution);
  }

protected:

  void insertSolution(int v) {
    if(solution.contains(v))
      return;
    solution.insert(v);
    for(int e : hg.edgesContaining.at(v)) {
      hit[e]++;
    }
  }

  void removeSolution(int v) {
    if(!solution.contains(v))
      return;
    solution.erase(v);
    for(int e : hg.edgesContaining.at(v)) {
      hit[e]--;
    }
  }

  bool useful(int v) const {
    for(int e : hg.edgesContaining[v])
      if(hit.contains(e) && hit.at(e) == 1)
        return true;

    return false;
  }

  double countScore(int v) {
    double ret = 0;

    if(bound == -1) {
      for(int a :hg.edgesContaining.at(v)) {
        if(!hit.contains(a))
          ret++;
      }
    } else if(bound == 0) {
      for(int a :hg.edgesContaining.at(v)) {
        if(!hit.contains(a))
          ret += 1.0 / hg.edges[a].size();
      }
    }
    else {
      double p = 1.0 - bound / hg.countVertices();
      for(int a :hg.edgesContaining.at(v)) {
        if(!hit.contains(a)) {
          ret += std::max(1e-6, (double)pow(p, hg.edges[a].size()-1));
          assert(ret > 0);
        }
      }
    }

    return ret;
  }


  int chooseVertex() {
    do {
      auto [old_val,rnd,v] = heap.top();
      heap.pop();
      if (!solution.contains(v)) {
        auto cur_val = countScore(v);
        if(fast || elapsed() >timeLimit) {
          fast = true;
          if(cur_val > 0)
            return v;
          else
            continue;
        }
        if(old_val == cur_val && cur_val > 0) {
          return v;
        }
        else if (cur_val > 0){
          heap.push(std::make_tuple(cur_val,rnd,v));
        }
      }
    } while(!heap.empty());
    assert(false);
    return -1;
  }
};
