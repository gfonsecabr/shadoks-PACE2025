#pragma once

#include <random>
#include <chrono>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <cassert>
#include <string.h>
#include <stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<signal.h>
#include<stdlib.h>

static auto beginTime = std::chrono::high_resolution_clock::now();

inline double elapsed() {
  auto end = std::chrono::high_resolution_clock::now();
  auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - beginTime);

  return dur.count() / 1e6;
}

static std::mt19937 rgen(1);

// Returns a random element from a vector
template<class T>
T randomElement(const std::vector<T> &v) {
  std::uniform_int_distribution<> distrib2(0,v.size()-1);
  return v[distrib2(rgen)];
}

// Returns a random element from a vector and remove it from the vector
template<class T>
T extractRandomElement(std::vector<T> &v) {
  std::uniform_int_distribution<> distrib2(0,v.size()-1);
  std::swap(v[distrib2(rgen)], v.back());
  T ret = v.back();
  v.pop_back();
  return ret;
}

// Returns a random permutation from a vector without changing the vector
template<class T>
std::vector<T> randomPermutation(const std::vector<T> &v) {
  std::vector<T> ret = v;
  std::shuffle(ret.begin(), ret.end(), rgen);
  return ret;
}

// Returns the reverse of a vector without changing the vector
template<class T>
std::vector<T> reverse(const std::vector<T> &v) {
  std::vector<T> ret = v;
  std::reverse(ret.begin(),ret.end());
  return ret;
}


// Returns a vectors with all the subsets of s that have size k
inline std::vector<std::vector<int>> combinations(int s, int k) {
  std::vector<std::vector<int>> previous, ret;
  ret.push_back(std::vector<int>());

  for(int i = 0; i < k; i++) {
    previous = ret;
    ret.clear();
    for(int j = 0; j < s; j++) {
      for(auto ps : previous) {
        if(ps.empty() || ps.back() < j) {
          ps.push_back(j);
          ret.push_back(ps);
        }
      }
    }
  }

  return ret;
}

// https://stackoverflow.com/questions/39844193/generate-next-combination-of-size-k-from-integer-vector
inline bool nextCombination(std::vector<int>& v, int k, int N) {
  // We want to find the index of the least significant element
  // in v that can be increased.  Let's call that index 'pivot'.
  int pivot = k - 1;
  while (pivot >= 0 && v[pivot] == N - k + pivot)
    --pivot;

  // pivot will be -1 iff v == {N - k, N - k + 1, ..., N - 1},
  // in which case, there is no next combination.
  if (pivot == -1)
    return false;

  ++v[pivot];
  for (int i = pivot + 1; i < k; ++i)
    v[i] = v[pivot] + i - pivot;
  return true;
}

template <class T>
void print(T v) {
  for(auto x : v)
    std::cout << x << ' ';
  std::cout << std::endl;
}

