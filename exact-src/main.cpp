// g++ -static -o ttmip-static -Ofast -Wall -std=c++20 ttmip.cpp libglpk.a
// #undef NDEBUG
#include "hypergraph.hpp"
#include "instance.hpp"
#include "reduce.hpp"
#include "conflict.hpp"
#include "tools.hpp"
#include "local.hpp"
#include "greedy.hpp"
#include "maxsat.hpp"
#include <iostream>

const double maxTime = 1800;
const std::string ANYTIME = "./anytime";

std::unordered_set<int> initialSolution(HyperGraph &hg, int fs, double timeLimit, double eachLimit, int inc = 0) {
  if(hg.countEdges() == 0)
    return {{}};

  HyperGraph bestHG;
  std::unordered_set<int> bestSol;

  for(int i = inc; elapsed() < timeLimit - 5; i++) { // Was timeLimit - 15
    if(i == 0)
      ;
    else if(i == 1)
      hg.leastFrequentFirst();
    else if(i == 2)
      hg.leastFrequentLast();
    else
      hg.shuffle();

    int anytimeTime = std::min(round(timeLimit - elapsed()), eachLimit);

    std::cout << "c Anytime SAT solver (maxtime = " << anytimeTime << "): " << std::flush;
    double t0 = elapsed();
    std::unordered_set<int> nsol = solveSAT(hg, t0 + anytimeTime, ANYTIME);
    std::cout << nsol.size() + fs << " in " << elapsed() - t0 << std::endl;
    
    if(bestSol.empty() || bestSol.size() > nsol.size()) {
      bestSol = nsol;
      bestHG = hg;
    }
  }

  hg = bestHG;

  return bestSol;
}

