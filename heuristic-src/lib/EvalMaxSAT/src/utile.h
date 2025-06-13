#ifndef EVALMAXSAT_UTILE_H__
#define EVALMAXSAT_UTILE_H__

#include <vector>
#include <cmath>
#include <cassert>
#include <tuple>
#include <zlib.h>

#include "ParseUtils.h"

#include "SBVA.h"






std::string savePourTest_file;

typedef unsigned long long int t_weight;

class WeightVector {
    std::vector<long long int> _weight;
public:
    long long int operator[](int lit) const {
        assert(abs(lit) < _weight.size());
        return (_weight[abs(lit)] ^ (lit >> 31)) - (lit >> 31);
    }

    unsigned int size() const {
        return _weight.size();
    }

    void resize(unsigned int newSize) {
        _weight.resize(newSize);
    }

    void push_back(long long int v) {
        _weight.push_back(v);
    }

    void set(int lit, long long int weight) {
        assert(abs(lit) < _weight.size());
        if(lit < 0 ) {
            _weight[-lit] = -weight;
        } else {
            _weight[lit] = weight;
        }
    }

    void add(int lit, long long int weight) {
        assert(abs(lit) < _weight.size());
        if(lit < 0 ) {
            _weight[-lit] += -weight;
        } else {
            _weight[lit] += weight;
        }
    }
};

template<class T>
class doublevector {

    std::vector<T> posIndexVector;
    std::vector<T> negIndexVector;

public:

    void push_back(const T& v) {
        posIndexVector.push_back(v);
    }

    T& back() {
        if(posIndexVector.size()) {
            return posIndexVector.back();
        } else {
            return negIndexVector[0];
        }
    }

    void pop_back() {
        if(posIndexVector.size()) {
            posIndexVector.pop_back();
        } else {
            negIndexVector.erase(negIndexVector.begin());
        }
    }

    void add(int index, const T& val) {
        if(index >= 0) {
            if(index >= posIndexVector.size()) {
                posIndexVector.resize(index+1);
            }
            posIndexVector[index] = val;
        } else {
            if(-index >= negIndexVector.size()) {
                negIndexVector.resize((-index)+1);
            }
            negIndexVector[-index] = val;
        }
    }

    T& operator [](int index) {
        if(index >= 0) {
            assert( index < posIndexVector.size() );
            return posIndexVector[index];
        } else {
            assert( -index < negIndexVector.size() );
            return negIndexVector[-index];
        }
        assert(false);
    }

    T& get(int index) {
        if(index >= 0) {
            if(index >= posIndexVector.size()) {
                posIndexVector.resize(index+1);
            }
            return posIndexVector[index];
        } else {
            if(-index >= negIndexVector.size()) {
                negIndexVector.resize((-index)+1);
            }
            return negIndexVector[-index];
        }
    }


};


/// POUR DEBUG ////
template<class B>
static void readClause(B& in, std::vector<int>& lits) {
    int parsed_lit;
    lits.clear();
    for (;;){
        parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        lits.push_back( parsed_lit );
    }
}

/// POUR DEBUG ////
t_weight calculateCost(const std::string & file, std::vector<bool> &result) {
    t_weight cost = 0;
    auto in_ = gzopen(file.c_str(), "rb");
    t_weight weightForHardClause = -1;

    StreamBuffer in(in_);

    std::vector<int> lits;
    for(;;) {
        skipWhitespace(in);

        if(*in == EOF)
            break;

        else if(*in == 'c') {
            skipLine(in);
        } else if(*in == 'p') { // Old format
          ++in;
          if(*in != ' ') {
              std::cerr << "c PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
              return false;
          }
          skipWhitespace(in);

          if(eagerMatch(in, "wcnf")) {
              parseInt(in); // # Var
              parseInt(in); // # Clauses
              weightForHardClause = parseWeight(in);
          } else {
              std::cerr << "c PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
              return false;
          }
      }
        else {
            t_weight weight = parseWeight(in);
            readClause(in, lits);
            if(weight >= weightForHardClause) {
                bool sat=false;
                for(auto l: lits) {
                    if(abs(l) >= result.size()) {
                        result.resize(abs(l)+1, 0);
                        assert(!"Error size result");
                    }
                    if ( (l>0) == (result[abs(l)]) ) {
                        sat = true;
                        break;
                    }
                }
                if(!sat) {
                    //std::cerr << "Error : solution no SAT !" << std::endl;
                    return -1;
                }
            } else {
                bool sat=false;
                for(auto l: lits) {
                    if(abs(l) >= result.size()) {
                        result.resize(abs(l)+1, 0);
                        assert(!"Error size result");
                    }

                    if ( (l>0) == (result[abs(l)]) ) {
                        sat = true;
                        break;
                    }
                }
                if(!sat) {
                    cost += weight;
                }
            }
        }
    }

    gzclose(in_);
    return cost;
}


