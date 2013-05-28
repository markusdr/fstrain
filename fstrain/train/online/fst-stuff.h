#ifndef FSTRAIN_TRAIN_ONLINE_FST_STUFF_H
#define FSTRAIN_TRAIN_ONLINE_FST_STUFF_H

/**
 * This file is for online training code that uses FST operations.
 */

#include "fst/mutable-fst.h"
#include "fst/vector-fst.h"
#include "fst/symbol-table.h"
#include "fst/map.h"
#include "fst/compose.h"
#include "fst/shortest-distance.h"

#include "fstrain/train/online/train.h"
#include "fstrain/train/set-feature-weights.h"
#include "fstrain/util/data.h"
#include "fstrain/util/print-fst.h"
#include "fstrain/util/string-to-fst.h"
#include "fstrain/util/double-precision-weight.h"
#include "fstrain/core/neg-log-of-signed-num.h"

namespace fst {
/**
 * @brief Converts to different Arc type and also inserts new weight
 * based on features of old arc.
 */
template <class FromArc, class ToArc>
struct ConvertAndUpdateWeightMapper {
  explicit ConvertAndUpdateWeightMapper(const double* feat_weights)
      : feat_weights_(feat_weights) {}
  ToArc operator()(const FromArc &arc) const {
    using fstrain::core::MDExpectations;
    using fstrain::core::NeglogNum;
    const MDExpectations& e = arc.weight.GetMDExpectations();
    double w = 0.0;
    if (e.size() == 0) {
      w = arc.weight.Value();
    }
    else {
      for (MDExpectations::const_iterator it = e.begin(); it != e.end(); ++it) {
        w += feat_weights_[it->first]; // assume it fires only once
      }
    }
    return ToArc(arc.ilabel, arc.olabel, typename ToArc::Weight(w), arc.nextstate);
  }
  MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; }
  MapSymbolsAction InputSymbolsAction() const { return MAP_COPY_SYMBOLS; }
  MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS;}
  uint64 Properties(uint64 props) const { return props; }
  const double* feat_weights_;
};

}