// New version
std::unordered_set<int> improve(HyperGraph &hg, int fs, const std::unordered_set<int> &gsol, double timeLimit) {
  if(hg.countEdges() == 0)
    return {};
  Local localSolver(hg, gsol);
  std::cout << "c Running local search with exact solver for " <<int(timeLimit - elapsed()) << " seconds" << std::endl;
  std::cout << "c Initial cost: " << localSolver.getSolution().size()+fs << std::flush;
  if(localSolver.trim())
    std::cout << " -> " << localSolver.getSolution().size()+fs;
  std::cout << std::endl;
  std::cout << "c time\tcost\tgain (main+alt)\tlimit\tsteps\tfail\tmain solver" << std::endl;

  int minlimit = 40, maxlimit = 4000;
  int limitGLPK = maxlimit, successGLPK = minlimit;
  int limit = minlimit;
  int iter = 0;
  int gain[4] = {0, 0, 0, 0}, steps[4] = {0,0,0,0}, failed[4] = {0,0,0,0};
  double time[4] = {0.0, 0.0, 0.0, 0.0};
  double t0 = elapsed();
  double testTime = t0;
  double totalTime = timeLimit - t0;
  int nogain = 0;

  if(maxlimit >= 3 * localSolver.getSolution().size())
    maxlimit = 3 * localSolver.getSolution().size() - 1;

  bool useSAT = false;
  for(iter = 0; ; iter++) {
    if(limit > limitGLPK) {
      useSAT = true;
      time[3] = 1000;
      if(iter%4 == 3)
        iter++;
    }

    double titer = elapsed();
    int g = 0;
    steps[iter%4]++;

    int adjustedlimit = limit;
    // If limit reaches twice the solution size, we get the whole graph
    if(limit > localSolver.getSolution().size()) {
      adjustedlimit = (limit + localSolver.getSolution().size()) / 2;
    }
    int delta = std::max(5,adjustedlimit / 20);
    int actualLimit = adjustedlimit + delta*(iter%4==2) - delta*(iter%4==0);

    if(iter%4 != 3) { // Normal run
      if(useSAT)
        g = localSolver.localStepSAT(actualLimit, 1.0 + std::min(nogain/3.0,2.5));
      else
        g = localSolver.localStep(actualLimit, .5);
    }
    else { // Inverse run
      if(!useSAT)
        g = localSolver.localStepSAT(actualLimit, 1.0 + std::min(nogain/3.0,2.5));
      else
        g = localSolver.localStep(actualLimit, .5);
    }

    if((useSAT && iter%4 == 3) || (!useSAT && iter%4 != 3)) {
      if(g != -1) {
        successGLPK = std::max(successGLPK, actualLimit);
      }
      else { // Once GLPK fails, disable it for values larger than previous success
        limitGLPK = std::min(limitGLPK, successGLPK);
      }
    }

    if(g == -1) {
      failed[iter%4]++;
      g = 0;
    }

    gain[iter%4] += g;
    time[iter%4] += elapsed() - titer;

    if(g && nogain > 4) {
      std::cout << "c Success with limit = " << actualLimit << " and took " << elapsed() - titer << std::endl;
    }


    int mainFailed = failed[0]+failed[1]+failed[2];
    int mainSteps = steps[0]+steps[1]+steps[2];

    bool premature = ( mainFailed == mainSteps && mainFailed >= 4);

    if(elapsed() > timeLimit)
      break;

    if(premature || elapsed() - testTime > 7.0) {
      int trimmed = localSolver.trim();
      if(trimmed)
        std::cout << "c \t\t" << trimmed << " +" << std::endl;
      int totalFailed = failed[0]+failed[1]+failed[2]+failed[3];
      int totalSteps = steps[0]+steps[1]+steps[2]+steps[3];
      int totalGain = gain[0]+gain[1]+gain[2]+gain[3]+trimmed;
      if(totalGain == 0)
        nogain++;

      std::cout << "c " << (int) elapsed() << "\t" << localSolver.getSolution().size() + fs << "\t" << gain[0]+gain[1]+gain[2] << "+" << gain[3] << "\t= " << totalGain << "\t" << limit << "\t" << totalSteps << "\t" << mainFailed << "+" << totalFailed-mainFailed << "=" << totalFailed << (useSAT ? "\tEvalMaxSAT":"\tGLPK") << std::endl;

      double efficiency[4] = {gain[0]/time[0], gain[1]/time[1], gain[2]/time[2], gain[3]/time[3]};

      if(!failed[3] && !failed[1] && steps[3] && steps[1]
        && time[3]/steps[3] < time[1]/ steps[1]) {
        useSAT = !useSAT;
      }
      else if(failed[1] + failed[3] > 0 && steps[1] && steps[3]
        && double(failed[3])/steps[3] < double(failed[1]) / steps[1]) {
        useSAT = !useSAT;
      }

      if(mainFailed && (gain[0] + gain[1] + gain[2] == 0)) {
        maxlimit = mainFailed == mainSteps ? limit - 15 : (limit+maxlimit) / 2;
        limit -= std::max(int(0.25*limit), 25);
      }
      else if(mainFailed) {
        limit -= std::max(int(0.25*limit), 25);
      }
      else if(!mainFailed && (gain[0] + gain[1] + gain[2] == 0)) { // No gain, no failure
        limit += std::max(int(0.3*limit), 30);
        maxlimit = std::max(maxlimit, limit);
      }
      else if(!mainFailed && (gain[0] == 0 || gain[1] == 0 || gain[2] == 0)) { // Gains are small
        limit += std::max(int(0.2*limit), 20);
        maxlimit = std::max(maxlimit, limit);
      }
      else if(efficiency[0] > 1.2 * efficiency[1] && efficiency[1] > 1.2 * efficiency[2]) // Small limit is much more efficient
        limit -= std::max(int(0.15*limit), 15);
      else if(efficiency[2] > 1.2 * efficiency[1] && efficiency[1] > 1.2 * efficiency[0]) // Large limit is much more efficient
        limit += std::max(int(0.15*limit), 15);
      else if(efficiency[0] > efficiency[1] && efficiency[1] > efficiency[2]) // Small limit is a little more efficient
        limit -= std::max(int(0.1*limit), 10);
      else if(efficiency[2] > efficiency[1] && efficiency[1] > efficiency[0]) // Large limit is a little more efficient
        limit += std::max(int(0.1*limit), 10);

      // Mix the solution with conflict optimizer
      if(gain[0]+gain[1]+gain[2]+gain[3] == 0) {
        Conflict conf(hg,localSolver.getSolution());
        if(conf.conflictStep(elapsed()+.1)) {
          std::cout << "c Conflict optimizer improved cost to " << conf.getSolution().size() + fs << std::endl;
        }

        // To check changes to the solution:
        // std::unordered_set<int> oldsol = localSolver.getSolution();
        // std::unordered_set<int> newsol = conf.getSolution();
        // std::vector<int> diff;
        // std::copy_if(oldsol.begin(), oldsol.end(), std::back_inserter(diff),
        //              [&newsol] (int needle) { return !newsol.contains(needle); });
        // std::copy_if(newsol.begin(), newsol.end(), std::back_inserter(diff),
        //              [&oldsol] (int needle) { return !oldsol.contains(needle); });
        // std::cout << "c Changed solution by " << diff.size() << std::endl;

        localSolver = Local(hg, conf.getSolution());
      }

      testTime = elapsed();
      limit = std::min(limit, maxlimit);
      limit = std::max(limit, minlimit);
      gain[0] = gain[1] = gain[2] = gain[3] = 0;
      steps[0] = steps[1] = steps[2] = steps[3] = 0;
      failed[0] = failed[1] = failed[2] = failed[3] = 0;
      time[0] = time[1] = time[2] = time[3] = 0.0;
    }
  }

  std::cout << "c Iterations: " << iter << std::endl;
  std::cout << "c Number of runs per edge: " << *std::min_element(localSolver.testedEdges.begin(), localSolver.testedEdges.end())  << " <= " << *std::max_element(localSolver.testedEdges.begin(), localSolver.testedEdges.end()) << std::endl;

  // gsol cannot be smaller, but it can be equal
  if(gsol.size() <= localSolver.getSolution().size())
    return gsol;

  return localSolver.getSolution();
}

