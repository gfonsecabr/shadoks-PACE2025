// From https://github.com/hgarrereyn/SBVA

#ifndef EVALMAXSAT_SBVA_H__
#define EVALMAXSAT_SBVA_H__


#include <cstdlib>
#include <cstring>
#include <iostream>
#include <queue>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <set>

#include <getopt.h>
#include <stdio.h>

#include <Eigen/SparseCore>

#include "coutUtil.h"

using namespace std;
using namespace MaLib;


class VirtualCache {
public:
    virtual void add(const std::vector<int> & set) = 0;
    virtual bool contains(const std::vector<int> &query) = 0;
};

class noCache : public VirtualCache {
public:
    void add(const std::vector<int> & set) { }
    bool contains(const std::vector<int> &query) { return false; }
};

class subset10 : public VirtualCache {
    struct Node {
        bool used=false;
        std::unordered_map<unsigned int, Node*> children;

        //friend Node* subset10::newNode();

        //private:
        Node() { }
    };
    std::vector<Node*> _allNodes;

    Node* newNode() {
        _allNodes.push_back( new Node() );
        return _allNodes.back();
    }

    std::unordered_map<int, int> _val2index;
    int maxValX = -1;
    std::vector<unsigned int> convert2index(const std::vector<int> &set) {
        std::vector<unsigned int> result;
        for(auto &e: set) {
            if(_val2index.count(e) == 0 ) {
                _val2index[e] = ++maxValX;
            }
            result.push_back(_val2index[e]);
        }
        return result;
    }

    Node* root;

public:

    subset10() {
        root = newNode();

    }

    ~subset10() {
        for(auto ptr: _allNodes) {
            delete ptr;
        }
    }


    void add(const std::vector<int> & set) {
        std::vector<unsigned int> new_set = convert2index(set);
        std::sort(new_set.begin(), new_set.end());
        auto last = std::unique(new_set.begin(), new_set.end());
        new_set.erase(last, new_set.end());

        Node* node = root;
        for(unsigned int i=0; i<new_set.size(); i++) {
            if(node->children.count(new_set[i]) == 0) {
                node->children[new_set[i]] = newNode();
            }
            node = node->children[new_set[i]];
        }
        //node->values.push_back(value);
        node->used = true;
    }


    bool contains(const std::vector<int> &query) {
        std::vector<unsigned int> set_query = convert2index(query);
        std::sort(set_query.begin(), set_query.end());
        auto last = std::unique(set_query.begin(), set_query.end());
        set_query.erase(last, set_query.end());

        //std::unordered_map<unsigned int, std::vector<Node*>> _dynTable2;
        std::vector<std::vector<Node*>> _dynTable2;
        _dynTable2.resize(maxValX+1);


        //for(auto &e: set_query) {
        for(unsigned int i=0; i<set_query.size(); i++) {
            auto &e = set_query[i];

            //if(_dynTable2.count(e) > 0) {
                for(auto ptrNode: _dynTable2[e]) {
                    if(ptrNode->used) {
                        return true;
                    }

                    if( set_query.size()-i < ptrNode->children.size()) {
                        for(unsigned int j=i+1; j<set_query.size(); j++) {
                            auto search = ptrNode->children.find(set_query[j]);
                            if(search != ptrNode->children.end()) {
                                _dynTable2[ set_query[j] ].push_back(search->second);
                            }
                        }
                    } else {
        
                        for(auto & [key, ptrNode2]: ptrNode->children) {
                            assert(key > e);
                            _dynTable2[key].push_back(ptrNode2);
                        }
                    }
                }
            //}

            if(root->children.count(e) > 0) {
                if(root->children[e]->used) {
                    return true;
                }

                if( set_query.size()-i < root->children[e]->children.size()) {
                    for(unsigned int j=i+1; j<set_query.size(); j++) {
                        auto search = root->children[e]->children.find(set_query[j]);
                        if(search != root->children[e]->children.end()) {
                            _dynTable2[ set_query[j] ].push_back(search->second);
                        }
                    }
                } else {
                    for(auto & [key, ptrNode]: root->children[e]->children) {
                        assert(key > e);
                        _dynTable2[key].push_back(ptrNode);
                    }
                }

            }
        }

        return false;
    }
};


