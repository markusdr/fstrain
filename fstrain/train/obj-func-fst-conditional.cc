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
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cmath>
#include "fstrain/train/obj-func-fst-conditional.h"
#include "fstrain/train/obj-func-fst.h"
#include "fstrain/train/obj-func-fst-util.h"
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/vector-fst.h"
// #include "fst/arcsort.h"
#include "fstrain/core/expectation-arc.h"
#include "fstrain/core/debug.h"
#include "fstrain/util/string-to-fst.h"
#include "fstrain/util/print-fst.h"
#include "fstrain/util/timer.h"
#include "fstrain/util/memory-info.h"
#include "fstrain/util/compose-fcts.h"

#include <boost/foreach.hpp>
#include <boost/thread.hpp>

using namespace fst;

namespace fstrain { namespace train {

struct ProcessInputOutputPair_Fct {
  ObjectiveFunctionFstConditional* obj;
  const util::Data& data;
  const std::size_t first, last;
  int iteration;  
  ProcessInputOutputPair_Fct(ObjectiveFunctionFstConditional* obj_, 
                             const util::Data& data_,
                             std::size_t first_,
                             std::size_t last_,
                             int iteration_) 
      : obj(obj_), data(data_), first(first_), last(last_), iteration(iteration_) {}
  void operator()() {
    for(std::size_t i = first; i <= last; ++i) {
      const std::pair<std::string, std::string>& inout = data[i];
      obj->ProcessInputOutputPair(inout.first, inout.second, iteration);
      boost::this_thread::interruption_point();
    }
  }
};

struct AbortThreadsOnDivergentObjective {
  ObjectiveFunctionFstConditional* obj;
  const std::vector< boost::shared_ptr<boost::thread> >& watched_threads;
  bool stop;
  AbortThreadsOnDivergentObjective(ObjectiveFunctionFstConditional* obj_,
                                   const std::vector< boost::shared_ptr<boost::thread> >& watched_threads_)
      : obj(obj_), watched_threads(watched_threads_), stop(false) {}
  void Stop() {stop = true;}
  void operator()() {
    while(!stop) {
      if(obj->GetFunctionValue() == core::kPosInfinity) {
        std::cerr << "Divergence. Aborting other threads." << std::endl;
        BOOST_FOREACH(boost::shared_ptr<boost::thread> t_ptr, watched_threads) {
          t_ptr->interrupt();
        }
        break;
      }
      boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }
  }
};

ObjectiveFunctionFstConditional::~ObjectiveFunctionFstConditional() {
  delete data_;
  delete isymbols_;
  delete osymbols_;
  delete compose_input_fct_;
  delete compose_output_fct_;
}

void ObjectiveFunctionFstConditional::ComputeGradientsAndFunctionValue(const double* x) {  
  static int call_counter = -1;
  ++call_counter;
  using nsObjectiveFunctionFstUtil::GetFeatureMDExpectations;

  util::Timer timer;
  size_t num_params = GetNumParameters();
  double* gradients = GetGradients();

  SetFunctionValue(0.0);
  for(size_t i = 0; i < num_params; ++i){
    gradients[i] = 0.0;
  }

  // regularize
  double norm = 0.0;
  for(int i = 0; i < num_params; ++i){
    norm += (x[i] * x[i]);
    gradients[i] += x[i] / variance_;
  }   
  SetFunctionValue(GetFunctionValue() + norm / (2.0 * variance_));

  if(GetNumThreads() == 1) {
    ProcessInputOutputPair_Fct f(this, *data_, 0, data_->size() - 1, call_counter);
    f();
  } 
  else {
    std::vector<boost::shared_ptr<boost::thread> > threads;
    const int num_threads = std::min(GetNumThreads(), (int)data_->size()); // TEST
    int prev_last = -1;
    int block_size = data_->size() / num_threads;
    for(int i = 0; i < num_threads; ++i) {
      const std::size_t first = prev_last + 1;
      const std::size_t last = i < num_threads - 1 ? prev_last + block_size : data_->size() - 1;
      prev_last = last;
      ProcessInputOutputPair_Fct f(this, *data_, first, last, call_counter);
      threads.push_back(boost::shared_ptr<boost::thread>(new boost::thread(f)));    
    }
    std::cerr << "Started " << threads.size() << " threads" << std::endl;  
    AbortThreadsOnDivergentObjective watch_fct(this, threads);
    boost::thread watch_thread(boost::ref(watch_fct));
    BOOST_FOREACH(boost::shared_ptr<boost::thread> t_ptr, threads) {
      t_ptr->join();
    }
    watch_fct.Stop();
    watch_thread.interrupt();
    watch_thread.join();
  }

  std::cerr << setprecision(8) 
            << "Returning x=" << x[0] << "\tg=" << gradients[0] 
            << "\tobj=" << GetFunctionValue();
  timer.stop();
  fprintf(stderr, "\t[%2.2f ms, %2.2f MB]\n", 
          timer.get_elapsed_time_millis(), 
          util::MemoryInfo::instance().getSizeInMB());

  if(GetFunctionValue() == core::kPosInfinity){
    double* gradients = GetGradients();
    // already done in GetFeatureMDExpectations, but not sure due to threads
    const int num_params = GetNumParameters();    
    for(std::size_t i = 0; i < num_params; ++i) {
      gradients[i] = 0.0;
    }
    SetFunctionValue(1.0);
    return;
  }

  SquashFunction();
} 

void ObjectiveFunctionFstConditional::ProcessInputOutputPair(
    const std::string& in, const std::string& out, int iteration) {
  FSTR_TRAIN_DBG_MSG(10, "(" << in << ", " << out << "), iter " << iteration << std::endl);
  using nsObjectiveFunctionFstUtil::GetFeatureMDExpectations;
  VectorFst<MDExpectationArc> inputFst;
  VectorFst<MDExpectationArc> outputFst;
  util::ConvertStringToFst(in, *isymbols_, &inputFst);
  util::ConvertStringToFst(out, *osymbols_, &outputFst);
  assert(inputFst.InputSymbols() == NULL);
  assert(GetFst().InputSymbols() == NULL);
  VectorFst<MDExpectationArc> unclamped;
  VectorFst<MDExpectationArc> clamped;
  //mutex_gradient_access_.lock();
  (*compose_input_fct_)(inputFst, GetFst(), &unclamped);
  //mutex_gradient_access_.unlock();
  (*compose_output_fct_)(unclamped, outputFst, &clamped);
  double* gradients = GetGradients();
  long timelimit = *GetTimelimit(); // copy
  double clamped_result = 0.0;
  try {
    clamped_result = GetFeatureMDExpectations<double, double*>(
        clamped, &gradients, GetNumParameters(), 
        false, 1.0,
        GetFstDelta(), &timelimit, &mutex_gradient_access_);
  }
  catch(...) {
    if(iteration == 0) {
      std::cerr << "Ignoring example: " << in << " / " << out << std::endl;
    }
    return;
  }
  FSTR_TRAIN_DBG_MSG(10, "CLAMPED=" << clamped_result << std::endl);
  long unlimited = -1;
  FSTR_TRAIN_DBG_EXEC(100, util::printFst(&unclamped, NULL, NULL, false, std::cerr));
  double unclamped_result = 
      GetFeatureMDExpectations(unclamped, &gradients, GetNumParameters(), 
                             true, 1.0, GetFstDelta(), 
                             iteration == 0 ? &unlimited : GetTimelimit(), 
                             &mutex_gradient_access_);
  // std::cerr << "New time limit: " << GetTimelimit() << std::endl;
  FSTR_TRAIN_DBG_MSG(10, "UNCLAMPED=" << unclamped_result << std::endl);
  {
    boost::mutex::scoped_lock lock(mutex_functionval_access_);
    SetFunctionValue(GetFunctionValue() + clamped_result - unclamped_result);
  }
}

} } // end namespace fstrain/train
