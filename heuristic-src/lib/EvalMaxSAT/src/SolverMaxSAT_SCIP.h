#ifndef SOLVERMAXSAT_SCIP_H
#define SOLVERMAXSAT_SCIP_H



#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

#include "coutUtil.h"
#include "utile.h"
#include "scip/scip.h"
#include "scip/scipdefplugins.h"



class SolverMaxSAT_SCIP {
    SCIP *scip;
    std::vector<SCIP_VAR *> SCIP_vars;
    int numClause;
    int numVar;
    SCIP_SOL *sol;
    double costMin;

    bool bIsWeighted = false;
public:
    SolverMaxSAT_SCIP() : scip(nullptr), numClause(1), numVar(1), sol(nullptr), costMin(0) {

        SCIP_CALL_ABORT(SCIPcreate(&scip));

        // Activer les plugins SCIP pour les problèmes linéaires en nombre entier
        SCIP_CALL_ABORT(SCIPincludeDefaultPlugins(scip));

        // Créer une nouvelle instance de problème
        SCIP_CALL_ABORT(SCIPcreateProbBasic(scip, "PLNE"));

        SCIP_CALL_ABORT(SCIPsetIntParam(scip, "display/verblevel", 0));

        SCIP_vars.resize(1); // fake var_0
    }

    ~SolverMaxSAT_SCIP() {
        //if(sol != nullptr) {
        //    SCIP_CALL_ABORT(SCIPfreeSol(scip, &sol));
        //}
        for(unsigned int i=1; i<SCIP_vars.size(); i++) {
            SCIP_CALL_ABORT(SCIPreleaseVar(scip, &SCIP_vars[i]));
        }
        SCIP_CALL_ABORT(SCIPfree(&scip));
    }

    unsigned int nInputVars=0;
    void setNInputVars(unsigned int nInputVars) {
        this->nInputVars=nInputVars;
    }
    unsigned int nVars() {
        return SCIP_vars.size()-1;
    }

    void addClause(std::vector<int> clause, double weight) {
        if(sol != nullptr) {
            SCIPfreeTransform(scip);
            sol=nullptr;
        }

        assert(weight > 0);
        auto softLit = newSoftVar(true, weight);
        clause.push_back(-softLit);
        addClause(clause);
    }


    void addAtMost(const std::vector<int> &clause, int k, double weight) { // c1 + c2 + c3 + c4 + c5 <= k
        SCIP_CONS *c = NULL;
        std::string name = "c"+ std::to_string(numClause);
        int rhs=k;
        for(auto id: clause) {
            if(id<0) {
                rhs--;
            }
        }
        SCIP_CALL_ABORT(SCIPcreateConsBasicLinear(scip, &c, name.c_str(), 0, NULL, NULL, -SCIPinfinity(scip), rhs));

        for(auto id: clause) {
            assert(abs(id) < SCIP_vars.size());
            SCIP_CALL_ABORT(SCIPaddCoefLinear(scip, c, SCIP_vars[abs(id)], id>0?1:-1));
        }

        SCIP_vars.resize(SCIP_vars.size()+1);
        std::string nameV = "W" + std::to_string(numVar);
        SCIP_CALL_ABORT(SCIPcreateVarBasic(scip, &(SCIP_vars.back()), nameV.c_str(), 0, clause.size(), weight, SCIP_VARTYPE_INTEGER));
        SCIP_CALL_ABORT(SCIPaddVar(scip, SCIP_vars.back()));
        numVar++;

        SCIP_CALL_ABORT(SCIPaddCoefLinear(scip, c, SCIP_vars.back(), -1));

        SCIP_CALL_ABORT(SCIPaddCons(scip, c));
        SCIP_CALL_ABORT(SCIPreleaseCons(scip, &c));
        numClause++;
    }