struct Clause {
    bool deleted;
    vector<int> lits;
    mutable uint32_t hash = 0;

    Clause() : deleted(false) {
    }

    Clause(const vector<int> &lits) : deleted(false), lits(lits) {
    }

    void print() {
        if (deleted) {
            printf("DELETED: ");
        }
        for (int i = 0; i < lits.size(); i++) {
            printf("%d ", lits[i]);
        }
        printf("\n");
    }


    uint32_t hash_val() const {
        if (hash == 0) {
            hash = murmur3_vec((uint32_t *) lits.data(), lits.size(), 0);
        }
        return hash;
    }

    bool operator==(const Clause &other) const {
        if (lits.size() != other.lits.size()) {
            return false;
        }
        for (int i = 0; i < lits.size(); i++) {
            if (lits[i] != other.lits[i]) {
                return false;
            }
        }
        return true;
    }

private:
    // Derived from: https://en.wikipedia.org/wiki/MurmurHash
    inline uint32_t murmur_32_scramble(uint32_t k) const {
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        return k;
    }

    // Derived from: https://en.wikipedia.org/wiki/MurmurHash
    uint32_t murmur3_vec(uint32_t *data, uint32_t size, uint32_t seed) const {
        uint32_t h = seed;
        for (uint32_t i = 0; i < size; i++) {
            h ^= murmur_32_scramble(data[i]);
            h = (h << 13) | (h >> 19);
            h = h * 5 + 0xe6546b64;
        }
        h ^= size;
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;
        return h;
    }
};

struct ClauseHash {
    size_t operator()(const Clause &c) const {
        return c.hash_val();
    }
};

struct ClauseCache : public VirtualCache {
    unordered_set<Clause, ClauseHash> clauses;

    ClauseCache() {}

    void add(const std::vector<int> &lits) {
        clauses.insert(Clause(lits));
    }

    bool contains(const std::vector<int> &lits) {
        Clause c(lits);
        return clauses.find(c) != clauses.end();
    }
};

enum Tiebreak {
    ThreeHop, // default
    None, // use sorted order (should be equivalent to original BVA)
};

inline uint32_t lit_index(int32_t lit) {
    return (lit > 0 ? lit * 2 - 2 : -lit * 2 - 1);
}

inline uint32_t sparsevec_lit_idx(int32_t lit) {
    return (lit > 0 ? lit - 1: -lit - 1);
}

inline uint32_t sparcevec_lit_for_idx(int32_t lit) {
    return (lit + 1);
}

inline int reduction(int lits, int clauses) {
    return (lits * clauses) - (lits + clauses);
}