template<class MAXSAT_SOLVER>
std::vector<int> readClause(StreamBuffer &in, MAXSAT_SOLVER* solveur) {
    std::vector<int> clause;

    for (;;) {
        int lit = parseInt(in);

        if (lit == 0)
            break;
        clause.push_back(lit);
        while( abs(lit) > solveur->nVars()) {
            solveur->newVar();
        }
    }

    return clause;
}

template<class MAXSAT_SOLVER>
bool parse(const std::string& filePath, MAXSAT_SOLVER* solveur) {
    savePourTest_file = filePath;
    auto gz = gzopen( filePath.c_str(), "rb");

    StreamBuffer in(gz);
    t_weight weightForHardClause = -1;

    if(*in == EOF) {
        return false;
    }

    std::vector < std::tuple < std::vector<int>, t_weight> > softClauses;

    for(;;) {
        skipWhitespace(in);

        if(*in == EOF) {
            break;
        }

        if(*in == 'c') {
            skipLine(in);
        } else if(*in == 'p') { // Old format
            ++in;
            if(*in != ' ') {
                std::cerr << "c PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
                return false;
            }
            skipWhitespace(in);

            if(eagerMatch(in, "wcnf")) {
                parseInt(in); // # Var
                parseInt(in); // # Clauses
                weightForHardClause = parseWeight(in);
            } else {
                std::cerr << "c PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
                return false;
            }
        } else {
            t_weight weight = parseWeight(in);
            std::vector<int> clause = readClause(in, solveur);

            if(weight >= weightForHardClause) {
                solveur->addClause(clause);
            } else {
                // If it is a soft clause, we have to save it to add it once we are sure we know the total number of variables.
                softClauses.push_back({clause, weight});
            }
        }
    }

    solveur->setNInputVars(solveur->nVars());
    for(auto & [clause, weight]: softClauses) {
        solveur->addClause(clause, weight);
    }

    gzclose(gz);
    return true;
 }


std::vector<int> readClause(StreamBuffer &in) {
    std::vector<int> clause;

    for (;;) {
        int lit = parseInt(in);
        if (lit == 0)
            break;
        clause.push_back(lit);
    }

    return clause;
}