namespace fstrain { namespace train { namespace online {

template<class Arc, class Map>
void AddFeatMDExpectations(const fst::Fst<Arc>& fst,
                         const double* weights,
                         bool negate,
                         Map* result) {
  using fstrain::util::LogDArc;
  using fstrain::util::LogDWeight;
  using fstrain::core::MDExpectations;
  using fstrain::core::NeglogNum;
  typedef typename Arc::Weight Weight;
  typedef typename Arc::StateId StateId;
  typedef fst::WeightConvertMapper<fst::MDExpectationArc, LogDArc> Map_EL;
  // typedef fst::ConvertAndUpdateWeightMapper<fst::MDExpectationArc, LogDArc> Map_EL;

  fst::MapFstOptions map_opts;
  map_opts.gc_limit = 0;  // no caching to save memory
  // fst::MapFst<Arc, LogDArc, Map_EL> mapped(fst, Map_EL(weights), map_opts);
  fst::MapFst<Arc, LogDArc, Map_EL> mapped(fst, Map_EL(), map_opts);
  LogDArc::StateId start_state = mapped.Start();

  std::vector<LogDArc::Weight> alphas;
  std::vector<LogDArc::Weight> betas;
  fst::ShortestDistance(mapped, &alphas);
  fst::ShortestDistance(mapped, &betas, true);
  const bool no_paths_fst = (betas.size() == 0);
  if (no_paths_fst) {
    throw std::runtime_error("no paths: bad fst");
  }

  MDExpectations feat_expectations;
  for (fst::StateIterator< fst::Fst<fst::MDExpectationArc> > siter(fst);
       !siter.Done(); siter.Next()) {
    StateId in = siter.Value();
    assert(alphas.size() > in);
    for (fst::ArcIterator<fst::Fst<fst::MDExpectationArc> > aiter(fst, in);
         !aiter.Done(); aiter.Next()) {
      StateId out = aiter.Value().nextstate;
      if (betas.size() > out && betas[out] != LogDWeight::Zero()
         && betas[out].Value() == betas[out].Value()) { // fails for NaN
        const MDExpectations& e = aiter.Value().weight.GetMDExpectations();
        for (MDExpectations::const_iterator it = e.begin(); it != e.end(); ++it) {
          const int index = it->first;
          const NeglogNum& expectation = it->second;
	  NeglogNum addval =
              NeglogTimes(NeglogTimes(alphas[in].Value(), expectation),
                          betas[out].Value());
	  MDExpectations::iterator found = feat_expectations.find(index);
	  if (found != feat_expectations.end()) {
	    found->second = NeglogPlus(found->second, addval);
	  }
	  else {
	    feat_expectations.insert(index, addval);
	  }
        }
      }
    }
  }

  for (MDExpectations::const_iterator it = feat_expectations.begin();
      it != feat_expectations.end(); ++it) {
    int index = it->first;
    double expected_count =
        GetOrigNum(NeglogDivide(it->second, betas[start_state].Value()));
    (*result)[index] += (negate ? -expected_count : expected_count);
  }

}

template<class Arc, class Map>
void AddGradients(const fst::Fst<Arc>& fst,
                  const double* weights,
                  const fst::SymbolTable& isyms,
                  const fst::SymbolTable& osyms,
                  const fstrain::util::Data& data,
                  int data_index,
                  Map* result) {
  using namespace fst;
  const std::pair<std::string,std::string>& data_point = data[data_index];
  VectorFst<Arc> input_fst;
  VectorFst<Arc> output_fst;
  util::ConvertStringToFst(data_point.first, isyms, &input_fst);
  util::ConvertStringToFst(data_point.second, osyms, &output_fst);
  VectorFst<Arc> unclamped;
  VectorFst<Arc> clamped;
  Compose(input_fst, fst, &unclamped);
  fst::Map(&unclamped, SetFeatureWeightsMapper<double>(weights));
  Compose(unclamped, output_fst, &clamped);
  AddFeatMDExpectations(clamped, weights, true, result);
  AddFeatMDExpectations(unclamped, weights, false, result);
}

template<class Arc>
class FstBatches : public Batches {

  const fst::Fst<Arc>& orig_fst_;
  fst::Fst<Arc>* model_fst_;
  double* weights_;
  const fst::SymbolTable& isyms_;
  const fst::SymbolTable& osyms_;
  const fstrain::util::Data& data_;
  const int batch_size_;
  int data_index_;

 public:

  FstBatches(const fst::Fst<Arc>& model_fst,
             double* weights,
             const fst::SymbolTable& isyms,
             const fst::SymbolTable& osyms,
             const fstrain::util::Data& data,
             int batch_size)
      : orig_fst_(model_fst), model_fst_(NULL), weights_(weights), isyms_(isyms), osyms_(osyms),
        data_(data), batch_size_(batch_size), data_index_(0)
  {
    // UpdateWeights();
  }

  ~FstBatches() {
    delete model_fst_;
  }

  /**
   * @brief Initializes a pass over the data (permutes the data)
   */
  void Init() {
    data_index_ = 0;
    // TODO: permute data
  }

  bool Done() const {
    return data_index_ >= data_.size();
  }

  void Next() {
    // UpdateWeights();
    ++data_index_;
  }

  Batches::Map Value() const {
    Batches::Map gradients;
    AddGradients(orig_fst_, weights_, isyms_, osyms_, data_, data_index_, &gradients);
    // AddGradients(*model_fst_, weights_, isyms_, osyms_, data_, data_index_, &gradients);
    return gradients;
  }

  int GetBatchSize() const {
    return batch_size_;
  }

 private:

  void UpdateWeights() {
    // This should not be necesary ...
    typedef fstrain::train::SetFeatureWeightsMapper<double> WInsMapper;
    fst::MapFstOptions opts;
    opts.gc = false;
    opts.gc_limit = 0;
    delete model_fst_;
    model_fst_ = new fst::MapFst<Arc, Arc, WInsMapper> (orig_fst_,
                                                        WInsMapper(weights_),
                                                        opts);
  }


};

} } } // end namespaces

#endif
