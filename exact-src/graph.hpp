#pragma once

#include <vector>
#include <queue>
#include <cassert>
#include <unordered_set>
#include <stack>
#include <algorithm>

template<class Vertex>
class Graph {
  std::vector<std::vector<Vertex>> adj;

public:
  Graph() {
  }

  Graph(int n) {
    adj.resize(n);
  }

  void addVertex(Vertex v) {
    if(v >= (int) adj.size())
      adj.resize(v+1);
  }

  void addEdge(Vertex u, Vertex v) {
    addVertex(v);
    addVertex(u);

    if(u != v) {
      adj[u].push_back(v);
      adj[v].push_back(u);
    }
  }

  int countVertices() const {
    return adj.size();
  }

  int countEdges() const {
    int ret = 0;
    for(const auto &v : adj) {
      ret += v.size();
    }
    assert(ret % 2 == 0);
    return ret / 2;
  }

  void clear() {
    adj.clear();
  }

  std::vector<Vertex> vertices() const {
    std::vector<Vertex> ret;
    for(int v = 0; v < countVertices(); v++) {
      ret.push_back(v);
    }
    return ret;
  }

  std::unordered_set<Vertex> verticesSet() const {
    std::unordered_set<Vertex> ret;
    for(int v = 0; v < countVertices(); v++) {
      ret.insert(v);
    }
    return ret;
  }


  void trim() {
    for(int i = 0; i < countVertices(); i++) {
      std::sort(adj[i].begin(), adj[i].end());
      auto last = std::unique(adj[i].begin(),adj[i].end());
      if(last != adj[i].end()) {
        adj[i].erase(last,adj[i].end());
      }
      std::erase(adj[i],i);
    }
  }

  const std::vector<Vertex> &neighbors(Vertex v) const {
    return adj[v];
  }

  // Neighbors including v itself
  std::vector<Vertex> closedNeighbors(Vertex v) const {
    std::vector<Vertex> ret = neighbors(v);
    ret.push_back(v);
    return ret;
  }

  std::vector<Vertex> bfs(Vertex v, int maxv = 0) const {
    std::unordered_set<Vertex> visited;
    std::vector<Vertex> ret;
    std::queue<Vertex> fifo;

    if(maxv == 0)
      maxv = countVertices();

    fifo.push(v);
    while(!fifo.empty() && ret.size() < (size_t) maxv) {
      Vertex u = fifo.front();
      fifo.pop();
      if(visited.count(u) != 0)
        continue;
      ret.push_back(u);
      visited.insert(u);
      for(Vertex w : neighbors(u))
        fifo.push(w);
    }

    return ret;
  }

  std::vector<std::vector<int>> components() const {
    std::vector<char> visited(countVertices(),0);
    std::vector<std::vector<int>> ret;

    for(Vertex v = 0; v < countVertices(); v++) {
      if(visited[v] == 0) {
        ret.push_back(bfs(v));
        for(int u : ret.back())
          visited[u] = 1;
      }
    }
    std::sort(ret.begin(),ret.end(),[](auto &a, auto &b){return a.size() > b.size();});

    return ret;
  }

  std::vector<std::vector<int>> biconnectedComponents() {
    int n = adj.size(), timer = 0;
    std::vector<int> index(n, -1), low(n, -1);
    std::stack<std::pair<int, int>> edgeStack;
    std::vector<std::vector<int>> biconnectedComponents;

    struct Frame {
      int u, parent, childIndex;
    };

    for (int start = 0; start < n; ++start) {
      if (index[start] != -1) continue;

      std::stack<Frame> stack;
      stack.push({start, -1, 0});

      while (!stack.empty()) {
        Frame& frame = stack.top();
        int u = frame.u, parent = frame.parent;

        if (index[u] == -1) {
          index[u] = low[u] = timer++;
        }

        if (frame.childIndex < adj[u].size()) {
          int v = adj[u][frame.childIndex++];
          if (v == parent) continue;
          if (index[v] == -1) {
            edgeStack.emplace(u, v);
            stack.push({v, u, 0});
          } else {
            low[u] = std::min(low[u], index[v]);
            if (index[v] < index[u]) {
              edgeStack.emplace(u, v);
            }
          }
        } else {
          stack.pop();
          if (parent != -1) {
            low[parent] = std::min(low[parent], low[u]);
            if (low[u] >= index[parent]) {
              std::vector<int> component;
              std::unordered_set<int> addedVertices;
              std::pair<int, int> edge;
              do {
                edge = edgeStack.top(); edgeStack.pop();
                if (addedVertices.insert(edge.first).second)
                  component.push_back(edge.first);
                if (addedVertices.insert(edge.second).second)
                  component.push_back(edge.second);
              } while (edge != std::make_pair(parent, u));
              biconnectedComponents.push_back(component);
            }
          }
        }
      }
    }

    std::sort(biconnectedComponents.begin(),biconnectedComponents.end(),[](auto &a, auto &b){return a.size() > b.size();});

    return biconnectedComponents;
  }


  // https://www.naukri.com/code360/library/biconnected-components-in-graph
  std::vector<std::unordered_set<int>> oldBiconnectedComponents() const {
    std::vector<int>discoveryTime(countVertices(),-1);
    std::vector<int>low(countVertices(),-1);
    std::vector<int>parent(countVertices(),-1);
    int time = 0;
    std::vector<std::unordered_set<int>> output;

    std::stack<std::pair<int,int>> stackk;
    for (int i = 0; i < countVertices(); i++) {
      if (discoveryTime[i] == -1)
        dfsHelper(i, discoveryTime, low, stackk, parent, time, output);

      if(stackk.size())
        output.push_back({});
      while (stackk.size() > 0) {
        // std::cout << stackk.top().first << "--" << stackk.top().second << " ";
        output.back().insert(stackk.top().first);
        output.back().insert(stackk.top().second);
        stackk.pop();
      }
      // std::cout<<std::endl;
    }

    std::sort(output.begin(),output.end(),[](auto &a, auto &b){return a.size() > b.size();});

    return output;
  }

  protected:

  void dfsHelper(int u, std::vector<int>&discoveryTime, std::vector<int>&low,
                std::stack<std::pair<int,int>> &stackk, std::vector<int>&parent,
                int &time, std::vector<std::unordered_set<int>> &output) const {
    discoveryTime[u] = low[u] = ++time;
    int children = 0;

    for(auto v:adj[u]){
      if (discoveryTime[v] == -1) {
        children++;
        parent[v] = u;

        stackk.push({u,v});
        dfsHelper(v, discoveryTime, low, stackk, parent, time, output);

        low[u] = std::min(low[u], low[v]);

        if ((discoveryTime[u] == 1 && children > 1) ||
            (discoveryTime[u] > 1 && low[v] >= discoveryTime[u])) {
          output.push_back({});
          while (stackk.top().first != u || stackk.top().second != v) {
            // std::cout << stackk.top().first << "--" << stackk.top().second << " ";
            output.back().insert(stackk.top().first);
            output.back().insert(stackk.top().second);
            stackk.pop();
          }
          // std::cout << stackk.top().first << "--" << stackk.top().second;
          output.back().insert(stackk.top().first);
          output.back().insert(stackk.top().second);
          stackk.pop();
          // std::cout << std::endl;
        }
      }
      else if (v != parent[u]) {
        low[u] = std::min(low[u], discoveryTime[v]);
        if (discoveryTime[v] < discoveryTime[u]) {
          stackk.push({u,v});
        }
      }
    }
  }
};

