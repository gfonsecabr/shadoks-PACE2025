#pragma once
#include "hypergraph.hpp"
#include <cassert>
#include <glpk.h>
#include <numeric>

class Mip {
protected:
  const HyperGraph &hg;
  std::vector<int> solution;
public:
  Mip(const HyperGraph &_hg)
  : hg(_hg) {
  }

  bool solveExact(double timeout) {
    int m =  hg.countEdges();
    if(m == 0)
      return true;
    // std::cout << std::endl << hg.countVertices() << ' ' << m << std::endl;
    glp_prob *mip = glp_create_prob();
    glp_set_prob_name(mip, "hs");
    glp_set_obj_dir(mip, GLP_MIN);


    glp_add_cols(mip, hg.countVertices());
    for(int v = 0; v < hg.countVertices(); v++) {
      std::string name = "v"+std::to_string(v);
      glp_set_col_kind(mip, v + 1, GLP_BV);
      glp_set_col_name(mip, v + 1, name.c_str());
      glp_set_obj_coef(mip, v + 1, 1.0);
    }

    glp_add_rows(mip, m);
    for(int e = 0; e < m; e++) {
      std::string name = "row"+std::to_string(e);
      glp_set_row_bnds(mip, e + 1, GLP_LO, 1.0, 1.0);
      glp_set_row_name(mip, e + 1, name.c_str());

      int *ind = new int[1 + hg.edges[e].size()];
      double *val = new double[1 + hg.edges[e].size()];
      for(int j = 0; j < (signed) hg.edges[e].size(); j++) {
        ind[j+1] = hg.edges[e][j] + 1;
        val[j+1] = 1.0;
      }
      glp_set_mat_row(mip, e + 1, hg.edges[e].size(), ind, val);
      delete[] ind;
      delete[] val;
    }

    glp_iocp parm;
    glp_init_iocp(&parm);
    parm.presolve = GLP_ON;
    parm.tm_lim = timeout * 1000;
    auto nullOutput = [](void *info, const char *s) {return 1;};
    glp_term_hook(nullOutput, NULL);
    glp_intopt(mip, &parm);

    if(glp_mip_status(mip) != GLP_OPT) {
      glp_delete_prob(mip);
      // std::cout << '!' << std::flush;
      return false;
    }

    solution.clear();
    for(int v = 0; v < hg.countVertices(); v++) {
      if(glp_mip_col_val(mip, v + 1) > .5)
        solution.push_back(v);
    }

    glp_delete_prob(mip);
    return true;
  }

  const std::vector<int> &getSolution() const {
    return solution;
  }

  const std::vector<int> getSolutionLabels() const {
    return hg.getLabels(solution);
  }
};