class Formula {
public:
/*
    static Formula *parse(FILE *fin) {
        Formula *f = new Formula();
        f->read_cnf(fin);
        return f;
    }
*/
    Formula() {
        found_header = false;
        adj_deleted = 0;
    }
/*
    void read_cnf(FILE *fin) {
        char *line = NULL;
        size_t len = 0;

        ClauseCache cache;

        int curr_clause = 0;

        while (getline(&line, &len, fin) != -1) {
            if (len == 0) {
                continue;
            }
            if (line[0] == 'c') {
                continue;
            } else if (line[0] == 'p') {
                sscanf(line, "p cnf %d %d", &num_vars, &num_clauses);
                clauses = new vector<Clause>(num_clauses);
                clauses->reserve(num_clauses * 10);
                lit_to_clauses = new vector< vector<int> >(num_vars * 2);
                lit_count_adjust = new vector<int>(num_vars * 2);
                adjacency_matrix_width = num_vars * 4;
                adjacency_matrix.resize(num_vars);
                found_header = true;
            } else {
                if (!found_header) {
                    fprintf(stderr, "Error: CNF file does not have a header\n");
                    exit(1);
                }

                if (curr_clause >= num_clauses && line[0] == 0) {
                    fprintf(stderr, "Error: CNF file has more clauses than specified in header\n");
                    exit(1);
                }

                int lit = 0;
                char *curr = line;
                while (sscanf(curr, "%d", &lit) > 0) {
                    if (lit == 0) {
                        break;
                    }
                    if (abs(lit) > num_vars) {
                        fprintf(stderr, "Error: CNF file has a variable that is greater than the number of variables specified in the header\n");
                        exit(1);
                    }
                    clauses->operator[](curr_clause).lits.push_back(lit);
                    curr = strchr(curr, ' ');
                    curr++;
                }

                sort(clauses->operator[](curr_clause).lits.begin(), clauses->operator[](curr_clause).lits.end());

                auto *cls = &clauses->operator[](curr_clause);
                if (cache.contains(cls)) {
                    cls->deleted = true;
                    adj_deleted++;
                } else {
                    cache.add(cls);
                    for (auto lit : clauses->operator[](curr_clause).lits) {
                        lit_to_clauses->operator[](lit_index(lit)).push_back(curr_clause);
                    }
                }

                curr_clause++;
            }
        }

        for (int i=1; i<=num_vars; i++) {
            update_adjacency_matrix(i);
        }
    }
    */

    int update_adjacency_matrix(int lit) {
        int abslit = std::abs(lit);
        if (adjacency_matrix[sparsevec_lit_idx(abslit)].nonZeros() > 0) {
            return 0;
        }
        Eigen::SparseVector<int> vec(adjacency_matrix_width);

        for (int cid : lit_to_clauses[lit_index(abslit)]) {
            Clause *cls = &(clauses[cid]);
            if (cls->deleted) continue;
            for (int v : cls->lits) {
                vec.coeffRef(sparsevec_lit_idx(v)) += 1;
            }
        }

        for (int cid : lit_to_clauses[lit_index(-abslit)]) {
            Clause *cls = &(clauses[cid]);
            if (cls->deleted) continue;
            for (int v : cls->lits) {
                vec.coeffRef(sparsevec_lit_idx(v)) += 1;
            }
        }

        adjacency_matrix[sparsevec_lit_idx(abslit)] = vec;

        return vec.nonZeros();
    }

    int tiebreaking_heuristic(int lit1, int lit2) {
        if (tmp_heuristic_cache_full.find(sparsevec_lit_idx(lit2)) != tmp_heuristic_cache_full.end()) {
            return tmp_heuristic_cache_full[sparsevec_lit_idx(lit2)];
        }
        int abs1 = std::abs(lit1);
        int abs2 = std::abs(lit2);
        update_adjacency_matrix(lit1);
        update_adjacency_matrix(lit2);

        Eigen::SparseVector<int> *vec1 = &adjacency_matrix[sparsevec_lit_idx(abs1)];
        Eigen::SparseVector<int> *vec2 = &adjacency_matrix[sparsevec_lit_idx(abs2)];

        int total_count = 0;
        for (int *varPtr = vec2->innerIndexPtr(); varPtr < vec2->innerIndexPtr() + vec2->nonZeros(); varPtr++) {
            int var = sparcevec_lit_for_idx(*varPtr);
            int count = vec2->coeffRef(sparsevec_lit_idx(var));
            update_adjacency_matrix(var);
            Eigen::SparseVector<int> *vec3 = &adjacency_matrix[sparsevec_lit_idx(var)];
            total_count += count * vec3->dot(*vec1);
        }
        tmp_heuristic_cache_full[sparsevec_lit_idx(lit2)] = total_count;
        return total_count;
    }

/*
    void to_cnf(FILE *fout) {
        fprintf(fout, "p cnf %d %d\n", num_vars, num_clauses - adj_deleted);
        for (int i = 0; i < num_clauses; i++) {
            if (clauses->operator[](i).deleted) {
                continue;
            }
            for (int j = 0; j < clauses->operator[](i).lits.size(); j++) {
                fprintf(fout, "%d ", clauses->operator[](i).lits[j]);
            }
            fprintf(fout, "0\n");
        }
    }
*/

