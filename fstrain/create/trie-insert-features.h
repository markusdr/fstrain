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
#ifndef FSTRAIN_CREATE_TRIE_INSERT_FEATURES_H
#define FSTRAIN_CREATE_TRIE_INSERT_FEATURES_H

#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fstrain/core/expectations.h"
#include "fstrain/create/debug.h"
// #include "fstrain/create/feature-functions.h"
// #include "fstrain/create/get-features.h"
// #include "fstrain/util/options.h"
#include "fstrain/create/features/extract-features.h"
#include "fstrain/create/features/feature-set.h"

#include <set>

namespace fstrain { namespace create {

namespace nsTrieInsertFeaturesUtil {

struct FeatureSetWrapper {

  explicit FeatureSetWrapper(fst::SymbolTable* syms,
                             core::MDExpectations* expectations)
      : syms_(syms), expectations_(expectations) {}

  void insert(const std::string& str) {
    int64 id = syms_->AddSymbol(str);
    expectations_->insert(id, 0.0);
  }

  fst::SymbolTable* syms_;
  core::MDExpectations* expectations_;
};

struct MyFeatureSet : public features::IFeatureSet {

  explicit MyFeatureSet(fst::SymbolTable* syms,
			     core::MDExpectations* expectations)
      : syms_(syms), expectations_(expectations),
        feature_set_wrapper_(syms, expectations)
  {}

  void insert(const std::string& str) {
    const std::string s = prefix_ + str;
    feature_set_wrapper_.insert(s);
  }

  void SetPrefix(const std::string& prefix) {
    prefix_ = prefix;
  }

  fst::SymbolTable* syms_;
  core::MDExpectations* expectations_;
  FeatureSetWrapper feature_set_wrapper_;
  std::string prefix_;
};

template<class Arc>
void TrieInsertFeatures0(const fst::SymbolTable& syms,
                         std::string history,
                         fst::SymbolTable* feature_ids,
                         features::ExtractFeaturesFct& extract_features_fct,
                         const std::string prefix,
                         fst::MutableFst<Arc>* trie,
                         typename Arc::StateId state) {
  using namespace fst;
  typedef typename Arc::StateId StateId;
  for (fst::MutableArcIterator< MutableFst<Arc> > ait(trie, state);
      !ait.Done(); ait.Next()) {
    Arc arc = ait.Value();
    const std::string symbol = syms.Find(arc.ilabel);
    if (symbol == "") {
      FSTR_CREATE_EXCEPTION("Could not find label " << arc.ilabel);
    }
    const std::string nexthist = history + (history.length() ? " " : "")
        + symbol;
    core::MDExpectations& expectations = arc.weight.GetMDExpectations();
    MyFeatureSet featset(feature_ids, &expectations);
    featset.SetPrefix(prefix);
    // GetFeatures(nexthist, &featset);
    extract_features_fct(nexthist, &featset);
    ait.SetValue(arc);
    TrieInsertFeatures0(syms, nexthist, feature_ids, extract_features_fct,
                        prefix, trie, arc.nextstate);

  }
}

} // end namespace

/**
 * @brief Inserts features into trie by expressing history as string
 * (e.g. S|S-c1 a|x-c1) and getting features using GetFeatures.
 */
template<class Arc>
void TrieInsertFeatures(const fst::SymbolTable& syms,
                        fst::SymbolTable* feature_ids,
                        features::ExtractFeaturesFct& extract_features_fct,
                        const std::string prefix,
                        fst::MutableFst<Arc>* trie) {
  using namespace nsTrieInsertFeaturesUtil;
  std::string history;
  TrieInsertFeatures0(syms, history,
                      feature_ids,
                      extract_features_fct,
                      prefix,
                      trie, trie->Start());
}

} } // end namespaces fstrain::create

#endif
