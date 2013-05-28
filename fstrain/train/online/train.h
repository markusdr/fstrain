#ifndef FSTRAIN_TRAIN_ONLINE_TRAIN_H
#define FSTRAIN_TRAIN_ONLINE_TRAIN_H

#include <vector>
#include <cmath>
#include <map>

#include "fstrain/util/timer.h"
#include "fstrain/util/memory-info.h"

namespace fstrain { namespace train { namespace online {

///////////////////////////////////////////////

class Batches {
 public:
  virtual ~Batches() {}
  typedef std::map<int,double> Map;
  virtual void Init() = 0;
  virtual bool Done() const = 0;
  virtual void Next() = 0;
  virtual Map Value() const = 0;
  virtual int GetBatchSize() const = 0;
};

class MockBatches : public Batches {
 public:
  void Init() {}
  bool Done() const {
    return true;
  }
  void Next() {}
  Batches::Map Value() const {
    return Batches::Map();
  }
  int GetBatchSize() const {
    return 1;
  }
};

///////////////////////////////////////////////

class LearningRateFct {
 public:
  virtual ~LearningRateFct() {}
  virtual double operator()(int k) = 0;
};

class ConstantLearningRateFct : public LearningRateFct {
  double eta_;
 public:
  ConstantLearningRateFct(double eta) : eta_(eta) {}
  double operator()(int k) { return eta_; }
};

class ExponentialLearningRateFct : public LearningRateFct {
  const double N_;
  const double eta_0_;
  const double alpha_;
 public:
  ExponentialLearningRateFct(size_t N,
                             double eta_0 = 0.2,
                             double alpha = 0.85) // recommended by Tsuruoka et al. (ACL 2009)
      : N_(static_cast<double>(N)), eta_0_(eta_0), alpha_(alpha) {}
  double operator()(int k) {
    return eta_0_ * pow(alpha_, k / N_);
  }
};

///////////////////////////////////////////////

class SgdTrainer {

  int num_passes_;
  Batches& batches_;
  LearningRateFct& learning_rate_fct_;
  std::vector<double>* weights_;

 public:

  SgdTrainer(int num_passes,
             Batches& batches,
             LearningRateFct& learning_rate_fct,
             std::vector<double>* weights) :
      num_passes_(num_passes),
      batches_(batches),
      learning_rate_fct_(learning_rate_fct),
      weights_(weights)
  {}

  void Train() {
    int k = 0;
    for(int pass = 0; pass < num_passes_; ++pass) {
      std::cerr << "PASS " << pass << std::endl;
      for(batches_.Init(); !batches_.Done(); batches_.Next()) {
        const double eta = learning_rate_fct_(k);
        Batches::Map batch_gradients = batches_.Value();
        for(Batches::Map::const_iterator it = batch_gradients.begin();
            it != batch_gradients.end(); ++it) {
          (*weights_)[it->first] += eta * it->second;
        }
        ++k;
      }
    }
  }

};

///////////////////////////////////////////////

class CumulativeL1Trainer {

  int num_passes_;
  Batches& batches_;
  LearningRateFct& learning_rate_fct_;
  std::vector<double>* weights_;
  double u_;
  const double C_; // regularization strength
  const int batch_size_;
  std::vector<double> q_;
  std::set<int> zero_weights_; // TEST

 public:

  CumulativeL1Trainer(int num_passes,
                      Batches& batches,
                      LearningRateFct& learning_rate_fct,
                      const double C,
                      std::vector<double>* weights) :
      num_passes_(num_passes),
      batches_(batches),
      learning_rate_fct_(learning_rate_fct),
      weights_(weights),
      u_(0),
      C_(C),
      batch_size_(batches.GetBatchSize())
  {
    q_.resize(weights_->size());
  }

  void Train() {
    int k = 0;
    for(int pass = 0; pass < num_passes_; ++pass) {
      util::Timer timer;
      std::cerr << "PASS " << pass << std::endl;
      int cnt = 1;
      for(batches_.Init(); !batches_.Done(); batches_.Next(), ++cnt) {
        if(cnt % 1000 == 0) {
          std::cerr << cnt << " ..." << std::endl;
        }
        const double eta = learning_rate_fct_(k);
        u_ += eta * C_ / batch_size_;
        Batches::Map batch_gradients = batches_.Value();
        for(Batches::Map::const_iterator it = batch_gradients.begin();
            it != batch_gradients.end(); ++it) {
          (*weights_)[it->first] += eta * it->second;
          ApplyPenalty(it->first);
          if((*weights_)[it->first] == 0.0) {
            zero_weights_.insert(it->first);
          }
        }
        ++k;
      }
      timer.stop();
      fprintf(stderr, "[%2.2f ms, %2.2f MB]\n",
              timer.get_elapsed_time_millis(),
              util::MemoryInfo::instance().getSizeInMB());
      std::cerr << "Num zero weights: " << zero_weights_.size() << std::endl;
    }

  }

 private:

  void ApplyPenalty(int i) {
    double& w_i = (*weights_)[i];
    double z = w_i;
    if(w_i > 0) {
      w_i = std::max(0.0, w_i - (u_ + q_[i]));
    }
    else if(w_i < 0) {
      w_i = std::min(0.0, w_i + (u_ - q_[i]));
    }
    q_[i] += (w_i - z);
  }

};

///////////////////////////////////////////////

} } } // end namespaces

#endif