    int least_frequent_not(Clause *clause, int var) {
        int lmin = 0;
        int lmin_count = 0;
        for (auto lit : clause->lits) {
            if (lit == var) {
                continue;
            }
            int count = lit_to_clauses[lit_index(lit)].size() + lit_count_adjust[lit_index(lit)];
            if (lmin == 0 || count < lmin_count) {
                lmin = lit;
                lmin_count = count;
            }
        }
        return lmin;
    }

    int real_lit_count(int lit) {
        return lit_to_clauses[lit_index(lit)].size() + lit_count_adjust[lit_index(lit)];
    }

    // Performs partial clause difference between clause and other, storing the result in diff.
    // Only the first max_diff literals are stored in diff.
    // Requires that clause and other are sorted.
    void clause_sub(Clause *clause, Clause *other, vector<int> &diff, int max_diff) {
        diff.resize(0);
        int idx_a = 0;
        int idx_b = 0;

        while (idx_a < clause->lits.size() && idx_b < other->lits.size() && diff.size() <= max_diff) {
            if (clause->lits[idx_a] == other->lits[idx_b]) {
                idx_a++;
                idx_b++;
            } else if (clause->lits[idx_a] < other->lits[idx_b]) {
                diff.push_back(clause->lits[idx_a]);
                idx_a++;
            } else {
                idx_b++;
            }
        }

        while (idx_a < clause->lits.size() && diff.size() <= max_diff) {
            diff.push_back(clause->lits[idx_a]);
            idx_a++;
        }
    }