    void addClause(const std::vector<int> &clause) {
        if(sol != nullptr) {
            SCIPfreeTransform(scip);
            sol=nullptr;
        }

        SCIP_CONS *c = NULL;
        std::string name = "c"+ std::to_string(numClause);
        int lhs=1;
        for(auto id: clause) {
            if(id<0) {
                lhs--;
            }
        }
        SCIP_CALL_ABORT(SCIPcreateConsBasicLinear(scip, &c, name.c_str(), 0, NULL, NULL, lhs, SCIPinfinity(scip)));

        for(auto id: clause) {
            assert(abs(id) < SCIP_vars.size());
            SCIP_CALL_ABORT(SCIPaddCoefLinear(scip, c, SCIP_vars[abs(id)], id>0?1:-1));
        }

        SCIP_CALL_ABORT(SCIPaddCons(scip, c));
        SCIP_CALL_ABORT(SCIPreleaseCons(scip, &c));
        numClause++;
    }

    int newVar() {
        if(sol != nullptr) {
            SCIPfreeTransform(scip);
            sol=nullptr;
        }

        SCIP_vars.resize(SCIP_vars.size()+1);

        std::string name = "x" + std::to_string(numVar);

        SCIP_CALL_ABORT(SCIPcreateVarBasic(scip, &(SCIP_vars.back()), name.c_str(), 0, 1, 0, SCIP_VARTYPE_BINARY));
        SCIP_CALL_ABORT(SCIPaddVar(scip, SCIP_vars.back()));
        numVar++;
        return SCIP_vars.size()-1;
    }
    bool isWeighted() {
        return bIsWeighted;
    }
    int newSoftVar(bool value, double w) {
        if(w != 1) {
            bIsWeighted = true;
        }
        if(sol != nullptr) {
            SCIPfreeTransform(scip);
            sol=nullptr;
        }

        if(!value) {
            w = -w;
        }
        if(w > 0) {
            costMin += -w;
        }
        assert(w != 0);
        SCIP_vars.resize(SCIP_vars.size()+1);

        std::string name = "x" + std::to_string(numVar);

        SCIP_CALL_ABORT(SCIPcreateVarBasic(scip, &(SCIP_vars.back()), name.c_str(), 0, 1, -w, SCIP_VARTYPE_BINARY));
        SCIP_CALL_ABORT(SCIPaddVar(scip, SCIP_vars.back()));
        numVar++;

        return SCIP_vars.size()-1;
    }

    bool solve(t_weight UB = std::numeric_limits<t_weight>::max()) { // TODO: ajouter UB dans SCIP


        if(UB != std::numeric_limits<t_weight>::max()) {
            SCIPupdateCutoffbound(scip, nextafter((double)UB, DBL_MAX) );
        }

        // Résoudre le problème
        SCIP_CALL_ABORT(SCIPsolve(scip));

        if (SCIP_STATUS_OPTIMAL == SCIPgetStatus(scip)) {
            sol = SCIPgetBestSol(scip);
            return true;
        } else {
            return false;
        }
    }


    int solveLimited(double timesSec, t_weight UB = std::numeric_limits<t_weight>::max()) {

        // TODO: Use UB to help SCIP ?

        // Timeout 1min
        SCIP_CALL_ABORT(SCIPsetRealParam(scip, "limits/time", timesSec));

        // Résoudre le problème
        SCIP_CALL_ABORT(SCIPsolve(scip));

        if (SCIP_STATUS_OPTIMAL == SCIPgetStatus(scip)) {
            MaLib::MonPrint("SCIP: OPTIMAL SOLUTION FOUND");
            sol = SCIPgetBestSol(scip);
            return 1;
        } if (SCIP_STATUS_INFEASIBLE == SCIPgetStatus(scip)) {
            MaLib::MonPrint("SCIP: NOT SOLUTION FOUND");
            return 0;
        } else {
            MaLib::MonPrint("SCIP: NOT OPTIMAL SOLUTION FOUND");
            sol = SCIPgetBestSol(scip);
            MaLib::MonPrint("SCIP: resultat non optimal : ", getCost());
            if(getCost() < 0) {
                MaLib::MonPrint("SCIP: solution non admissible.");
                return 0;
            }
            return -1;
        }
    }

    int getCost() {
        return (SCIPgetSolOrigObj(scip, sol)+0.5) - costMin;
    }

    bool getValue(int id) {
        return SCIPgetSolVal(scip, sol, SCIP_vars[id]) > 0.5;
    }
};



#endif // SOLVERMAXSAT_SCIP_H
