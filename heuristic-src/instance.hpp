#pragma once

#include "hypergraph.hpp"
#include <string>
#include <fstream>
#include <cassert>
#include "tools.hpp"


class Instance : public HyperGraph {
  std::string inputFile, outputFile;
public:
  Instance(std::string _in = "", std::string _out = "") : inputFile(_in), outputFile(_out) {
    load();
  }

  void load() {
    std::ifstream f(inputFile);

    std::string line;
    bool hs = false;

    if(!f) {
      inputFile = "";
      std::cout << "c Reading instance from standard input" << std::endl;
    }
    else {
      std::cout << "c Instance: " << inputFile << std::endl;
    }

    while (!f ? std::getline(std::cin, line) : std::getline(f, line)) {
      // std::cout<<line<<std::endl;
      if(line.empty() || line[0] == 'c')
        continue; // Comment line
      if(line[0] == 'p') {
        auto v = splitString(line);
        assert(v.size() == 4);
        assert(v[1] == "ds" || v[1] == "hs");
        if(v[1] == "ds") {
          int n = std::stoi(v[2]);
          edges.resize(n);
          edgesContaining.resize(n);
          labels.resize(n);
          for(int i = 0; i < n; i++) {
            labels[i] = i+1;
            addToEdge(i,i);
          }
        }
        else if(v[1] == "hs") {
          hs = true;
          int n = std::stoi(v[2]);
          labels.resize(n);
          edgesContaining.resize(n);
          for(int i = 0; i < n; i++) {
            labels[i] = i+1;
          }
        }
      }
      else {
        auto v = splitString(line);
        if(hs) {
          int e = newEdge();
          for(std::string str : v) {
            int a = std::stoi(str);
            int ai = a-1;
            addToEdge(ai,e);
          }
        }
        else {
          assert(v.size() == 2);
          int a = std::stoi(v[0]);
          int ai = a-1;
          int b = std::stoi(v[1]);
          int bi = b-1;
          addToEdge(ai,bi);
          addToEdge(bi,ai);
        }
      }
    }
    sort();
  }

  static std::vector<std::string> splitString(const std::string &s, char sep = ' ') {
    std::vector<std::string> v;
    std::string cur = "";

    for(char c : s) {
      if(c == sep) {
        if(cur.size() != 0)
          v.push_back(cur);
        cur = "";
      }
      else
        cur += c;
    }

    if(cur.size() != 0)
      v.push_back(cur);

    return v;
  }

  template <class SOL>
  void saveSol(const SOL &sol, const std::vector<int> &fixed) const {
    if(outputFile.empty())
      saveSolTo(sol, fixed, std::cout);
    else {
      std::ofstream f(outputFile);
      saveSolTo(sol, fixed, f);
    }
  }

protected:

  template <class SOL, class T>
  void saveSolTo(const SOL &sol,  std::vector<int> fixed, T &stream) const {
    for(auto x : sol)
      fixed.push_back(x);
    stream << "c Time: " << elapsed() << std::endl;
    stream << fixed.size() << std::endl;
    for(auto v : fixed) {
        stream << v << std::endl;
    }
  }
};