// https://www.variadic.xyz/post/0120-hashing-tuples/
template<class T>
inline void hash_combine(std::size_t& seed, const T& val) {
  std::hash<T> hasher;
  seed ^= hasher(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// https://www.variadic.xyz/post/0120-hashing-tuples/
namespace std {
  template<class S, class T>
  struct hash<std::pair<S, T>> {
    inline size_t operator()(const std::pair<S, T>& val) const {
      size_t seed = 0;
      hash_combine(seed, val.first);
      hash_combine(seed, val.second);
      return seed;
    }
  };
}

inline bool isSubSet(const std::vector<int> &v1, const std::vector<int> &v2) {
  size_t i1 = 0, i2 = 0;
  if(v1.size() > v2.size())
    return false;

  for(; i1 < v1.size() && i2 < v2.size(); i1++, i2++) {
    while(v1[i1] > v2[i2]) {
      i2++;
      if(i2 == v2.size())
        return false;
      if(v1.size() - i1 > v2.size() - i2) // Not enough left
        return false;
    }
    if(v1[i1] != v2[i2])
      return false;
  }
  return i1 == v1.size();
}

inline bool isSubSet(const std::vector<int> &v1, const std::unordered_set<int> &s2) {
  for(int i : v1)
    if(!s2.contains(i))
      return false;
  return true;
}

inline bool hasCommonElement(const std::vector<int>& v1, const std::vector<int>& v2) {
  int i = 0, j = 0;

  while (i < (int)v1.size() && j < (int)v2.size()) {
    if (v1[i] == v2[j]) {
      return true; // Common element found
    } else if (v1[i] < v2[j]) {
      i++; // Move the pointer in the first vector
    } else {
      j++; // Move the pointer in the second vector
    }
  }

  return false; // No common element found
}

inline std::string exec(std::string cmd) {
  // std::cout << std::endl << cmd << std::endl;
  std::array<char, 128> buffer;
  std::string result;
  auto voidpclose = [](FILE *f){pclose(f);};
  std::unique_ptr<FILE, decltype(voidpclose)> pipe(popen(cmd.c_str(), "r"), voidpclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
    // std::cout << result << std::endl;
  }
  return result;
}

inline std::array<std::vector<int>,3> intersectionRest(const std::vector<int>& v1, const std::vector<int>& v2) {
  int i = 0, j = 0;
  std::array<std::vector<int>,3> ret;

  while (i < (int)v1.size() && j < (int)v2.size()) {
    if (v1[i] == v2[j]) {
      ret[0].push_back(v1[i]);
      i++;
      j++;
    } else if (v1[i] < v2[j]) {
      ret[1].push_back(v1[i]);
      i++; // Move the pointer in the first vector
    } else {
      ret[2].push_back(v2[j]);
      j++; // Move the pointer in the second vector
    }
  }

  return ret;
}

inline std::string randomName() {
  std::uniform_int_distribution rchar(0,25);
  std::string ret = "tmp";
  for(int i = 0; i < 10; i++)
    ret.push_back('a'+rchar(rgen));
  return ret;
}

inline std::string runExternal(const std::string& program, const std::vector<std::string>& args, const std::string& input) {
  int in_pipe[2];  // Parent -> Child
  int out_pipe[2]; // Child -> Parent

  pipe(in_pipe);
  pipe(out_pipe);

  pid_t pid = fork();
  if (pid == 0) {
    // Child process

    // Redirect stdin
    dup2(in_pipe[0], STDIN_FILENO);
    close(in_pipe[0]);
    close(in_pipe[1]);

    // Redirect stdout
    dup2(out_pipe[1], STDOUT_FILENO);
    close(out_pipe[0]);
    close(out_pipe[1]);

    // Convert args to execvp-compatible format
    std::vector<char*> c_args;
    c_args.push_back(const_cast<char*>(program.c_str()));
    for (const auto& arg : args) {
      c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    c_args.push_back(nullptr);

    execvp(program.c_str(), c_args.data());
    // If execvp returns, it failed
    perror("c execvp failed");
    _exit(1);
  } else {
    // Parent process
    close(in_pipe[0]);  // Close unused read end
    close(out_pipe[1]); // Close unused write end

    // Write to child's stdin
    write(in_pipe[1], input.c_str(), input.size());
    close(in_pipe[1]); // Send EOF

    // Read from child's stdout
    std::string output;
    char buffer[256];
    ssize_t count;
    while ((count = read(out_pipe[0], buffer, sizeof(buffer))) > 0) {
      output.append(buffer, count);
      // for(int i = 0; i < count; i++)
      //   std::cout << buffer[i];
    }
    close(out_pipe[0]);

    // Wait for child to finish
    waitpid(pid, nullptr, 0);

    return output;
  }
}

//https://martin.ankerl.com/2018/12/08/fast-random-bool/
#define UNLIKELY(x) __builtin_expect((x), 0)
template <typename U = uint64_t> class RandomizerWithSentinelShift {
public:
  template <typename Rng> bool operator()(Rng &rng) {
    if (UNLIKELY(1 == m_rand)) {
      m_rand = std::uniform_int_distribution<U>{}(rng) | s_mask_left1;
    }
    bool const ret = m_rand & 1;
    m_rand >>= 1;
    return ret;
  }
private:
  static constexpr const U s_mask_left1 = U(1) << (sizeof(U) * 8 - 1);
  U m_rand = 1;
};
