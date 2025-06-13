#pragma once

#include <sstream>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <numeric>
#include "tools.hpp"
#include "graph.hpp"


struct HyperGraph {
  std::vector<int> labels;
  std::vector<std::vector<int>> edges;
  std::vector<std::vector<int>> edgesContaining;

  HyperGraph(){}

  HyperGraph(int n, int m) : labels(n), edges(m), edgesContaining(n) {}

  void sort() {
    sortData(edges);
    sortData(edgesContaining);
  }

  int countVertices() const {
    return edgesContaining.size();
  }

  int countEdges() const {
    return edges.size();
  }

  int maxEdgeDegree() const {
    size_t ret = 0;
    for(auto &v : edges)
      ret = std::max(ret, v.size());
    return ret;
  }

  int maxVertexDegree() const {
    size_t ret = 0;
    for(auto &v : edgesContaining)
      ret = std::max(ret, v.size());
    return ret;
  }

  template<class VSET>
  int maxVertexDegree(const VSET &vset) const {
    size_t ret = 0;
    for(int v : vset) {
      ret = std::max(ret, edgesContaining.at(v).size());
    }
    return ret;
  }

  int sumDegree() const {
    size_t ret = 0;
    for(auto &v : edgesContaining)
      ret += v.size();
    return ret;
  }

  int minEdgeDegree() const {
    size_t ret = countVertices();
    for(auto &v : edges)
      ret = std::min(ret, v.size());
    return ret;
  }

  int minVertexDegree() const {
    size_t ret = countEdges();
    for(auto &v : edgesContaining)
      ret = std::min(ret, v.size());
    return ret;
  }

  void printStats() {
    std::cout << "c Hypergraph with " << countVertices() << " vertices and " << countEdges() << " hyperedges" << std::endl;
    //
    std::cout << "c Maximum vertex degree is " << maxVertexDegree() << ", maximum edge degree is " << maxEdgeDegree() << std::endl;
    // std::cout << "Minimum vertex degree is " << minVertexDegree() << " minimum edge degree is " << minEdgeDegree() << std::endl;
  }

  void buildEdges() {
    edges.clear();
    int maxEdge = -1;
    for(const std::vector<int> &v :edgesContaining)
      for(int x : v)
        maxEdge = std::max(x,maxEdge);
    edges.resize(maxEdge+1);
    for(int i = 0; i < countVertices(); i++)
      for(int ei : edgesContaining[i])
        edges[ei].push_back(i);
    sort();
  }

  void buildEdgesContaining() {
    edgesContaining.clear();
    edgesContaining.resize(labels.size());
    for(int ei = 0; ei < countEdges(); ei++)
      for(int vi : edges[ei])
        edgesContaining[vi].push_back(ei);
    sort();
  }

  template<class T>
  std::vector<int> getLabels(T v) const {
    std::vector<int> ret;
    for(int i : v)
      ret.push_back(labels.at(i));
    return ret;
  }

  template<class T>
  std::unordered_set<int> getLabelsSet(T v) const {
    std::unordered_set<int> ret;
    for(int i : v)
      ret.insert(labels.at(i));
    return ret;
  }

  int newEdge() {
    int e = edges.size();
    edges.push_back({});
    return e;
  }

  void addToEdge(int ai, int e) {
    edges[e].push_back(ai);
    edgesContaining[ai].push_back(e);
  }

  HyperGraph edgeSubGraph(const std::unordered_set<int> &eset) const {
    std::map<int,int> subVertices; // Maps the *this vertex indices to the subgraph vertex indices
    // Using map to maintain vertex order

    for(int e : eset) {
      for(int v : edges[e]) {
        subVertices[v] = -1;
      }
    }
    int n = subVertices.size();

    HyperGraph sub(n, eset.size());
    sub.edgesContaining.resize(n);
    sub.labels.resize(n);
    int i = 0;
    for(auto & [v,j] : subVertices) {
      sub.labels[i] = v;
      j = i;
      i++;
    }
    int se = 0;
    for(int e : eset) {
      sub.edges[se].reserve(edges[e].size());
      for(int v : edges[e])
        sub.addToEdge(subVertices[v], se);
      se++;
    }
    return sub;
  }

  HyperGraph componentSubGraph(const std::vector<int> &component) {
    int n = component.size();
    std::unordered_set<int> subEdges;
    for(int v :component)
      for(int e : edgesContaining[v])
        subEdges.insert(e);
    int m = subEdges.size();

    std::unordered_map<int,int> newIndex;
    for(int i = 0; int v :component) {
      newIndex[v] = i;
      i++;
    }

    HyperGraph sub(n, m);
    sub.labels = component;
    sub.edges.resize(m);
    for(int i = 0; int e : subEdges) {
      sub.edges[i].resize(edges[e].size());
      for(int j = 0; j < (int) edges[e].size(); j++)
        sub.edges[i][j] = newIndex[edges[e][j]];

      i++;
    }

    sub.buildEdgesContaining();
    return sub;
  }

  std::vector<std::vector<int>> components() const {
    std::vector<std::vector<int>> ret;
    std::vector<char> visited(countVertices(),0);

    for(int i = 0; i < countVertices(); i++) {
      if(!visited[i]) {
        std::vector<int> comp;
        std::vector<int> toExplore = {i};
        visited[i] = 1;
        while(!toExplore.empty()) {
          int v = toExplore.back(); // Explore an arbitrary vertex v of toExplore
          toExplore.pop_back();
          comp.push_back(v);
          for(int e :edgesContaining.at(v)) {
            for(int u :edges.at(e)) {
              if(!visited[u]) {
                toExplore.push_back(u);
                visited[u] = 1;
              }
            }
          }
        }
        ret.push_back(comp);
      }
    }
    std::sort(ret.begin(),ret.end(),[](auto &a, auto &b){return a.size() > b.size();});
    return ret;
  }