    void run(Tiebreak tiebreak_mode, MaLib::TimeOut& timeout, double ratio_newvar_added=0, int minimal_deletion=1) {
        unsigned int max_replacements = num_vars * ratio_newvar_added;

        struct pair_op {
            bool operator()(const pair<int, int> &a, const pair<int, int> &b) {
                return a.first < b.first;
            }
        };

        // The priority queue keeps track of all the literals to evaluate for replacements.
        // Each entry is the pair (num_clauses, lit)
        priority_queue<pair<int,int>, vector< pair<int,int> >, pair_op> pq;

        // Add all of the variables from the original formula to the priority queue.
        for (int i = 1; i <= num_vars; i++) {
            pq.push(make_pair(real_lit_count(i), i));
            pq.push(make_pair(real_lit_count(-i), -i));
        }

        vector<int> matched_lits;
        vector<int> matched_clauses;
        vector<int> matched_clauses_swap;
        vector<int> matched_clauses_id;
        vector<int> matched_clauses_id_swap;
        matched_lits.reserve(10000);
        matched_clauses.reserve(10000);
        matched_clauses_swap.reserve(10000);
        matched_clauses_id.reserve(10000);
        matched_clauses_id_swap.reserve(10000);

        // Track the index of the matched clauses from every literal that is added to matched_lits.
        vector< tuple<int, int> > clauses_to_remove;
        clauses_to_remove.reserve(10000);

        // Used for computing clause differences
        vector<int> diff;
        diff.reserve(10000);

        // Keep track of the matrix of swaps that we can perform.
        // Each entry is of the form (literal, <clause index>, <index in matched_clauses>)
        //
        // For example, given the formula:
        // (A v E)  (A v F)  (A v G)  (A v H)
        // (B v E)  (B v F)  (B v G)  (B v H)
        // (C v E)  (C v F)           (C v H)
        // (D v E)  (D v F)
        // 
        // We would start with the following matrix:
        // matched_entries:     (A, (A v E), 0)  (A, (A v F), 1)  (A, (A v G), 2)  (A, (A v H), 3)
        // matched_clauses_id:  0  1  2  3
        // matched_clauses:     (A v E)  (A v F)  (A v G)  (A v H)
        // 
        // Then, when we add B to matched_lits, we would get:
        // matched_entries:     (A, (A v E), 0)  (A, (A v F), 1)  (A, (A v G), 2)  (A, (A v H), 3)
        //                      (B, (B v E), 0)  (B, (B v F), 1)  (B, (B v G), 2)  (B, (B v H), 3)
        // matched_clauses_id:  0  1  2  3
        // matched_clauses:     (A v E)  (A v F)  (A v G)  (A v H)
        //
        // Then, when we add C to matched_lits, we would get:
        // matched_entries:     (A, (A v E), 0)  (A, (A v F), 1)  (A, (A v G), 2)  (A, (A v H), 3)
        //                      (B, (B v E), 0)  (B, (B v F), 1)  (B, (B v G), 2)  (B, (B v H), 3)
        //                      (C, (C v E), 0)  (C, (C v F), 1)                   (C, (C v H), 3)
        // matched_clauses_id:  0  1  3
        // matched_clauses:     (A v E)  (A v F)  (A v H)
        //
        // Adding D to matched_lits would not result in a reduction so we stop here.
        // 
        // The matched_clauses_id is then used as a filter to find the clauses to remove:
        // 
        // to_remove:   (A v E)  (A v F)  (A v H)
        //              (B v E)  (B v F)  (B v H)
        //              (C v E)  (C v F)  (C v H)
        //
        vector< tuple<int, int, int> > matched_entries;
        matched_entries.reserve(10000);

        // Keep a list of the literals that are matched so we can sort and count later.
        vector<int> matched_entries_lits;
        matched_entries_lits.reserve(10000);

        // Used for priority queue updates.
        unordered_set<int> lits_to_update;
        lits_to_update.reserve(10000);

        /*
        if (generate_proof) {
            proof = new vector<ProofClause>();
        }
        */

        // Track number of replacements (new auxiliary variables).
        int num_replacements = 0;


        while (pq.size() > 0) {
            if(timeout()) {
                MonPrint("SBVA: Timeout");
                return;
            }
            if(max_replacements != 0 && num_replacements >= max_replacements) {
                MonPrint("Hit replacement limit (", max_replacements, ")");
                return;
            }
            /*
            // check timeout
            if (end_time != 0) {
                time_t curr = time(0);
                if (curr >= end_time) {
                    MonPrint("SBVA: Timeout");
                    return;
                }
            }
            */

            // check replacement limit
            /*
            if (max_replacements != 0 && num_replacements == max_replacements) {
                MonPrint("Hit replacement limit (", max_replacements, ")");
                return;
            }
            */
            
            matched_lits.resize(0);
            matched_clauses.resize(0);
            matched_clauses_id.resize(0);
            clauses_to_remove.resize(0);
            tmp_heuristic_cache_full.clear();

            // Get the next literal to evaluate.
            pair<int, int> p = pq.top();
            pq.pop();

            int var = p.second;
            int num_matched = p.first;

            if (num_matched == 0 || num_matched != real_lit_count(var)) {
                continue;
            }

            MonPrint("Trying ", var, " (", num_matched, ")");

            // Mlit := { l }
            matched_lits.push_back(var);
            
            // Mcls := F[l]
            for (int i = 0; i < lit_to_clauses[lit_index(var)].size(); i++) {
                int clause_idx = lit_to_clauses[lit_index(var)][i];
                if (!clauses[(clause_idx)].deleted) {
                    matched_clauses.push_back(clause_idx);
                    matched_clauses_id.push_back(i);
                    clauses_to_remove.push_back(make_tuple(clause_idx, i));
                }
            }

            while (1) {
                // P = {}
                matched_entries.resize(0);
                matched_entries_lits.resize(0);

                /*
                if (enable_trace) {
                    cout << "Iteration, Mlit: ";
                    for (int i = 0; i < matched_lits->size(); i++) {
                        cout << matched_lits->operator[](i) << " ";
                    }
                    cout << endl;
                }
                */

                // foreach C in Mcls
                for (int i = 0; i < matched_clauses.size(); i++) {
                    int clause_idx = matched_clauses[i];
                    int clause_id = matched_clauses_id[i];
                    auto *clause = &(clauses[clause_idx]);

                    /*
                    if (enable_trace) {
                        cout << "  Clause " << clause_idx << " (" << clause_id << "): ";
                        clause->print();
                    }
                    */

                    // let lmin in (C \ {l}) be least occuring in F
                    int lmin = least_frequent_not(clause, var);
                    if (lmin == 0) {
                        continue;
                    }

                    // foreach D in F[lmin]
                    for (auto other_idx : lit_to_clauses[lit_index(lmin)]) {
                        auto *other = &(clauses[other_idx]);
                        if (other->deleted) {
                            continue;
                        }

                        if (clause->lits.size() != other->lits.size()) {
                            continue;
                        }

                        // diff := C \ D (limited to 2)
                        clause_sub(clause, other, diff, 2);
                        
                        // if diff = {l} then
                        if (diff.size() == 1 && diff[0] == var) {
                            // diff := D \ C (limited to 2)
                            clause_sub(other, clause, diff, 2);

                            // if diff = {lmin} then
                            auto lit = diff[0];

                            // TODO: potential performance improvement
                            bool found = false;
                            for (auto l : matched_lits) {
                                if (l == lit) {
                                    found = true;
                                    break;
                                }
                            }

                            // if lit not in Mlit then
                            if (!found) {
                                // Add to clause match matrix.
                                matched_entries.push_back(make_tuple(lit, other_idx, i));
                                matched_entries_lits.push_back(lit);
                            }
                        }
                    }
                }

                // lmax := most frequent literal in P

                sort(matched_entries_lits.begin(), matched_entries_lits.end());

                int lmax = 0;
                int lmax_count = 0;

                std::vector<int> ties;
                ties.reserve(16);
                for (int i = 0; i < matched_entries_lits.size();) {
                    int lit = matched_entries_lits[i];
                    int count = 0;

                    while (i < matched_entries_lits.size() && matched_entries_lits[i] == lit) {
                        count++;
                        i++;
                    }

                    //MonPrint("MBVA: lit: ", lit, " count: ", count);

                    if (count > lmax_count) {
                        lmax = lit;
                        lmax_count = count;
                        ties.clear();
                        ties.push_back(lit);
                    } else if (count == lmax_count) {
                        ties.push_back(lit);
                    }
                }

                if (lmax == 0) {
                    break;
                }

                int prev_clause_count = matched_clauses.size();
                int new_clause_count = lmax_count;

                int prev_lit_count = matched_lits.size();
                int new_lit_count = prev_lit_count + 1;

                // if adding lmax to Mlit does not result in a reduction then stop
                int current_reduction = reduction(prev_lit_count, prev_clause_count);
                int new_reduction = reduction(new_lit_count, new_clause_count);

                MonPrint("MBVA: lmax: ", lmax, " (", lmax_count, ")");
                MonPrint("MBVA: current_reduction: ", current_reduction);
                MonPrint("MBVA: new_reduction: ", new_reduction);

                if (new_reduction <= current_reduction) {
                    break;
                }

                // Break ties
                if (ties.size() > 1 && tiebreak_mode == Tiebreak::ThreeHop) {
                    int max_heuristic_val = tiebreaking_heuristic(var, ties[0]);
                    for (int i=1; i<ties.size(); i++) {
                        int h = tiebreaking_heuristic(var, ties[i]);
                        if (h > max_heuristic_val) {
                            max_heuristic_val = h;
                            lmax = ties[i];
                        }
                    }
                }


                // Mlit := Mlit U {lmax}
                matched_lits.push_back(lmax);

                // Mcls := Mcls U P[lmax]
                matched_clauses_swap.resize(lmax_count);
                matched_clauses_id_swap.resize(lmax_count);

                int insert_idx = 0;
                for (auto [lit, clause_idx, idx] : matched_entries) {
                    if (lit != lmax)
                        continue;

                    matched_clauses_swap[insert_idx] = matched_clauses[idx];
                    matched_clauses_id_swap[insert_idx] = matched_clauses_id[idx];
                    insert_idx += 1;

                    clauses_to_remove.push_back(make_tuple(clause_idx, matched_clauses_id[idx]));
                }

                swap(matched_clauses, matched_clauses_swap);
                swap(matched_clauses_id, matched_clauses_id_swap);

                /*
                if (enable_trace) {
                    cout << "  Mcls: ";
                    for (int i = 0; i < matched_clauses->size(); i++) {
                        cout << matched_clauses->operator[](i) << " ";
                    }
                    cout << endl;
                    cout << "  Mcls_id: ";
                    for (int i = 0; i < matched_clauses_id->size(); i++) {
                        cout << matched_clauses_id->operator[](i) << " ";
                    }
                    cout << endl;
                }
                */
            }

            if (matched_lits.size() == 1) {
                continue;
            }

            /*
            if (matched_lits->size() <= 2 && matched_clauses->size() <= 2) {
                continue;
            }
            */

            if( (int)(matched_lits.size() * matched_clauses.size()) - (int)(matched_lits.size() + matched_clauses.size()) < minimal_deletion ) {
                continue;
            }

            MonPrint("Simplification: save  ", (int)(matched_lits.size() * matched_clauses.size()) - (int)(matched_lits.size() + matched_clauses.size()), " clauses");

            int matched_clause_count = matched_clauses.size();
            int matched_lit_count = matched_lits.size();

            /*
            if (enable_trace) {
                cout << "  mlits: ";
                for (int i = 0; i < matched_lits->size(); i++) {
                    cout << matched_lits->operator[](i) << " ";
                }
                cout << endl;
                cout << "  mclauses:\n";
                for (int i = 0; i < matched_clauses->size(); i++) {
                    clauses->operator[](matched_clauses->operator[](i)).print();
                }
                cout << endl;

                cout << "--------------------" << endl;
            }
            */

            // Do the substitution
            num_vars += 1;
            int new_var = num_vars;

            // Prepare to add new clauses.
            clauses.resize(num_clauses + matched_lit_count + matched_clause_count); // + (preserve_model_count ? 1 : 0));
            lit_to_clauses.resize(num_vars * 2);
            lit_count_adjust.resize(num_vars * 2);
            if (sparsevec_lit_idx(new_var) >= adjacency_matrix_width) {
                // The vectors must be constructed with a fixed, pre-determined width.
                //
                // This is quite an annoying limitation, as it means we have to re-construct
                // all the vectors if we go above the width limit
                adjacency_matrix_width = num_vars * 2;
                adjacency_matrix.clear();
            }
            adjacency_matrix.resize(num_vars);

            // Add (f, lit) clauses.
            for (int i = 0; i < matched_lit_count; ++i) {
                int lit = matched_lits[i];
                int new_clause = num_clauses + i;

                auto cls = Clause();
                cls.lits.push_back(lit);
                cls.lits.push_back(new_var); // new_var is always largest value
                clauses[new_clause] = cls;

                lit_to_clauses[lit_index(lit)].push_back(new_clause);
                lit_to_clauses[lit_index(new_var)].push_back(new_clause);

                /*
                if (generate_proof) {
                    auto proof_lits = vector<int>();
                    proof_lits.push_back(new_var); // new_var needs to be first for proof
                    proof_lits.push_back(lit);
                    proof->push_back(ProofClause(true, proof_lits));
                }
                */
            }

            // Add (-f, ...) clauses.
            for (int i = 0; i < matched_clause_count; ++i) {
                int clause_idx = matched_clauses[i];
                auto new_clause = num_clauses + matched_lit_count + i;

                auto cls = Clause();
                cls.lits.push_back(-new_var); // -new_var is always smallest value
                lit_to_clauses[lit_index(-new_var)].push_back(new_clause);

                auto match_cls = clauses[clause_idx];
                for (auto mlit : match_cls.lits) {
                    if (mlit != var) {
                        cls.lits.push_back(mlit);
                        lit_to_clauses[lit_index(mlit)].push_back(new_clause);
                    }
                }
                clauses[new_clause] = cls;

                /*
                if (generate_proof) {
                    proof->push_back(ProofClause(true, cls.lits));
                }
                */
            }

            // Preserving model count:
            //
            // The only case where we add a model is if both assignments for the auxiiliary variable satisfy the formula
            // for the same assignment of the original variables. This only happens if all(matched_lits) *AND*
            // all(matches_clauses) are satisfied.
            //
            // The easiest way to fix this is to add one clause that constrains all(matched_lits) => -f
            /*
            if (preserve_model_count) {
                int new_clause = num_clauses + matched_lit_count + matched_clause_count;
                auto cls = Clause();
                cls.lits.push_back(-new_var);
                for (int i = 0; i < matched_lit_count; ++i) {
                    int lit = (*matched_lits)[i];
                    cls.lits.push_back(-lit);
                    (*lit_to_clauses)[lit_index(-lit)].push_back(new_clause);
                }

                (*clauses)[new_clause] = cls;
                (*lit_to_clauses)[(lit_index(-new_var))].push_back(new_clause);

                
                //if (generate_proof) {
                //    proof->push_back(ProofClause(true, cls.lits));
                //}
            }
            */


            set<int> valid_clause_ids;
            for (int i = 0; i < matched_clause_count; ++i) {
                valid_clause_ids.insert(matched_clauses_id[i]);
            }

            // Remove the old clauses.
            int removed_clause_count = 0;
            lits_to_update.clear();

            for (auto [clause_idx, clause_id] : clauses_to_remove) {

                if (valid_clause_ids.find(clause_id) == valid_clause_ids.end()) {
                    continue;
                }

                auto cls = &(clauses[clause_idx]);
                cls->deleted = true;
                removed_clause_count += 1;
                for (auto lit : cls->lits) {
                    lit_count_adjust[lit_index(lit)] -= 1;
                    lits_to_update.insert(lit);
                }

                /*
                if (generate_proof) {
                    proof->push_back(ProofClause(false, cls->lits));
                }
                */
            }

            adj_deleted += removed_clause_count;
            num_clauses += matched_lit_count + matched_clause_count; // + (preserve_model_count ? 1 : 0);

            // Update priorities.
            for (auto lit : lits_to_update) {
                // Q.push(lit);
                pq.push(make_pair(
                    real_lit_count(lit),
                    lit
                ));

                // Reset adjacency matrix
                adjacency_matrix[sparsevec_lit_idx(lit)] = Eigen::SparseVector<int>(adjacency_matrix_width);
            }

            // Q.push(new_var);
            pq.push(make_pair(
                lit_to_clauses[lit_index(new_var)].size() + lit_count_adjust[lit_index(new_var)],
                new_var
            ));

            // Q.push(-new_var);
            pq.push(make_pair(
                lit_to_clauses[lit_index(-new_var)].size() + lit_count_adjust[lit_index(-new_var)],
                -new_var
            ));

            // Q.push(var);
            pq.push(make_pair(
                lit_to_clauses[lit_index(var)].size() + lit_count_adjust[lit_index(var)],
                var
            ));

            num_replacements += 1;
        }
    }

private:
public:
    bool found_header;
    int num_vars;
    int num_clauses;
    int adj_deleted;
    vector<Clause> clauses;

    // maps each literal to a vector of clauses that contain it
    vector< vector<int> > lit_to_clauses;
    vector<int> lit_count_adjust;

    int adjacency_matrix_width;
    vector< Eigen::SparseVector<int> > adjacency_matrix;
    map< int, int > tmp_heuristic_cache_full;

    // proof storage
    //vector<ProofClause> *proof;
};

/*
void runBVA(FILE *fin, FILE *fout, FILE *fproof, Tiebreak tiebreak) {
    Formula *f = Formula::parse(fin);
    f->run(tiebreak);
    f->to_cnf(fout);
    //if (fproof != NULL) {
    //    f->to_proof(fproof);
    //}
}
*/



#endif