// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: markus.dreyer@gmail.com (Markus Dreyer)
//
#ifndef FSTRAIN_UTIL_CHECK_CONVERGENCE_H
#define FSTRAIN_UTIL_CHECK_CONVERGENCE_H

#include <vector>
#include <algorithm>
#include "fst/fst.h"
#include "fst/map.h"
#include "fstrain/util/debug.h"
#include "fstrain/util/options.h"

namespace fstrain { namespace util {

using namespace fst;

namespace nsCheckConvergenceUtil {

/**
 * @brief Moves one step further from the current distances. This
 * is a matrix multiplication Ax.
 */
template<class Arc>
void DoOneStep(const fst::Fst<Arc>& fst, 
               std::vector<typename Arc::Weight>* curr_distances) {
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;
  std::vector<Weight> next_distances(curr_distances->size(), Weight::Zero());
  for(StateIterator< Fst<Arc> > siter(fst); !siter.Done(); siter.Next()) {
    StateId s = siter.Value();
    if((*curr_distances)[s] == Weight::Zero()){
      continue;
    }
    for(ArcIterator< Fst<Arc> > aiter(fst, s); !aiter.Done(); aiter.Next()){
      const Arc& arc = aiter.Value();
      next_distances[arc.nextstate] = Plus(next_distances[arc.nextstate], 
                                           Times((*curr_distances)[s], 
                                                 arc.weight));
    }        
  }
  std::copy(next_distances.begin(), next_distances.end(), 
            curr_distances->begin());
}

/**
 * @brief Scales the distances using the L1 norm.
 */
template<class Arc>
void ScaleDistances(std::vector<typename Arc::Weight>* distances) {
  typedef typename Arc::Weight Weight;
  Weight sum = Weight::Zero();
  for(int i = 0; i < distances->size(); ++i){
    sum = Plus(sum, (*distances)[i]);
  }
  for(int i = 0; i < distances->size(); ++i){
    if(sum == Weight::Zero()){
      assert((*distances)[i] == Weight::Zero());
    }
    else{
      (*distances)[i] = Divide((*distances)[i], sum);
    }        
  }      
}

template<class W>
bool IsGreater(const std::vector<W>& x, 
               const std::vector<W>& y) {
  assert(x.size() == y.size());
  for(int i = 0; i < x.size(); ++i){
    if(x[i].Value() <= y[i].Value()){
      return false;
    }
  }    
  return true;
}
    
/**
 * @brief Computes the eigenvector. eigenvector = t(x) A x / t(x)
 * x. This will also run a test that checks if the next step is
 * already smaller than the current state. (See Nagatou & Ishii
 * (2007): Validated computation tool for Perron-Frobenius
 * eigenvalues, theorem 4.2) If so, eigenvalue Zero is returned.
 */
template<class Arc>
typename Arc::Weight ComputeEigenvalue (
    const Fst<Arc>& fst, 
    const std::vector<typename Arc::Weight>& distances) {
  typedef typename Arc::Weight Weight;
  std::vector<Weight> next_distances(distances.size(), Weight::Zero());
  std::copy(distances.begin(), distances.end(),
            next_distances.begin());
  DoOneStep(fst, &next_distances); // Ax
  bool will_converge = IsGreater(next_distances, distances); // greater neglog numbers
  if(will_converge){
    return Weight::Zero(); 
  }
  Weight numerator = Weight::Zero();
  for(int i = 0; i < distances.size(); ++i){ // xAx
    numerator = Plus(numerator, 
                     Times(distances[i], next_distances[i]));
  }
  Weight denominator = Weight::Zero();
  for(int i = 0; i < distances.size(); ++i){ // t(x) %*% x
    denominator = Plus(denominator,
                       Times(distances[i], distances[i]));
  }      
  Weight result = Weight::Zero();
  if(denominator == Weight::Zero()){
    assert(numerator == Weight::Zero());
  }
  else {
    result = Divide(numerator, denominator);
  }
  return result;
}

} // end namespace

struct CheckConvergenceOptions {
  size_t min_iter; // because eigenvalue may not move for the first
  // 1 or 2 or so iterations
  size_t max_iter;
  double eigenval_converge_tol; 
  size_t successive_eigenval_convergence; // how many times in a row
                                          // it needs to appear converged
  size_t successive_good_eigenval; // how many times in a row
                                   // it needs to be < 1 to abort
  size_t successive_bad_eigenval; // how many times in a row
                                   // it needs to be >= 1 to abort
  double* return_eigenvalue;    
  explicit CheckConvergenceOptions(size_t max_iter_ = 20000,
                                   double eigenval_converge_tol_ = 10e-12,
                                   double* return_eigenvalue_ = NULL)
      : min_iter(10), max_iter(max_iter_), 
        eigenval_converge_tol(eigenval_converge_tol_),
        successive_eigenval_convergence(3),
        successive_good_eigenval(1000),
        successive_bad_eigenval(1000),
        return_eigenvalue(return_eigenvalue_) 
  {
    // for now, these global options will overwrite 
    if(options.has("eigenvalue-maxiter")) {
      max_iter = options.get<int>("eigenvalue-maxiter");
      std::cerr << "Setting eigenvalue maxiter " << max_iter << std::endl;
    }
    if(options.has("eigenvalue-convergence-tol")) {
      eigenval_converge_tol = options.get<double>("eigenvalue-convergence-tol");
      std::cerr << "Setting eigenvalue convergence tol " << eigenval_converge_tol << std::endl;
    }    
  }    
};

// Decides if it is a good eigenval
// TODO: check eigenvalue without using Value()
template<class W>
bool IsSmallerThanOne(const W& w) {
  return w.Value() > 0.0; // neglog
}

template<class W>
bool AbortFromBadEigenvals(const W& eigenvalue, size_t successive_bad_eigenval, 
                           size_t *num_bad_eigenval) {
  bool do_abort = false;
  if(!IsSmallerThanOne(eigenvalue)){ // bad
    ++(*num_bad_eigenval);
    if(*num_bad_eigenval >= successive_bad_eigenval){
      do_abort = true;
    }
  }
  else {
    *num_bad_eigenval = 0;
  }
  return do_abort;
}

template<class W>
bool AbortFromGoodEigenvals(const W& eigenvalue, size_t successive_good_eigenval, 
                            size_t *num_good_eigenval) {
  bool do_abort = false;
  if(IsSmallerThanOne(eigenvalue)){ // good
    ++(*num_good_eigenval);
    if(*num_good_eigenval >= successive_good_eigenval){
      do_abort = true;
    }
  }
  else {
    *num_good_eigenval = 0;
  }
  return do_abort;
}


/**
 * @brief Checks if the FST converges, i.e. if there are no infinite
 * loops. Uses the power iteration method to compute the principal
 * (largest) eigenvalue (see here,
 * http://www.cs.princeton.edu/introcs/95linear).  If that
 * eigenvalue is greater than one, then the FST diverges (we count
 * arc and final weights).
 *
 */
template<class Arc>
bool CheckConvergence(
    const Fst<Arc>& fst, 
    const CheckConvergenceOptions& opts = CheckConvergenceOptions()) {
    
  using namespace nsCheckConvergenceUtil;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;
  bool converges = true;

  size_t num_states = 0;
  for(StateIterator< Fst<Arc> > siter(fst); !siter.Done(); siter.Next()) {
    ++num_states;
  }
  std::vector<Weight> distances(num_states, Weight::Zero());
  distances[fst.Start()] = Weight::One();
  //srand(time(NULL)); 
  //for(int i = 0; i < distances.size(); ++i){
  //  distances[i] = -1.0 * rand();
  //}

  size_t iter = 0;    
  bool done = false;
  Weight eigenvalue = Weight::Zero();
  size_t num_converged = 0;
  // size_t num_good_eigenval = 0;
  size_t num_bad_eigenval = 0;
  while(!done && iter < opts.max_iter){   
    if((iter + 1) % 1000 == 0){
      std::cerr << "Convergence test, iteration " << iter << " ..." 
		<< std::endl;
    }
    DoOneStep(fst, &distances);
    ScaleDistances<Arc>(&distances);
    Weight new_eigenvalue = ComputeEigenvalue(fst, distances);
    double relative_change = 
        (exp(-new_eigenvalue.Value()) - exp(-eigenvalue.Value()))
        / (exp(-eigenvalue.Value())+10e-16);
    bool eigenvalue_converged = iter == 0 ? false
        : std::abs(relative_change) < opts.eigenval_converge_tol;
    if(eigenvalue_converged){
      ++num_converged;      
      if(num_converged >= opts.successive_eigenval_convergence 
         && iter >= opts.min_iter){
        break;
      }
    }
    else {
      if(AbortFromBadEigenvals(new_eigenvalue, opts.successive_bad_eigenval, &num_bad_eigenval)) {
        std::cerr << "Seen " << num_bad_eigenval << " bad eigenvalues. Diverge." << std::endl;
        break;
      }
//      else if(AbortFromGoodEigenvals(new_eigenvalue, opts.successive_good_eigenval, &num_good_eigenval)) {
//        std::cerr << "Seen " << num_good_eigenval << " good eigenvalues. Converge." << std::endl;
//        break;
//      }
      num_converged = 0;
    }
    eigenvalue = new_eigenvalue;
    FSTR_UTIL_DBG_MSG(10, "it " << iter << ", eigenvalue = " << exp(-1.0 * eigenvalue.Value()) << std::endl);
    ++iter;
  }        

  FSTR_UTIL_DBG_MSG(10,
		    "eigenvalue=" << exp(-eigenvalue.Value()) 
		    << ", " << iter << " iterations." << std::endl;);
  
  if(iter >= opts.max_iter){
    std::cerr << "Reached max iter " << opts.max_iter << std::endl;
    // TODO: pass option in, do not use global options
    const bool max_iter_means_diverge = !(options.has("eigenvalue-maxiter-checkvalue")
					  && options.get<bool>("eigenvalue-maxiter-checkvalue"));
    if(max_iter_means_diverge) {
      std::cerr << "Returning divergence." << std::endl;
      converges = false; 
    }
  }   
  if(!IsSmallerThanOne(eigenvalue)) {
    FSTR_UTIL_DBG_MSG(1, "Divergence. Eigenvalue = " 
                      << exp(-1.0 * eigenvalue.Value()) << std::endl);
    converges = false;
  }
    
  FSTR_UTIL_DBG_MSG(10, "Eigenvalue = " 
                    << exp(-1.0 * eigenvalue.Value()) << std::endl);    
  if(opts.return_eigenvalue != NULL){
    *opts.return_eigenvalue = exp(-1.0 * eigenvalue.Value());
  }    
  return converges;
}

} } // end namespace fstrain::util

#endif