template<class MAXSAT_SOLVER>
bool parse_and_simplify(const std::string& filePath, MAXSAT_SOLVER* solveur, unsigned int timeout_sec=-1, double ratio_newvar_added=0.3, int minimal_deletion=1) {
    savePourTest_file = filePath;

    std::vector < std::tuple < std::vector<int>, t_weight> > softClauses;
    std::vector< std::vector<int> > hardClauses;

    std::unique_ptr<VirtualCache> cache_ptr;
    unsigned int nbIncluded=0;
    bool addToSubset = true;

    {
        Formula F;
        auto gz = gzopen( filePath.c_str(), "rb");

        StreamBuffer in(gz);
        t_weight weightForHardClause = -1;

        if(*in == EOF) {
            return false;
        }

        unsigned int poidFormule = 0;
        for(;;) {
            skipWhitespace(in);

            if(*in == EOF) {
                break;
            }

            if(*in == 'c') {
                skipLine(in);
            } else if(*in == 'p') { // Old format
                ++in;
                if(*in != ' ') {
                    std::cerr << "c PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
                    return false;
                }
                skipWhitespace(in);

                if(eagerMatch(in, "wcnf")) {
                    parseInt(in); // # Var
                    parseInt(in); // # Clauses
                    weightForHardClause = parseWeight(in);
                } else {
                    std::cerr << "c PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
                    return false;
                }
            } else {
                t_weight weight = parseWeight(in);
                auto clause = readClause(in, solveur);
                sort(clause.begin(), clause.end());

                if(weight >= weightForHardClause) {
                    hardClauses.push_back(clause);
                } else {
                    softClauses.push_back({clause, weight});
                }
            }
        }
        gzclose(gz);

        auto timeout = MaLib::TimeOut(timeout_sec);

        std::cout << "c hardClauses.size() = " << hardClauses.size() << std::endl;

        if(hardClauses.size() < 100000) {
            cache_ptr = std::make_unique<subset10>();
            auto compare = [](const std::vector<int>& a, const std::vector<int>& b) {
                return a.size() < b.size();
            };
            std::sort(hardClauses.begin(), hardClauses.end(), compare);
        } else if(hardClauses.size() < 100000000) {
            cache_ptr = std::make_unique<ClauseCache>();
        } else {
            cache_ptr = std::make_unique<noCache>();
        }

                                    {
                                        F.clauses.reserve(hardClauses.size());
                                        MaLib::Chrono C_inclusion;

                                        for(unsigned int i=0; i<hardClauses.size(); i++) {
                                            auto &clause = hardClauses[i];
                                            if(cache_ptr->contains(clause) == false) {
                                                F.clauses.push_back(clause);
                                                for (auto lit : clause) {
                                                    if(F.lit_to_clauses.size() <= lit_index(lit)) {
                                                        F.lit_to_clauses.resize(lit_index(lit)+1);
                                                    }
                                                    F.lit_to_clauses[lit_index(lit)].push_back( F.clauses.size()-1 );
                                                }
                                                cache_ptr->add(clause);
                                            } else {
                                                nbIncluded++;
                                            }
                                            if(i%10000 == 0) {
                                                if(timeout()) {
                                                    cache_ptr = std::make_unique<noCache>();
                                                }
                                            }
                                        }
                                    }


        solveur->setNInputVars(solveur->nVars());

        F.num_vars = solveur->nVars();
        F.num_clauses = F.clauses.size();




        assert(F.lit_to_clauses.size() <= solveur->nVars() * 2);
        if(F.lit_to_clauses.size() < solveur->nVars() * 2) {
            F.lit_to_clauses.resize(solveur->nVars() * 2);
        }
        //F.lit_count_adjust = new vector<int>(solveur->nVars() * 2);
        F.lit_count_adjust = vector<int>(solveur->nVars() * 2);
        F.adjacency_matrix_width = solveur->nVars() * 4;
        F.adjacency_matrix.resize(solveur->nVars());
        int taille = 0;
        for (int i=1; i<=solveur->nVars(); i++) {
            if(i%1000 == 0) {
                //std::cout << "taille = " << taille << std::endl;
                if(timeout()) {
                    break;
                }
                if(taille > 1000000000) {
                    break;
                }
            }
            taille += F.update_adjacency_matrix(i);
        }
        //std::cout << "taille = " << taille << std::endl; 


        //std::cout << "c run simplify" << std::endl;
        if( (!timeout()) && (taille <= 1000000000) ) {
            F.run(Tiebreak::ThreeHop, timeout, ratio_newvar_added, minimal_deletion);
        } else {
            std::cout << "c timeout ou formule trop grosse²" << std::endl;
        }

        std::cout << "c adj_deleted = " << F.adj_deleted << std::endl;

        for(int i=0; i<F.clauses.size(); i++) {
            if (F.clauses[i].deleted) {
                continue;
            }
            for(auto lit: F.clauses[i].lits) {
                while( abs(lit) > solveur->nVars()) {
                    solveur->newVar();
                }
            }
            solveur->addClause(F.clauses[i].lits);
        }
    }

    for(auto & [clause, weight]: softClauses) {
        if(cache_ptr->contains(clause) == false) {
            solveur->addClause(clause, weight);
        } else {
            MonPrint("c soft clause covered by hard clause");
            nbIncluded++;
        }
    }

    std::cout << "c nbIncluded = " << nbIncluded << std::endl;

    return true;
 }




#endif
