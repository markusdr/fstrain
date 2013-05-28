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
#ifndef FSTRAIN_TRAIN_OBJ_FUNC_FST_UTIL_H
#define FSTRAIN_TRAIN_OBJ_FUNC_FST_UTIL_H

#include <iostream>
#include <cmath>
#include "fst/fst.h"
#include "fst/map.h"
#include "fst/mutable-fst.h"
#include "fst/vector-fst.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/train/shortest-distance-timeout.h"
#include "fstrain/util/shortest-distance-m.h"
#include "fstrain/train/timeout.h"
// #include "fstrain/core/fst-util.h"
#include "fstrain/util/timer.h"
#include "fstrain/util/check-convergence.h"
#include "fstrain/util/double-precision-weight.h"
#include "fstrain/train/debug.h"
#include <boost/thread/mutex.hpp>

namespace fstrain { namespace train {

namespace nsObjectiveFunctionFstUtil {

/**
 * @brief Runs ShortestDistance with a timeout; a timeout is seen as
 * an indication that it might diverge and it runs the eigenvalue
 * check to make sure.
 *
 * @param timelimit_ms The time limit for shortest-distance in ms
 * (needed for potentially divergent FSTs); use -1 for no limit; value
 * will be set to max(timelimit_ms, elapsed time) which may be bigger
 * than timelimit_ms if FST was found to be convergent after timeout.
 *
 * @return true if ShortestDistance was successful, false if it diverged
 */
template<class Arc>
bool TimedShortestDistance(const fst::Fst<Arc>& fst,
			   std::vector<typename Arc::Weight>* result,
			   bool reverse,
			   double kDelta,
			   long* timelimit_ms) {
  using util::LogDArc;
  if(*timelimit_ms >= 0 && *timelimit_ms < 100){
    FSTR_TRAIN_DBG_MSG(10, "Resetting time limit to " << 100 << std::endl);
    *timelimit_ms = 100;
  }
  Timeout timeout(*timelimit_ms);
  ShortestDistance(fst, result, reverse, kDelta, &timeout);
  if(timeout()){
    std::cerr << *timelimit_ms << " ms reached." << std::endl;
    // bool ok = util::CheckConvergence(fst);
    typedef fst::WeightConvertMapper<Arc, LogDArc> Map_AL;
    fst::MapFstOptions map_opts;
    map_opts.gc_limit = 0;  // no caching to save memory
    util::CheckConvergenceOptions opts;
    fst::MapFst<Arc, LogDArc, Map_AL> mapped_fst(fst, Map_AL(), map_opts);
    bool ok = util::CheckConvergence(mapped_fst, opts);
    if(!ok){
      std::cerr << "DIVERGE" << std::endl;
      return false;
    }
    std::cerr << "CONVERGE" << std::endl;
    std::cerr << "Rerun without time limit" << std::endl;
    timeout = Timeout(-1);
    const std::string matrix_opt = "use-matrix-distance";
    if(util::options.has(matrix_opt) && util::options.get<bool>(matrix_opt)) {
      util::ShortestDistanceM(fst, result, reverse, 1e-5, 100, 10000);
    }
    else {
      train::ShortestDistance(fst, result, reverse, kDelta, &timeout);
    }
    long elapsed = timeout.GetElapsedTime();
    std::cerr << "Done. Took " << elapsed << " ms." << std::endl;
    // allow up to twice as long next time
    *timelimit_ms = std::max(*timelimit_ms, std::min(*timelimit_ms * 2, (long)elapsed));
    std::cerr << "New limit: " << *timelimit_ms << " ms." << std::endl;
  }
  return true;
}

template<class ArrayT>
void ResetArray(ArrayT* array, int array_size, boost::mutex* mutex = NULL) {
  if(mutex != NULL) {
    mutex->lock();
  }
  for(int i = 0; i < array_size; ++i){
    (*array)[i] = 0.0;
  }
  if(mutex != NULL) {
    mutex->unlock();
  }
}

template<class DoubleT, class ArrayT>
DoubleT GetFeatureMDExpectations(const fst::Fst<fst::MDExpectationArc>& fst,
			       ArrayT* array,
			       int array_size,
			       bool negate,
			       DoubleT factor, // = 1.0,
			       DoubleT kDelta , //= 1e-10,
			       long* timelimit_ms,
                               boost::mutex* mutex_gradient_access = NULL) {
  // util::printTransducer(&fst, NULL, NULL, std::cerr);
  // util::printFstSize("", &fst, std::cerr);
  using util::LogDArc;
  using util::LogDWeight;
  using core::MDExpectations;
  using core::NeglogNum;
  using core::NeglogTimes;
  using core::NeglogDivide;
  typedef fst::MDExpectationArc::StateId StateId;
  typedef fst::WeightConvertMapper<fst::MDExpectationArc, LogDArc> Map_EL;
  fst::MapFstOptions map_opts;
  map_opts.gc_limit = 0;  // no caching to save memory
  fst::MapFst<fst::MDExpectationArc, LogDArc, Map_EL> mapped(fst, Map_EL(), map_opts);

  std::vector<LogDWeight> alphas;
  std::vector<LogDWeight> betas;
  LogDArc::StateId start_state = mapped.Start();

  bool success1 = TimedShortestDistance(mapped, &alphas, false, kDelta, timelimit_ms);
  // TODO: handle mutex
  if(!success1) {
    ResetArray(array, array_size, mutex_gradient_access);
    return fstrain::core::kNegInfinity;
  }
  assert(alphas.size() > 0);

  bool success2 = TimedShortestDistance(mapped, &betas, true, kDelta, timelimit_ms);
  if(!success2){
    ResetArray(array, array_size, mutex_gradient_access);
    return fstrain::core::kNegInfinity;
  }
  const bool no_paths_fst = (betas.size() == 0);
  if(no_paths_fst){
    throw std::runtime_error("no paths: bad fst");
  }

  MDExpectations feat_expectations;
  for (fst::StateIterator< fst::Fst<fst::MDExpectationArc> > siter(fst); !siter.Done(); siter.Next()){
    StateId in = siter.Value();
    assert(alphas.size() > in);
    for (fst::ArcIterator<fst::Fst<fst::MDExpectationArc> > aiter(fst, in); !aiter.Done(); aiter.Next()){
      StateId out = aiter.Value().nextstate;
      if(betas.size() > out && betas[out] != LogDWeight::Zero()
         && betas[out].Value() == betas[out].Value()) { // fails for NaN
        const MDExpectations& e = aiter.Value().weight.GetMDExpectations();
        for(MDExpectations::const_iterator it = e.begin(); it != e.end(); ++it){
          const int index = it->first;
          const NeglogNum& expectation = it->second;
	  NeglogNum addval = NeglogTimes(NeglogTimes(alphas[in].Value(), expectation),
					 betas[out].Value());
	  MDExpectations::iterator found = feat_expectations.find(index);
	  if(found != feat_expectations.end()) {
	    found->second = NeglogPlus(found->second, addval);
	  }
	  else {
	    feat_expectations.insert(index, addval);
	  }
        }
      }
    }
  }

  // TODO: use DoubleT
  if(mutex_gradient_access != NULL) {
    mutex_gradient_access->lock();
  }
  for(MDExpectations::const_iterator it = feat_expectations.begin();
      it != feat_expectations.end(); ++it){
    int index = it->first;
    double expected_count = GetOrigNum(NeglogDivide(it->second, betas[start_state].Value()));
    (*array)[index] += factor * (negate ? -expected_count : expected_count);
  }
  if(mutex_gradient_access != NULL) {
    mutex_gradient_access->unlock();
  }

  DoubleT result = betas[start_state].Value();
  // std::cerr << "result=" << result << std::endl;
  return factor * result;
}

} // end namespace nsObjectiveFunctionFstUtil

} } // end namespace train

#endif