// Old version
std::unordered_set<int> oldImprove(HyperGraph &hg, int fs, const std::unordered_set<int> &gsol, double timeLimit) {
  if(hg.countEdges() == 0)
    return {};
  Local localSolver(hg, gsol);
  std::cout << "c Local: " << localSolver.getSolution().size() + fs << std::flush;

  const int minlimit = 20, maxlimit = 2200;
  int limit = minlimit + 20;
  int iter = 0;
  int gain[3] = {0, 0, 0};
  double time[3] = {0.0, 0.0, 0.0};
  double t0 = elapsed();
  double testTime = t0;
  int failed = 0;
  bool improved = false;
  double totalTime = timeLimit - t0;
  double lastImproved = t0;

  int useSAT = 0;
  std::cout << " [" << limit << "]" << std::flush;
  while(elapsed() < timeLimit - 5 * useSAT) {
    double titer = elapsed();
    int g = 0;
    if(useSAT)
      g = localSolver.localStepSAT(4*(limit + 5*(iter%3==2) - 5 *(iter%3==0)));
    else
      g = localSolver.localStep(limit + 5*(iter%3==2) - 5 *(iter%3==0));
    if(g == -1) {
      failed++;
      std::cout << "!" << std::flush;
      g = 0;
    }
    gain[iter%3] += g;
    time[iter%3] += elapsed() - titer;

    if(elapsed() - testTime > 3.0 + 5.0 * useSAT) {
      if(gain[0]+gain[1]+gain[2] > 0) {
        std::cout << " -> " << localSolver.getSolution().size() + fs << std::flush;
        improved = true;
        lastImproved = elapsed();
        if(useSAT)
          useSAT++;
      }
      else {
        if(elapsed() - lastImproved > totalTime / 4) {
          if(!useSAT) {
            std::cout << " SAT " << std::flush;
            useSAT = true;
          }
        }
        if(elapsed() - lastImproved > totalTime / 2)
          break;
      }

      double efficiency[3] = {gain[0]/time[0], gain[1]/time[1], gain[2]/time[2]};
      if(!useSAT && (failed > 2 || limit == maxlimit)) {
        std::cout << " SAT " << std::flush;
        useSAT = true;
      }
      else if(failed > 1 && useSAT < 4)
        limit /= 2;
      else if(failed > 1)
        limit -= std::max(int(0.2*limit) - 10, 20);
      else if(!failed && (gain[0] + gain[1] + gain[2] == 0)) // Gains are very small
        limit += std::max(int(0.2*limit), 20);
      else if(!failed && (gain[0] == 0 || gain[1] == 0 || gain[2] == 0)) // Gains are very small
        limit += 10;
      else if(efficiency[0] > 1.2 * efficiency[1] && efficiency[1] > 1.2 * efficiency[2])
        limit -= 10;
      else if(efficiency[2] > 1.2 * efficiency[1] && efficiency[1] > 1.2 * efficiency[0])
        limit += 10;
      else if(efficiency[0] > efficiency[1] && efficiency[1] > efficiency[2])
        limit -= 5;
      else if(efficiency[2] > efficiency[1] && efficiency[1] > efficiency[0])
        limit += 5;

      limit = std::min(limit, maxlimit);
      limit = std::max(limit, minlimit);
      std::cout << " [" << limit << "]" << std::flush;
      gain[0] = gain[1] = gain[2] = 0;
      time[0] = time[1] = time[2] = 0.0;
      failed = 0;
      testTime = elapsed();
    }

    iter++;
  }

  std::cout << " -> " << localSolver.getSolution().size() + fs << std::endl;
  std::cout << "c Iterations: " << iter << std::endl;

  // gsol cannot be smaller, but it can be equal
  if(gsol.size() <= localSolver.getSolution().size())
    return gsol;
  return localSolver.getSolution();
}

