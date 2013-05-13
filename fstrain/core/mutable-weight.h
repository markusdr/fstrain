#ifndef FSTRAIN_CORE_MUTABLE_WEIGHT_H
#define FSTRAIN_CORE_MUTABLE_WEIGHT_H

namespace fstrain {
namespace core {

// Workaround because OpenFst's Weight SetValue is not public anymore.
template<class W>
class MutableWeight : public W {

 public:
  template<class FloatT>
  void SetValue(FloatT f) {
    W::SetValue(f);
  }

};

}
}

#endif
