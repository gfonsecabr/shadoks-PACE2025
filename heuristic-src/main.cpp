// #undef NDEBUG
#include "hypergraph.hpp"
#include "instance.hpp"
#include "reduce.hpp"
#include "conflict.hpp"
#include "tools.hpp"
#include "local.hpp"
#include <iostream>

const double maxTime = 315;
const std::string ANYTIME = "./anytime";

std::unordered_set<int> initialSolution(HyperGraph &hg, int fs, double timeLimit) {
  if(hg.countEdges() == 0)
    return {};

  HyperGraph bestHG;
  std::unordered_set<int> bestSol;

  for(int i = 0; elapsed() < timeLimit - 10; i++) { // Was timeLimit - 15
    if(i == 0)
      ;
    else if(i == 1)
      hg.leastFrequentFirst();
    else if(i == 2)
      hg.leastFrequentLast();
    else
      hg.shuffle();

    int anytimeTime = round(timeLimit - elapsed());

    if(hg.countVertices() < 5000)
      anytimeTime /= 2;

    std::cout << "c Anytime SAT solver (maxtime = " << anytimeTime << "): " << std::flush;
    double t0 = elapsed();
    std::unordered_set<int> nsol = solveSAT(hg, t0 + anytimeTime, ANYTIME);
    std::cout << nsol.size() + fs << " in " << elapsed() - t0 << std::endl;

    if(hg.countVertices() >= 5000 && hg.countVertices() < 200000)
      return nsol;

    if(bestSol.empty() || bestSol.size() > nsol.size()) {
      bestSol = nsol;
      bestHG = hg;
    }
  }

  hg = bestHG;

  return bestSol;
}

bool signaled = false;

std::vector<int> improve(HyperGraph &hg, int fs, const std::unordered_set<int> &gsol, double timeLimit) {
  if(hg.countEdges() == 0)
    return {};
  Local localSolver(hg, gsol);
  std::cout << "c Running local search with exact solver to the end" << std::endl;
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
    if(iter%4 == 3 && steps[3] > 100) // Enough for statistics
      iter++;

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

    // if(g && nogain > 4) {
    //   std::cout << "c Success with limit = " << actualLimit << " and took " << elapsed() - titer << std::endl;
    // }


    int mainFailed = failed[0]+failed[1]+failed[2];
    int mainSteps = steps[0]+steps[1]+steps[2];

    if(elapsed() > timeLimit)
      break;

    if(elapsed() > timeLimit - 15 || signaled)
      nogain = 0; // To finish faster

    bool premature = ( mainFailed == mainSteps && mainFailed >= 4);

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
        if(conf.conflictStep(elapsed()+.3)) {
          std::cout << "c Conflict optimizer improved cost to " << conf.getSolution().size() + fs << std::endl;
        }

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
    return std::vector<int>(gsol.begin(), gsol.end());

  return std::vector<int>(localSolver.getSolution().begin(), localSolver.getSolution().end());
}

void handler(int signum) {
  signaled = true;
}

int main(int argc, char **argv) {
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  // action.sa_handler = [](int signum){};
  action.sa_handler = handler;
  sigaction(SIGTERM, &action, NULL);

  std::string inputFile = argc >=2 ? argv[1] : "";
  std::string outputFile = argc >=3 ? argv[2] : "";
  Instance inst(inputFile, outputFile);

  inst.printStats();

  HyperGraph hg = inst;
  hg.leastFrequentLast(); // Helps reducing faster
  Reduce reducer(hg);
  std::vector<int> fixed = reducer.reduce(maxTime/3);
  hg.printStats();
  reduceComponents(hg, fixed, elapsed() + 70);
  std::cout << "c Fixed vertices: " << fixed.size() << std::endl;
  hg.printStats();

  std::vector<int> sol;

  if(hg.countEdges()) {
    double anytimeTime = elapsed() + 65;

    if(hg.countVertices() < 5000)
      anytimeTime += 15;
    else if(hg.countVertices() >= 5000 && hg.countVertices() < 200000)
      anytimeTime += 65;
    anytimeTime = std::min(3*maxTime/4, anytimeTime);
    auto gsol = initialSolution(hg, fixed.size(), anytimeTime);
    std::cout << "c We have " << maxTime - elapsed() << " seconds left" << std::endl;
    auto isol = improve(hg, fixed.size(), gsol, maxTime);
    sol = hg.getLabels(isol);
  }
  else {
    std::cout << "c Found optimal solution because the instance has been reduced to nothing" << std::endl;
  }

  std::cout << "c Cost: " << sol.size()  + fixed.size() << std::endl;
  std::cout << "c Time: " << elapsed() << std::endl;

  inst.saveSol(sol, fixed);

  if(!signaled && elapsed() < maxTime - 30) {
    sleep(maxTime - elapsed() - 15); // Use nearly all the time available
  }

  return 0;
}