std::vector<int> solveExact(HyperGraph &hg, int fs) {
  double t0 = elapsed();
  bool opt = false;
  {
    auto gsol = initialSolution(hg, fs, elapsed() + 30, 30);
    auto isol = improve(hg, fs, gsol, elapsed() + 180);
    MaxSAT solver(hg);

    std::cout << "c Running EvalMaxSAT for 500s with initial solution of cost " << isol.size() + fs << std::endl;
    // Will run until a solution is found if last parameter is false
    opt = solver.solveExact(500.0, 0.0, isol, true);
    if(opt)
      return solver.getSolutionLabels();
  }

  {
    auto gsol = initialSolution(hg, fs, elapsed() + 120, 30, 1);
    auto isol = improve(hg, fs, gsol, elapsed() + 180);
    MaxSAT solver(hg);

    std::cout << "c Time left:  " << maxTime - elapsed() << std::endl;
    std::cout << "c Starting EvalMaxSAT solver with initial solution of cost " << isol.size() + fs << std::endl;
    // Will run until a solution is found if last parameter is false
    opt = solver.solveExact(maxTime - elapsed(), 50.0, isol, false);
    if(opt)
      return solver.getSolutionLabels();
  }

    sleep(maxTime);
  return {};
}


int main(int argc, char **argv) {
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = [](int signum){};
  sigaction(SIGTERM, &action, NULL);

  std::string inputFile = argc >=2 ? argv[1] : "";
  std::string outputFile = argc >=3 ? argv[2] : "";
  Instance inst(inputFile, outputFile);

  inst.printStats();

  HyperGraph hg = inst;
  hg.leastFrequentLast();
  Reduce reducer(hg);
  std::vector<int> fixed = reducer.reduce(maxTime/3);
  hg.printStats();
  reduceComponents(hg, fixed, elapsed() + 90);
  std::cout << "c Fixed: " << fixed.size() << std::endl;
  hg.printStats();

  std::vector<int> sol;

  if(hg.countEdges()) {
    std::cout << "c We have " << maxTime - elapsed() << " seconds left" << std::endl;
    if(hg.maxEdgeDegree() != 2 || hg.countVertices() > 800) {
      if(sol.empty() && elapsed() < maxTime) {
        sol = solveExact(hg, fixed.size());
      }
    }
    else {
      double t0 = elapsed();
      MaxSAT solver(hg);
      std::cout << "c All hyperedges have degree 2" << std::endl;
      std::cout << "c Solving Vertex Cover with MCS solver for Independent Set" << std::endl;
      solver.solveVC(); // Will run forever unless a solution is found
      sol = solver.getSolutionLabels();
    }

    if(sol.empty()) {// No solution found
      std::cout << "c Optimal solution not found" << std::endl;
      exit(2);
    }
    else {
      std::cout << "c Found optimal solution!" << std::endl;
    }
  }
  else {
    std::cout << "c Found optimal solution because the instance was reduced to nothing" << std::endl;
  }

  std::cout << "c Cost: " << sol.size()  + fixed.size() << std::endl;
  std::cout << "c Time: " << elapsed() << std::endl;

  inst.saveSol(sol, fixed);

  return 0;
}
