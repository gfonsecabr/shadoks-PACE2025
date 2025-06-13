#ifndef EVALMAXSAT_SLK178903R_H
#define EVALMAXSAT_SLK178903R_H


#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <random>
#include <queue>

#include "SolverMaxSAT_SCIP.h"
#include "utile.h"
#include "Chrono.h"
#include "coutUtil.h"
#include "cadicalinterface.h"
#include "cardincremental.h"
#include "rand.h"
// #include "mcqd.h"
#include "MCS.h"
#include "MCR.h"
#include "MCQ.h"
#include "Algorithm.h"

using namespace MaLib;

// [first1, last1) and [first2, last2) have to be sorted
template<class InputIt1, class InputIt2>
bool intersection_empty(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
{
    while (first1 != last1 && first2 != last2)
    {
        if (*first1 < *first2)
            ++first1;
        else
        {
            if (!(*first2 < *first1))
                return false;
            ++first2;
        }
    }
    return true;
}


class LocalOptimizer2 {
    Solver_cadical *solver;
    WeightVector poids;
    t_weight initialWeight;

    Chrono chronoSinceLastOptimize;

public:
    LocalOptimizer2(Solver_cadical *solver, const WeightVector &poids, const t_weight &initialWeight) :
        solver(solver), poids(poids), initialWeight(initialWeight) {

    }

    t_weight calculateCostAssign(const std::vector<bool> & solution) {
        t_weight coutSolution = 0;
        for(unsigned int lit=1; lit<poids.size(); lit++) {
            assert( lit < solution.size() );
            if( poids[lit] > 0 ) {
                if( solution[lit] == 0 ) {
                    coutSolution += poids[lit];
                }
            } else if ( poids[lit] < 0 ) {
                if( solution[lit] == 1 ) {
                    coutSolution += -poids[lit];
                }
            }
        }

        return coutSolution + initialWeight;
    }

    t_weight optimize(std::vector<bool> & solution)  {
        
        double timeout_sec = std::min(60.0, chronoSinceLastOptimize.tacSec() * 0.1 );
        chronoSinceLastOptimize.tic();

        MonPrint("c optimize for ", timeout_sec, " sec");

        std::vector< std::tuple<t_weight, int> > softVarFalsified;
        std::vector< int > assumSatisfied;
        for(unsigned int lit=1; lit<poids.size(); lit++) {
            assert( lit < solution.size() );
            if( poids[lit] > 0 ) {
                assert(lit < solution.size() );
                if( solution[lit] == 0 ) {
                    softVarFalsified.push_back( {poids[lit], lit} );
                } else {
                    assumSatisfied.push_back(lit);
                }
            } else if( poids[lit] < 0 ) {
                assert(lit < solution.size() );
                if( solution[lit] == 1 ) {
                    softVarFalsified.push_back( {poids[-lit], -lit} );
                } else {
                    assumSatisfied.push_back(-lit);
                }
            }
        }

        bool modify = false;
        for(int id=softVarFalsified.size()-1; id >= 0; id--) {
            assumSatisfied.push_back( std::get<1>(softVarFalsified[id]) );
            if( solver->solveLimited(assumSatisfied, timeout_sec / (double)(softVarFalsified.size()+1) ) != 1 ) {
                assumSatisfied.pop_back();
            } else {
                modify = true;
            }
        }

        if(modify) {
            auto tmp = solver->solve(assumSatisfied);
            assert(tmp == 1);
            solution = solver->getSolution();
        }

        return calculateCostAssign(solution);
    }

};

template<class SAT_SOLVER=Solver_cadical>
class EvalMaxSAT {

    ///////////////////////////
    /// Representation of the MaxSAT formula
    ///
        SAT_SOLVER *solver = nullptr;
        WeightVector _poids; // _poids[lit] = weight of lit
        std::map<t_weight, std::set<int>> _mapWeight2Assum; // _mapWeight2Assum[weight] = set of literals with this weight
        t_weight cost = 0;

        struct LitComp {
            bool operator()(const int &a, const int &b) const {
                return abs(a) < abs(b);
            }
        };

        struct LitCard {
            std::shared_ptr<CardIncremental_Lazy<EvalMaxSAT<SAT_SOLVER>> > card;
            int atMost;
            t_weight initialWeight;

            LitCard(std::shared_ptr<CardIncremental_Lazy<EvalMaxSAT<SAT_SOLVER>>> card, int atMost, t_weight initialWeight) : card(card), atMost(atMost), initialWeight(initialWeight) {
                assert( card != nullptr );
                assert(atMost > 0);
                assert(initialWeight > 0);
            }

            int getLit() const {
                return card->atMost(atMost);
            }
        };
        std::vector< std::optional<LitCard> > _mapAssum2Card; // _mapAssum2Card[lit] = card associated to lit

        std::deque< std::tuple< std::vector<int>, t_weight> > _cardToAdd;   // cardinality to add
        std::vector<int> _litToRelax; // Lit to relax
    ///
    //////////////////////////

    ///////////////////////////
    /// Best solution found so far
    ///
        std::vector<bool> solution;
        t_weight solutionCost = std::numeric_limits<t_weight>::max();
    ///
    //////////////////////////
private:
    ///////////////////////////
    /// Hyperparameters
    ///
        double _initialCoef = 10;
        double _base = 400;
        unsigned int timeout_SCIP = 500;
        double _minimalRefTime = 5;
        double _maximalRefTime = 5*60;
        TimeOut totalSolveTimeout = TimeOut(0.9 * (1800 - timeout_SCIP) );

        bool _delayStrategy = true;
        bool _multiSolveStrategy = true;
        bool _UBStrategy = true;
        bool _saveCoresStrategy = true;
    ///
    //////////////////////////////

        TimeOut globalTimeOut = 1e9;

    double getTimeRefCoef() {
        if(totalSolveTimeout.getCoefPast() >= 1)
            return 0;
        return _initialCoef * pow(_base, -totalSolveTimeout.getCoefPast());
    }

public:

    ///////////////////////////
    /// Setter
    ///
        void setCoef(double initialCoef, double coefOnRefTime) {
            auto W = [](double x){
                // Approximation de la fonction W de Lambert par la série de Taylor
                double result = x;
                x *= x;
                result += -x;
                x *= x;
                result += 3*x/2.0;
                x *= x;
                result += -8*x/3.0;
                x *= x;
                result += 125*x/24.0;
                return result;
            };

            _initialCoef = initialCoef;
            _base = (-initialCoef / ( coefOnRefTime * W( (-initialCoef * std::exp(-initialCoef/coefOnRefTime)) / coefOnRefTime ) ));
        }
        void setTargetComputationTime(double targetComputationTime, unsigned int timeForSCIP = 60) {
            timeout_SCIP = timeForSCIP;
            totalSolveTimeout = TimeOut( (targetComputationTime - timeout_SCIP) * 0.9 );
        }
        void setBoundRefTime(double minimalRefTime, double maximalRefTime) {
            _minimalRefTime = minimalRefTime;
            _maximalRefTime = maximalRefTime;
        }
        void unactivateDelayStrategy() {
            _delayStrategy = false;
        }
        void unactivateMultiSolveStrategy() {
            _multiSolveStrategy = false;
        }
        void unactivateUBStrategy() {
            _UBStrategy = false;
        }
        void setSaveCores(bool saveCores) {
            _saveCoresStrategy = saveCores;
        }
    ///
    ////////////////////////


    public:
    EvalMaxSAT() : solver(new SAT_SOLVER) {
        _poids.push_back(0);          // Fake lit with id=0
        _mapAssum2Card.push_back({});  // Fake lit with id=0
    }

    ~EvalMaxSAT() {
        delete solver;
    }
public:
    void printInfo() {
        std::cout << "c Time for SCIP = " << timeout_SCIP << " sec" << std::endl;
        std::cout << "c Stop unsat improving after " << totalSolveTimeout.getVal() << " sec" << std::endl;
        std::cout << "c Minimal ref time = " << _minimalRefTime << " sec" << std::endl;
        std::cout << "c Initial coef = " << _initialCoef << std::endl;
    }

    bool isWeighted() {
        return _mapWeight2Assum.size() > 1;
    }

    int newVar(bool decisionVar=true) {
        _poids.push_back(0);
        _mapAssum2Card.push_back({});

        int var = solver->newVar(decisionVar);

        assert(var == _poids.size()-1);

        return var;
    }

    int newSoftVar(bool value, long long weight) {
        if(weight < 0) {
            ///// TODO: ajouer Offset_Cost -= weight
            value = !value;
            weight = -weight;
        }

        _poids.push_back(weight * (((int)value)*2 - 1));
        _mapAssum2Card.push_back({});
        _mapWeight2Assum[weight].insert( (((int)value*2) - 1) * ((int)(_poids.size())-1) );

        int var = solver->newVar();
        assert(var == _poids.size()-1);

        return var;
    }

    int addClause(const std::vector<int> &clause, std::optional<long long> w = {}) {
        if( w.has_value() == false ) { // Hard clause
            if( (clause.size() == 1) && ( _poids[clause[0]] != 0 ) ) {
                if( _poids[clause[0]] < 0 ) {
                    assert( _mapWeight2Assum[ _poids[-clause[0]] ].count( -clause[0] ) );
                    _mapWeight2Assum[ _poids[-clause[0]] ].erase( -clause[0] );
                    if( _mapWeight2Assum[ _poids[-clause[0]] ].size() == 0 ) {
                        _mapWeight2Assum.erase( _poids[-clause[0]] );
                    }
                    cost += _poids[-clause[0]];
                    MonPrint("c cost += ", _poids[-clause[0]], "      (cost = ", cost, ")");
                    relax(clause[0]);
                } else {
                    assert( _mapWeight2Assum[ _poids[clause[0]] ].count( clause[0] ) );
                    _mapWeight2Assum[ _poids[clause[0]] ].erase( clause[0] );
                    if( _mapWeight2Assum[ _poids[clause[0]] ].size() == 0 ) {
                        _mapWeight2Assum.erase( _poids[clause[0]] );
                    }
                }
                _poids.set(clause[0], 0);
            }

            solver->addClause(clause);
        } else {
            if(w.value() == 0)
                return 0;
            if(clause.size() > 1) { // Soft clause, i.e, "hard" clause with a soft var at the end
                if( w.value() > 0 ) {
                    auto softVar = newSoftVar(true, w.value());
                    std::vector<int> softClause = clause;
                    softClause.push_back( -softVar );
                    addClause(softClause);
                    assert(softVar > 0);
                    return softVar;
                } else {
                    assert(w.value() < 0);
                    auto softVar = newSoftVar(true, -w.value());
                    for(auto lit: clause) {
                        addClause({ -softVar, lit });
                    }
                    assert(softVar > 0);
                    return softVar;
                }
            } else if(clause.size() == 1) { // Special case: unit clause.
                addWeight(clause[0], w.value());
            } else { assert(clause.size() == 0); // Special case: empty soft clause.
                cost += w.value();
                MonPrint("c cost += ", w.value(), "      (cost = ", cost, ")");
            }
        }
        return 0;
    }


    bool getValue(int lit) {
        if(abs(lit) > solution.size())
            return false;
        if(lit>0)
            return solution[lit];
        if(lit<0)
            return !solution[-lit];
        assert(false);
        return 0;
    }


private:


    bool solveWithSCIP() {
        SolverMaxSAT_SCIP SCIP_solver;

        for(int var=1; var<_poids.size(); var++) {
            if(_poids[var] == 0) {
                auto tmp = SCIP_solver.newVar();
                assert(tmp == var);
            } else {
                assert( _mapWeight2Assum[ abs(_poids[var]) ].count( var ) || _mapWeight2Assum[ abs(_poids[var]) ].count( -var ) );
                auto tmp = SCIP_solver.newSoftVar(true, _poids[var]);
                assert(tmp == var);
            }
        }

        solver->exportClauses(SCIP_solver);

        // relaxer les card
        for(unsigned int var=1; var<_mapAssum2Card.size(); var++) {
            if(_mapAssum2Card[var].has_value()) {
                assert(false); // Car on lance avant de commencer la résolution
                SCIP_solver.addAtMost( _mapAssum2Card[var]->card->getClause(), 1, _mapAssum2Card[var]->initialWeight );
            }
        }

        auto s = SCIP_solver.solveLimited(timeout_SCIP, solutionCost);

        if(s != 0) {
            t_weight recalculerCout = 0;
            std::vector<bool> SCIPsolution;
            SCIPsolution.push_back(0); // fake
            for(unsigned int var=1; var<_poids.size(); var++) {
                if( SCIP_solver.getValue(var) ) {
                    SCIPsolution.push_back(true);
                    if( _poids[var] < 0 ) {
                        recalculerCout += -_poids[var];
                    }
                } else {
                    SCIPsolution.push_back(false);
                    if( _poids[var] > 0 ) {
                        recalculerCout += _poids[var];
                    }
                }
            }

            if(solver->solve(SCIPsolution) == false) {
                return false;
            }

            if( recalculerCout + cost < solutionCost ) {
                solution = SCIPsolution;
                solutionCost = recalculerCout + cost;

                for(unsigned int var=1; var<_poids.size(); var++) {
                    if( SCIP_solver.getValue(var) ) {
                        assert( getValue(var) == true );
                    } else {
                        assert( getValue(var) == false );
                    }
                }
            }

            return s == 1; // Is optimal ?
        }

        return false;
    }

    std::set<int, LitComp> initAssum(t_weight minWeightToConsider=1) {
        std::set<int, LitComp> assum;

        for(auto itOnMapWeight2Assum = _mapWeight2Assum.rbegin(); itOnMapWeight2Assum != _mapWeight2Assum.rend(); ++itOnMapWeight2Assum) {
            if(itOnMapWeight2Assum->first < minWeightToConsider)
                break;
            for(auto lit: itOnMapWeight2Assum->second) {
                assum.insert(lit);
            }
        }
        return assum;
    }

    bool extract_disjoint_cores(std::set<int, LitComp> &assum, t_weight &minWeightToConsider, LocalOptimizer2 &LO) {
        int resultLastSolve=0;

        assert( solver->solve(assum) == 0 );

        std::vector< std::tuple< std::vector<int>, double> > cores;
        for(;;) {
            if(resultLastSolve == 0) { // UNSAT
                double maxLastSolveOr1 = std::min<double>(_minimalRefTime + solver->getTimeUsedToSolve(), _maximalRefTime);

                auto conflict = solver->getConflict(assum);
                MonPrint("conflict size = ", conflict.size(), " trouvé en ", solver->getTimeUsedToSolve(), "sec, donc maxLastSolveOr1 = ", maxLastSolveOr1);

                if(conflict.size() == 0) {
                    MonPrint("Fin par coupure !");
                    assert(solver->solve() == 0);
                    return false;
                }

                // 1. find better core : seconds solve
                unsigned int nbSolve=0;
                unsigned int lastImprove = 0;
                double bestP = std::numeric_limits<double>::max();
                if(_multiSolveStrategy && (conflict.size() > 3)) {
                    Chrono C;
                    std::vector<int> vAssum(assum.begin(), assum.end());
                    while( ( nbSolve < 3*lastImprove ) || (nbSolve < 20) || ( (C.tacSec() < 0.1*maxLastSolveOr1*getTimeRefCoef()) && (nbSolve < 1000) ) ) {
                        nbSolve++;

                        auto tmp = solver->getConflict(assum);
                        auto p = oneMinimize(tmp, maxLastSolveOr1 * getTimeRefCoef() * 0.1, 10, true);

                        if(_saveCoresStrategy) {
                            if(tmp.size() < 50 && p <= bestP*2) {
                                cores.push_back({tmp, p});
                                std::sort( std::get<0>(cores.back()).begin(), std::get<0>(cores.back()).end() );
                            }
                        }

                        if(p<bestP) {
                            bestP = p;
                            conflict = tmp;
                            MonPrint("Improve conflict at the ", nbSolve, "th solve = ", conflict.size(),  " (p = ", p , ")");
                            if(conflict.size() <= 2) {
                                break;
                            }
                            lastImprove = nbSolve;
                        }

                        if(C.tacSec() >= maxLastSolveOr1*getTimeRefCoef() ) {
                            MonPrint("Stop second solve after ", nbSolve, "th itteration. (nb=",conflict.size(),"). (", C.tacSec(), " > ", maxLastSolveOr1, " * ", getTimeRefCoef(), ")");
                            break;
                        }

                        if(isWeighted() && nbSolve == 1) {
                            std::sort(vAssum.begin(), vAssum.end(), [&](int a, int b){
                                if(_poids[a] == _poids[b])
                                    return abs(a) < abs(b);
                                return abs(_poids[a]) > abs(_poids[b]);
                            });
                        } else {
                            MaLib::MonRand::shuffle(vAssum);
                        }

                        auto res = solver->solveWithTimeout(vAssum, std::min(checkGlobalTimeout(), maxLastSolveOr1 * getTimeRefCoef()));
                        if(res==-1) {
                            MonPrint("Stop second solve because solveWithTimeout. (nb=",conflict.size(),").");
                            break;
                        }
                    }
                    MonPrint("Stop second solve after ", nbSolve, " itteration in ", C.tacSec(), " sec. (nb=",conflict.size(),").");
                }

                if(_saveCoresStrategy) {
                    for(auto &[savedCore, p]: cores) {
                        if(p < bestP) {
                            bestP = p;
                            conflict = savedCore;
                        }
                    }
                }

                // 2. minimize core
                if(nbSolve>1) {
                    if(conflict.size() > 2) {
                        oneMinimize(conflict, maxLastSolveOr1*getTimeRefCoef(), 1000, false);
                    }
                }

                if(conflict.size() == 0) {
                    return false;
                }

                // 2.5. Mise a jours des cores
                if(_saveCoresStrategy) {
                    for(unsigned int c=0; c<cores.size(); ) {
                        std::vector<int> & core = std::get<0>(cores[c]);
                        if(intersection_empty(core.begin(), core.end(), conflict.begin(), conflict.end()) ) {
                            c++;
                        } else {
                            cores[c] = cores.back();
                            cores.pop_back();
                        }
                    }
                }

                // 3. replace assum by card
                long long minWeight = std::numeric_limits<long long>::max();
                for(auto lit: conflict) {
                    // assert(_poids[lit] > 0); // This assertion fails!
                    if( _poids[lit] < minWeight ) {
                        minWeight = _poids[lit];
                    }
                }
                // assert(minWeight>0); // This assertion fails!
                for(auto lit: conflict) {
                    _mapWeight2Assum[_poids[lit]].erase( lit );
                    if( _mapWeight2Assum[_poids[lit]].size() == 0 ) {
                        _mapWeight2Assum.erase(_poids[lit]);
                    }
                    _poids.add(lit, -minWeight);
                    if(_poids[lit] == 0) {
                        if(_mapAssum2Card[ abs(lit) ].has_value()) {
                            //_litToRelax.push_back(lit);

                            auto newLit = relax(lit);
                            if(newLit.has_value()) {
                                if( _poids[newLit.value()] >= minWeightToConsider ) {
                                    assum.insert(newLit.value());
                                }
                            }
                            
                        }
                    } else {
                        _mapWeight2Assum[_poids[lit]].insert( lit );
                    }
                    // assert(_poids[lit] >= 0);

                    if(_poids[lit] < minWeightToConsider) {
                        assum.erase(lit);
                    }
                }

                for(auto& l: conflict) {
                    l *= -1;
                }
                _cardToAdd.push_back( {conflict, minWeight} );
                MonPrint("cost = ", cost, " + ", minWeight);
                cost += minWeight;
                if(cost == solutionCost) {
                    MonPrint("c UB == LB");
                    return false;
                }
                if(_mapWeight2Assum.size()) {
                    MonPrint(_mapWeight2Assum.rbegin()->first, " >= ", solutionCost, " - ", cost, " (", solutionCost - cost, ") ?");
                    if( _mapWeight2Assum.rbegin()->first >= solutionCost - cost) {
                        MonPrint("c Déchanchement de Haden !");
                        if(harden(assum)) {
                            //assert(_mapWeight2Assum.size());
                            if(adapt_am1_VeryFastHeuristic()) {
                                assum = initAssum(minWeightToConsider);
                            }
                        }
                    }
                }
            } else if(resultLastSolve == 1) { // SAT
                if(_litToRelax.size()) { // TODO: relaxer directement ?
                    for(auto lit: _litToRelax) {
                        auto newLit = relax(lit);
                        if(newLit.has_value()) {
                            if( _poids[newLit.value()] >= minWeightToConsider ) {
                                assum.insert(newLit.value());
                            }
                        }
                    }
                    _litToRelax.clear();
                    assert( assum == initAssum(minWeightToConsider) );
                } else {
                    ///////////////////
                    /// For harden via optimize
                    ///
                        if(_UBStrategy && toOptimize) {
                            MonPrint("c SAT, mais pas formecement optimal, on optimize la solution !");
                            auto curSolution = solver->getSolution();
                            auto curCost = LO.optimize( curSolution );

                            if(curCost < solutionCost) {
                                // std::cout << "c o " << curCost << std::endl;
                                solutionCost = curCost;
                                solution = curSolution;

                                if(harden(assum)) {
                                    if(adapt_am1_VeryFastHeuristic()) {
                                        assum = initAssum(minWeightToConsider);
                                    }
                                }
                            }
                            //chronoLastOptimize.tic();
                        }
                    ///
                    ////////////////

                    break;
                }
            } else { // TIMEOUT
                minWeightToConsider=1;
                assum = initAssum(minWeightToConsider);
                break;
            }

            if(_delayStrategy == false) {
                break;
            }

            MonPrint("Limited SAT...");
            {
                /*
                if(_mapWeight2Assum.size() <= 1) {
                    resultLastSolve = solver->solveWithTimeout(assum, std::min(checkGlobalTimeout(), 60.0));
                } else {
                    resultLastSolve = solver->solve(assum);
                }
                */
                resultLastSolve = solver->solveWithTimeout(assum, std::min(checkGlobalTimeout(), 60.0) * _mapWeight2Assum.size());
                MonPrint("resultLastSolve = ", resultLastSolve);
            }
        }

        return true;
    }
    
    bool toOptimize = true;
public:
    void disableOptimize() {
        toOptimize = false;
    }

    double checkGlobalTimeout() {
        double t = globalTimeOut.timeLeft();
        if(t < 0)
            throw std::exception();
        return t;
    }

    int solve() {
        return solveWithTimeout(1e9);
    }

    // Returns -1 on timeout
    int solveWithTimeout(double to) {
        globalTimeOut = TimeOut(to);

        try {
            totalSolveTimeout.restart();

            MonPrint("c initial cost = ", cost);
            std::set<int, LitComp> assum;

            //Chrono chronoLastSolve;
            Chrono chronoLastOptimize;

            MonPrint("c test cost = ", cost);
            // std::cout << "a" << std::flush;
            adapt_am1_exact();
            // std::cout << "b" << std::flush;
            adapt_am1_FastHeuristicV7();
            // std::cout << "c" << std::flush;
            MonPrint("c test cost = ", cost);

            if(cost >= solutionCost) {
                return true;
            }

            if(_mapWeight2Assum.size() == 0) {
                double timeLeft = checkGlobalTimeout();
                int result = solver->solveWithTimeout(timeLeft);
                if(result == -1)
                    throw(std::exception());
                if(result == 1) {
                    solutionCost = cost;
                    solution = solver->getSolution();
                    return true;
                }

                return false;
            }
            // std::cout << "d" << std::flush;

            if(harden(assum)) {
                // std::cout << "e" << std::flush;
                assert(_mapWeight2Assum.size());
                if(adapt_am1_VeryFastHeuristic()) {
                    if(cost >= solutionCost) {
                        return true;
                    }
                }
            }
            // std::cout << "f" << std::flush;

            ///////////////////
            /// USING SCIP
            ///
            if(_mapWeight2Assum.size()) {
                // std::cout << "c using SCIP..." << std::endl;
                if(_mapWeight2Assum.rbegin()->first < 500000000 )
                    if(_poids.size() <= 100000) {
                            totalSolveTimeout.pause(true);
                            if(timeout_SCIP > 0) {
                                auto save_solutionCost = solutionCost;
                                if(solveWithSCIP()) {
                                    MonPrint("SCIP a trouvé une solution :)");
                                    return true;
                                }
                                if(save_solutionCost != solutionCost) {
                                    assert( save_solutionCost > solutionCost );
                                    MonPrint("SCIP a améliorer la UB !! (newval = ", solutionCost ,") :)");


                                    if(harden(assum)) {
                                        assert(_mapWeight2Assum.size());
                                        if(adapt_am1_VeryFastHeuristic()) {
                                            if(cost >= solutionCost) {
                                                return true;
                                            }
                                        }
                                    }
                                }
                                MonPrint("Fin SCIP");
                            }
                            totalSolveTimeout.pause(false);
                    }
                    // std::cout << "c Fin SCIP..." << std::endl;
                }
            //
            /////////////////

            // std::cout << "g" << std::flush;

            ///////////////////
            /// For harden via optimize
            ///
                assert(_cardToAdd.size() == 0);
                assert(_litToRelax.size() == 0);
                assert( [&](){
                    for(auto e: _mapAssum2Card) {
                        if(e.has_value())
                            return false;
                    }
                    return true;
                }() );

                LocalOptimizer2 LO(solver, _poids, cost);
            //
            /////////////////

            // std::cout << "h" << std::flush;


            t_weight minWeightToConsider=1;
            if(_mapWeight2Assum.size() > 0) {
                minWeightToConsider = chooseNextMinWeight( _mapWeight2Assum.rbegin()->first + 1 );
            }
            // std::cout << "i" << std::flush;


            // Initialize assumptions
            assum = initAssum(minWeightToConsider);

            //std::cout << "c o " << solutionCost << std::endl;
            // std::cout << "j" << std::flush;

            MonPrint("c initial2 cost = ", cost);

            int resultLastSolve;
            for(;;) {
                // std::cout << "L" << std::flush;

                MonPrint("Full SAT...");
                assert( _cardToAdd.size() == 0 );
                assert( _litToRelax.size() == 0 );

                if(cost >= solutionCost) {
                    return true;
                }

                //chronoLastSolve.tic();
                {
                    assert( assum == initAssum(minWeightToConsider) );
                    double timeLeft = checkGlobalTimeout();

                    resultLastSolve = solver->solveWithTimeout(assum,timeLeft);
                    if(resultLastSolve == -1)
                        throw(std::exception());
                    MonPrint("resultLastSolve = ", resultLastSolve);
                }
                //chronoLastSolve.pause(true);
                if(resultLastSolve) { // SAT
                    // std::cout << "m" << std::flush;
                    if(minWeightToConsider == 1) {
                        solutionCost = cost;
                        solution = solver->getSolution();
                        // std::cout << "c o " << solutionCost << std::endl;
                        return true; // Solution found
                    }

                    minWeightToConsider = chooseNextMinWeight(minWeightToConsider);
                    assum = initAssum(minWeightToConsider);
                } else { // UNSAT
                    // std::cout << "n" << std::flush;

                    MonPrint("c cost pres extract_disjoint_cores = ", cost);
                    if(!extract_disjoint_cores(assum, minWeightToConsider, LO)) {
                        return solutionCost != std::numeric_limits<t_weight>::max();
                    }

                    for(auto lit: _litToRelax) {
                        auto newLit = relax(lit);
                        if(newLit.has_value()) {
                            if( _poids[newLit.value()] >= minWeightToConsider ) {
                                assum.insert(newLit.value());
                            }
                        }
                    }
                    _litToRelax.clear();
                    // std::cout << "o" << std::flush;

                    for(auto c: _cardToAdd) {
                        std::shared_ptr<CardIncremental_Lazy<EvalMaxSAT<SAT_SOLVER>>> card = std::make_shared<CardIncremental_Lazy<EvalMaxSAT<SAT_SOLVER>>>(this, std::get<0>(c), 1);
                        int newAssumForCard = card->atMost(1);
                        if(newAssumForCard != 0) {
                            assert( _poids[newAssumForCard] == 0 );

                            _poids.set(newAssumForCard, std::get<1>(c));
                            _mapWeight2Assum[ std::get<1>(c) ].insert(newAssumForCard);
                            _mapAssum2Card[ abs(newAssumForCard) ] = LitCard(card, 1, std::get<1>(c));

                            if( _poids[newAssumForCard] >= minWeightToConsider ) {
                                assum.insert(newAssumForCard);
                            }
                        }
                    }
                    _cardToAdd.clear();
                }


                checkGlobalTimeout();
            }
        }
        catch (std::exception i) {
            return -1; // Timeout
        }

        assert(false);
        return true;
    }


    private:

    int harden(std::set<int, LitComp> &assum) {
        if(_mapWeight2Assum.size() == 0)
            return 0;
        if(_mapWeight2Assum.size()==1) {
            if(_mapWeight2Assum.begin()->first == 1) {
                return 0;
            }
        }

        int nbHarden = 0;
        t_weight maxCostLit = solutionCost - cost;


        while( _mapWeight2Assum.rbegin()->first >= maxCostLit ) {
            assert(_mapWeight2Assum.size());

            auto lits = _mapWeight2Assum.rbegin()->second;

            for(auto lit: lits) {
                assert( _poids[lit] >= maxCostLit );
                addClause({lit});
                assum.erase(lit);
                nbHarden++;
            }

            if(_mapWeight2Assum.size() <= 1) {
                break;
            }

            if(_mapWeight2Assum.rbegin()->second.size() == 0) {
                _mapWeight2Assum.erase(prev(_mapWeight2Assum.end()));
                if(_mapWeight2Assum.size() == 0) {
                    break;
                }
            }
        }

        MonPrint("c NUMBER HARDEN : ", nbHarden);

        return nbHarden;
    }


    double oneMinimize(std::vector<int>& conflict, double referenceTimeSec, unsigned int B=10, bool shuffle=true) {
        Chrono C;
        double minimize=0;
        double noMinimize=0;
        unsigned int nbRemoved=0;

        if(isWeighted()) {
            std::sort(conflict.begin(), conflict.end(), [&](int litA, int litB){

                assert(_poids[litA] >= 0);
                assert(_poids[litB] >= 0);

                if(_poids[ litA ] > _poids[ litB ])
                    return true;
                if(_poids[ litA ] < _poids[ litB ]) {
                    return false;
                }
                return abs(litA) < abs(litB);
            });
        }

        if(conflict.size()==0) {
            return 0;
        }
        //while( solver->solveLimited(conflict, 10*B, conflict.back()) == 0 ) {
        while( solver->solveWithTimeoutAndLimit(conflict, std::min(checkGlobalTimeout(), referenceTimeSec*getTimeRefCoef() - C.tacSec()), 10*B, conflict.back()) == 0 ) {
            conflict.pop_back();
            minimize++;
            nbRemoved++;
            if(conflict.size()==0) {
                return 0;
            }

            if((C.tacSec() > referenceTimeSec*getTimeRefCoef())) {
                MonPrint("\t\tbreak minimize");
                return conflict.size() / (double)_poids[ conflict.back() ];
            }
        }

        if((C.tacSec() > referenceTimeSec*getTimeRefCoef())) {
            MonPrint("\t\tbreak minimize");
            return conflict.size() / (double)_poids[ conflict.back() ];
        }

        t_weight weight = _poids[conflict.back()];
        // assert(weight>0);
        if(shuffle) {
            MaLib::MonRand::shuffle(conflict.begin(), conflict.end()-1);
        }

        for(int i=conflict.size()-2; i>=0; i--) {
            //switch(solver->solveLimited(conflict, B, conflict[i])) {
            switch(solver->solveWithTimeoutAndLimit(conflict, std::min(checkGlobalTimeout(), referenceTimeSec*getTimeRefCoef() - C.tacSec()), B, conflict[i])) {
            case -1: // UNKNOW
                [[fallthrough]];
            case 1: // SAT
            {
                noMinimize++;
                break;
            }

            case 0: // UNSAT
                conflict[i] = conflict.back();
                conflict.pop_back();
                minimize++;
                nbRemoved++;
                break;

            default:
                assert(false);
            }

            if( C.tacSec() > referenceTimeSec*getTimeRefCoef() ) {
                break;
            }
        }

        return conflict.size() / (double)weight;
    }


    //////////////////////////////
    /// For extractAM
    ///
        void extractAM() {
            adapt_am1_exact();
            adapt_am1_FastHeuristicV7();
        }

        bool adapt_am1_exact() {
            Chrono chrono;
            unsigned int nbCliqueFound=0;
            std::vector<int> assumption;

            for(auto & [w, lits]: _mapWeight2Assum) {
                assert(w != 0);
                for(auto lit: lits) {
                    assert( _poids[lit] > 0 );
                    assumption.push_back(lit);
                }
            }

            if(assumption.size() > 30000) { // hyper paramétre
                MonPrint("skip");
                return false;
            }

            MonPrint("Create graph for searching clique...");
            unsigned int size = assumption.size();

            std::vector< std::vector <char> > conn;
            conn.resize(size);

            for(unsigned int v = 0; v < size; v++) {
                conn[v].resize(size);
            }

            // bool **conn = new bool*[size];
            // for(unsigned int i=0; i<size; i++) {
            //     conn[i] = new bool[size];
            //     for(unsigned int x=0; x<size; x++)
            //         conn[i][x] = false;
            // }

            MonPrint("Create link in graph...");
            for(unsigned int i=0; i<size; ) {
                checkGlobalTimeout();
                int lit1 = assumption[i];

                std::vector<int> prop;
                // If literal in assumptions has a value that is resolvable, get array of all the other literals that must have
                // a certain value in consequence, then link said literal to the opposite value of these other literals in graph

                if(solver->propagate(std::vector<int>({lit1}), prop)) {
                    for(int lit2: prop) {
                        for(unsigned int j=0; j<size; j++) {
                            if(j==i)
                                continue;
                            if(assumption[j] == -lit2) {
                                conn[i][j] = true;
                                conn[j][i] = true;
                            }
                        }
                    }
                    i++;
                } else { // No solution - Remove literal from the assumptions and add its opposite as a clause
                    addClause({-lit1});

                    assumption[i] = assumption.back();
                    assumption.pop_back();

                    for(unsigned int x=0; x<size; x++) {
                        conn[i][x] = false;
                        conn[x][i] = false;
                    }

                    size--;
                }
            }

            if(size == 0) {
                return true;
            }

            std::vector<bool> active(size, true);
            for(;;) {
                checkGlobalTimeout();

                // int *qmax;
                // int qsize=0;
                // Maxclique md(conn, size, 0.025);
                int m = 0;
                for(const auto &v : conn)
                    for(char c : v)
                        m += c;

                double density = (double) m / size / size;
                // std::cerr << size << ' ' << m << ' ' << density <<std::flush;
                // Chrono t;
                // std::unique_ptr<Algorithm> pAlgorithm(new MCQ(conn));
                // std::unique_ptr<Algorithm> pAlgorithm(new MCS(conn));
                std::unique_ptr<Algorithm> pAlgorithm = density > .1 ? make_unique<MCS>(conn) : make_unique<MCQ>(conn);
                pAlgorithm->SetTimeout(checkGlobalTimeout());

                // std::cout<<"x"<<std::flush;
                std::list<std::list<int>> iset;

                pAlgorithm->Run(iset);
                // t.pause();
                // std::cerr << ' ' << t.tacSec() << std::endl;
                // iset.clear();
                // Chrono t2;
                // pAlgorithm2->Run(iset);
                // t2.pause();
                // std::cerr << ' ' << t2.tacSec() << ' ' << (t.tacSec() < t2.tacSec()) << std::endl;

                if(iset.empty())
                    throw(std::exception()); // Should not happen
                int qsize = iset.back().size();
                std::vector<int> qmax(iset.back().begin(),iset.back().end());

                // md.mcqdyn(qmax, qsize, 100000);

                checkGlobalTimeout();
                // std::cout<<"X"<<std::flush;

                if(qsize <= 2) { // Hyperparametre: Taille minimal a laquelle arreter la methode exact
                    MonPrint(nbCliqueFound, " cliques found in ", (chrono.tacSec()), "sec.");
                    return true;
                }
                nbCliqueFound++;

                {
                    //int newI=qmax[0];
                    std::vector<int> clause;

                    for (unsigned int i = 0; i < qsize; i++) {
                        int lit = assumption[qmax[i]];
                        active[qmax[i]] = false;
                        clause.push_back(lit);

                        for(unsigned int x=0; x<size; x++) {
                            conn[qmax[i]][x] = false;
                            conn[x][qmax[i]] = false;
                        }
                    }
                    auto newAssum = processAtMostOne(clause);
                    assert(qsize >= newAssum.size());

                    for(unsigned int j=0; j<newAssum.size() ; j++) {
                        assumption[ qmax[j] ] = newAssum[j];
                        active[ qmax[j] ] = true;

                        std::vector<int> prop;
                        if(solver->propagate({newAssum[j]}, prop)) {
                            for(int lit2: prop) {
                                for(unsigned int k=0; k<size; k++) {
                                    if(active[k]) {
                                        if(assumption[k] == -lit2) {
                                            conn[qmax[j]][k] = true;
                                            conn[k][qmax[j]] = true;
                                        }
                                    }
                                }
                            }
                         } else {
                            assert(solver->solve(std::vector<int>({newAssum[j]})) == false);
                            addClause({-newAssum[j]});
                         }
                    }
                }
            }

            assert(false);
        }

        // Harden soft vars in passed clique to then unrelax them via a new cardinality constraint
        std::vector<int> processAtMostOne(std::vector<int> clause) {
            std::vector<int> newAssum;

            while(clause.size() > 1) {
                checkGlobalTimeout();

                assert([&](){
                    for(unsigned int i=0; i<clause.size(); i++) {
                        for(unsigned int j=i+1; j<clause.size(); j++) {
                            assert(solver->solve(std::vector<int>({clause[i], clause[j]})) == 0 );
                        }
                    }
                    return true;
                }());

                auto saveClause = clause;
                auto w = _poids[ clause[0] ];
                assert(w > 0);

                for(unsigned int i=1; i<clause.size(); i++) {
                    if( w > _poids[ clause[i] ] ) {
                        w = _poids[ clause[i] ];
                    }
                }
                assert(w > 0);

                for(unsigned int i=0; i<clause.size(); ) {
                    assert( _poids[clause[i]] > 0 );
                    assert( _mapWeight2Assum[ _poids[ clause[i] ] ].count( clause[i] ) );
                    _mapWeight2Assum[ _poids[ clause[i] ] ].erase( clause[i] );
                    if( _mapWeight2Assum[ _poids[ clause[i] ] ].size() == 0 ) {
                        _mapWeight2Assum.erase( _poids[ clause[i] ] );
                    }

                    _poids.add( clause[i], -w );

                    assert( _poids[ clause[i] ] >= 0 );
                    if( _poids[ clause[i] ] == 0 ) {
                        relax( clause[i] );
                        clause[i] = clause.back();
                        clause.pop_back();
                    } else {
                        _mapWeight2Assum[ _poids[ clause[i] ] ].insert( clause[i] );
                        i++;
                    }
                }
                MonPrint("AM1: cost = ", cost, " + ", w * (t_weight)(saveClause.size()-1));
                cost += w * (t_weight)(saveClause.size()-1);

                assert(saveClause.size() > 1);
                newAssum.push_back( addClause(saveClause, w) );
                assert(newAssum.back() != 0);
                assert( _poids[ newAssum.back() ] > 0 );
            }

            if( clause.size() ) {
                newAssum.push_back(clause[0]);
            }
            return newAssum;
        }

        void reduceCliqueV2(std::list<int> & clique) {
            if(_mapWeight2Assum.size() > 1) {
                clique.sort([&](int litA, int litB){
                    assert( _poids[ -litA ] > 0 );
                    assert( _poids[ -litB ] > 0 );

                    return _poids[ -litA ] < _poids[ -litB ];
                });
            }
            for(auto posImpliquant = clique.begin() ; posImpliquant != clique.end() ; ++posImpliquant) {
                auto posImpliquant2 = posImpliquant;
                for(++posImpliquant2 ; posImpliquant2 != clique.end() ; ) {
                    checkGlobalTimeout();
                    if(solver->solveLimited(std::vector<int>({-(*posImpliquant), -(*posImpliquant2)}), 10000) != 0) { // solve != UNSAT
                        posImpliquant2 = clique.erase(posImpliquant2);
                    } else {
                        ++posImpliquant2;
                    }
                }
            }
        }

        unsigned int adapt_am1_VeryFastHeuristic() {
            std::vector<int> prop;
            unsigned int nbCliqueFound=0;

            for(int VAR = 1; VAR<_poids.size(); VAR++) {
                if(_poids[VAR] == 0)
                    continue;
                assert(_poids[VAR] != 0);

                int LIT = _poids[VAR]>0?VAR:-VAR;
                prop.clear();

                if(solver->propagate({LIT}, prop)) {
                    if(prop.size() == 0)
                        continue;

                    for(auto litProp: prop) {
                        if(_poids[litProp] < 0) {
                            assert(solver->solve(std::vector<int>({-litProp, LIT})) == false);
                            processAtMostOne( {-litProp, LIT} );
                            nbCliqueFound++;
                            if(_poids[VAR] == 0)
                                break;
                        }
                    }
                } else {
                    nbCliqueFound++;
                    addClause({-LIT});
                }
            }

            return nbCliqueFound;
        }

        unsigned int adapt_am1_FastHeuristicV7() {
            MonPrint("adapt_am1_FastHeuristic : (_weight.size() = ", _poids.size(), " )");

            Chrono chrono;
            std::vector<int> prop;
            unsigned int nbCliqueFound=0;

            for(int VAR = 1; VAR<_poids.size(); VAR++) {
                checkGlobalTimeout();
                if(_poids[VAR] == 0)
                    continue;

                assert(_poids[VAR] != 0);

                int LIT = _poids[VAR]>0?VAR:-VAR;
                prop.clear();
                if(solver->propagate({LIT}, prop)) {
                    if(prop.size() == 0)
                        continue;

                    std::list<int> clique;
                    for(auto litProp: prop) {
                        if(_poids[litProp] < 0) {
                            clique.push_back(litProp);
                            assert(solver->solve(std::vector<int>({-litProp, LIT})) == false);
                        }
                    }

                    if(clique.size() == 0)
                        continue;

                    reduceCliqueV2(clique); // retirer des elements pour que clique soit une clique

                    clique.push_back(-LIT);

                    if(clique.size() >= 2) {
                        nbCliqueFound++;

                        std::vector<int> clause;
                        for(auto lit: clique)
                            clause.push_back(-lit);

                        processAtMostOne(clause);
                    }
                } else {
                    nbCliqueFound++;
                    addClause({-LIT});
                }
            }

            MonPrint(nbCliqueFound, " cliques found in ", chrono);
            return nbCliqueFound;
        }
    ///
    /// End for extractAM
    ///////////////////////


public:
    void addWeight(int lit, long long weight) {
        assert(lit != 0);
        assert(weight != 0 && "Not an error, but it should not happen");
        if(weight < 0) {
            weight = -weight;
            lit = -lit;
        }

        while(abs(lit) >= _poids.size()) {
            newVar();
        }

        if( _poids[lit] == 0 ) {
            _poids.set(lit, weight);
            _mapWeight2Assum[weight].insert(lit);
        } else {
            if( _poids[lit] > 0 ) {
                assert( _mapWeight2Assum[_poids[lit]].count( lit ) );
                _mapWeight2Assum[_poids[lit]].erase( lit );
                if(_mapWeight2Assum[_poids[lit]].size() == 0) {
                    _mapWeight2Assum.erase(_poids[lit]);
                }
                _poids.add(lit, weight);
                assert( _poids[lit] < 0 ? !_mapAssum2Card[abs(lit)].has_value() : true ); // If -lit becomes a soft var, it should not be a cardinality
            } else { // if( _poids[lit] < 0 )
                assert( _mapWeight2Assum[_poids[-lit]].count( -lit ) );
                _mapWeight2Assum[_poids[-lit]].erase( -lit );
                if(_mapWeight2Assum[_poids[-lit]].size() == 0) {
                    _mapWeight2Assum.erase(_poids[-lit]);
                }

                cost += std::min(weight, _poids[-lit]);
                _poids.add(lit, weight);
                assert( _poids[-lit] > 0 ? !_mapAssum2Card[abs(lit)].has_value() : true ); // If lit becomes a soft var, it should not be a cardinality
            }

            if(_poids[lit] != 0) {
                if(_poids[lit] > 0) {
                    _mapWeight2Assum[_poids[lit]].insert( lit );
                } else {
                    _mapWeight2Assum[-_poids[lit]].insert( -lit );
                }
            } else {
                relax(lit);
            }
        }
    }
private:


    // If a soft variable is not soft anymore, we have to check if this variable is a cardinality, in which case, we have to relax the cardinality.
    std::optional<int> relax(int lit) {
        assert(lit != 0);
        std::optional<int> newSoftVar;
        unsigned int var = abs(lit);

        //assert(_mapAssum2Card[var].has_value());
        if(_mapAssum2Card[var].has_value()) { // If there is a cardinality constraint associated to this soft var
            int forCard = _mapAssum2Card[ var ]->card->atMost( _mapAssum2Card[ var ]->atMost + 1 );

            if(forCard != 0) {
                if( _mapAssum2Card[abs(forCard)].has_value() == false ) {
                    _mapAssum2Card[abs(forCard)] = LitCard(_mapAssum2Card[var]->card,  _mapAssum2Card[var]->atMost + 1,  _mapAssum2Card[var]->initialWeight);
                    assert( forCard == _mapAssum2Card[abs(forCard)]->getLit() );
                    _poids.set(forCard, _mapAssum2Card[abs(forCard)]->initialWeight);
                    _mapWeight2Assum[_poids[forCard]].insert( forCard );
                    newSoftVar = forCard;
                }
            }

            if(_poids[lit] == 0) {
                _mapAssum2Card[var].reset();
            }
        }
        return newSoftVar;
    }

    t_weight chooseNextMinWeight(t_weight minWeightToConsider=std::numeric_limits<t_weight>::max()) {
        auto previousWeight = minWeightToConsider;

        if(_mapWeight2Assum.size() <= 1) {
            return 1;
        }

        for(;;) {
            auto it = _mapWeight2Assum.lower_bound(minWeightToConsider);
            if(it == _mapWeight2Assum.begin()) {
                return 1;
            }
            --it;

            if(it == _mapWeight2Assum.end()) {
                assert(!"possible ?");
                return 1;
            }

            if( it->second.size() == 0 ) {
                assert(!"possible ?");
                _mapWeight2Assum.erase(it);
                if(_mapWeight2Assum.size() <= 1) {
                    return 1;
                }
            } else {
               auto it2 =it;
               it2--;
               if(it2 == _mapWeight2Assum.end()) {
                   MonPrint("minWeightToConsider == ", 1);
                   return 1;
               }

               /*
               if(it2->first < it->first * 0.1 ) {   // hyper paramétre
                   MonPrint("minWeightToConsider apres = ", it->first);
                   return it->first;
               }
               */

               if(it2->first < previousWeight * 0.5) {  // hyper paramétre
                   MonPrint("minWeightToConsider = ", it->first);
                   return it->first;
               }

               minWeightToConsider = it->first;
            }
        }

        assert(false);
        return 1;
    }

    ///////////////////////////
    /// Getter
    ///
        unsigned int nInputVars=0;
        public:
        void setNInputVars(unsigned int nInputVars) { // Only used to format the size of the solution
            this->nInputVars=nInputVars;
        }
        unsigned int nVars() const {
            return _poids.size()-1;
            //return solver->nVars();
        }
        unsigned int nClauses() const {
            return solver->nClauses();
        }
        std::vector<bool> getSolution() {
            if(solution.size() >= nInputVars+1) {
                return std::vector<bool>(solution.begin(), solution.begin() + nInputVars + 1);
            }
            std::vector<bool> res = solution;
            res.resize(nInputVars+1);
            return res;
        }
        t_weight getCost() {
            return solutionCost;
        }

        void setSolution(const std::vector<bool>& sol, t_weight solCost) {
            solution = sol;
            solutionCost = solCost;
        }
    //
    /////////////////
};





#endif // EVALMAXSAT_SLK178903R_H