  std::vector<int> neighbors(int v) {
    std::unordered_set<int> neigh;
    for(int e :edgesContaining.at(v)) {
      for(int u :edges.at(e)) {
        if(u != v)
          neigh.insert(u);
      }
    }

    return std::vector(neigh.begin(),neigh.end());
  }

  void sortData(std::vector<std::vector<int>> &v) {
    for(size_t i = 0; i < v.size(); i++) {
      std::sort(v[i].begin(), v[i].end());
      auto last = std::unique(v[i].begin(),v[i].end());
      if(last != v[i].end()) {
        // std::cerr << "Duplicate elements removed!" << std::endl;
        v[i].erase(last,v[i].end());
      }
    }
  }

  void leastFrequentLast() {
    {
      std::vector<int> vperm(countVertices());
      std::iota(vperm.begin(),vperm.end(), 0);
      std::sort(vperm.begin(),vperm.end(),
                [this](int a, int b){return edgesContaining[a].size() > edgesContaining[b].size();});
      vperm = inversePermutation(vperm);
      translateVec(labels, vperm);
      translateVec(edgesContaining, vperm);
      translateSub(edges, vperm);
    }

    {
      std::vector<int> eperm(countEdges());
      std::iota(eperm.begin(),eperm.end(), 0);
      std::sort(eperm.begin(),eperm.end(),
                [this](int a, int b){return edges[a].size() > edges[b].size();});
      eperm = inversePermutation(eperm);
      translateVec(edges, eperm);
      translateSub(edgesContaining, eperm);
    }

    sort();
  }

  void leastFrequentFirst() {
    {
      std::vector<int> vperm(countVertices());
      std::iota(vperm.begin(),vperm.end(), 0);
      std::sort(vperm.begin(),vperm.end(),
                [this](int a, int b){return edgesContaining[a].size() < edgesContaining[b].size();});
      vperm = inversePermutation(vperm);
      translateVec(labels, vperm);
      translateVec(edgesContaining, vperm);
      translateSub(edges, vperm);
    }

    {
      std::vector<int> eperm(countEdges());
      std::iota(eperm.begin(),eperm.end(), 0);
      std::sort(eperm.begin(),eperm.end(),
                [this](int a, int b){return edges[a].size() < edges[b].size();});
      eperm = inversePermutation(eperm);
      translateVec(edges, eperm);
      translateSub(edgesContaining, eperm);
    }

    sort();
  }

  void shuffle() {
    {
      std::vector<int> vperm(countVertices());
      std::iota(vperm.begin(),vperm.end(), 0);
      std::shuffle(vperm.begin(),vperm.end(),rgen);

      translateVec(labels, vperm);
      translateVec(edgesContaining, vperm);
      translateSub(edges, vperm);
    }

    {
      std::vector<int> eperm(countEdges());
      std::iota(eperm.begin(),eperm.end(), 0);
      std::shuffle(eperm.begin(),eperm.end(),rgen);
      translateVec(edges, eperm);
      translateSub(edgesContaining, eperm);
    }

    sort();
  }

  static std::vector<int> inversePermutation(const std::vector<int> &perm) {
    std::vector<int> inv(perm.size());
    for(size_t i = 0; i < perm.size(); i++) {
      inv[perm[i]] = i;
    }
    return inv;
  }

  template<class T>
  void translateVec(T &v, const std::vector<int> &perm) {
    T old = v;

    for(size_t i = 0; i <v.size(); i++) {
      v[perm.at(i)] = old[i];
    }
  }

  static void translateSub(std::vector<std::vector<int>> &v, const std::vector<int> &perm) {
    for(std::vector<int> &sv : v) {
      for(int &x : sv) {
        x = perm.at(x);
      }
    }
  }

  std::string stringSAT() const {
    std::stringstream f;

    for(auto &e : edges) {
      f << "h ";
      for(int v : e)
        f << v+1 << ' ';
      f << "0" << std::endl;
    }
    for(int i = 0; i < countVertices(); i++)
      f << "1 " << -(i+1) << " 0" << std::endl;

    return f.str();
  }

  void saveSAT(std::string fn, int fs = 0) const {
    std::ofstream f(fn);
    if(fs)
      f << "c fixed " << fs << std::endl;
    for(auto &e : edges) {
      f << "h ";
      for(int v : e)
        f << v+1 << ' ';
      f << "0" << std::endl;
    }
    for(int i = 0; i < countVertices(); i++)
      f << "1 " << -(i+1) << " 0" << std::endl;
  }

  void save(std::string fn) {
    std::ofstream f(fn);

    f << "p hs " << countVertices() << ' ' << countEdges() << std::endl;

    for(auto &e : edges) {
      for(int v : e)
        f << v+1 << ' ';
      f << std::endl;
    }
  }

  void saveLabels(std::string fn) {
    std::ofstream f(fn);

    f << "p hs " << countVertices() << ' ' << countEdges() << std::endl;

    for(auto &e : edges) {
      for(int v : e)
        f << labels.at(v) << ' ';
      f << std::endl;
    }
  }

  bool testSorted() {
    for(std::vector<int> &v : edges)
      if(!is_sorted(v.begin(),v.end())) {
        return false;
      }
    for(std::vector<int> &v : edgesContaining)
      if(!is_sorted(v.begin(),v.end())) {
        return false;
      }
    return true;
  }

};



