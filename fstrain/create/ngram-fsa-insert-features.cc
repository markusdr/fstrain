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

#include "fstrain/create/ngram-fsa-insert-features.h"

#include "fst/fst.h"
#include "fst/arc.h"
#include "fst/string-weight.h"
#include "fst/map.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"
#include "fst/shortest-distance.h"

#include "fstrain/core/expectation-arc.h"
#include "fstrain/util/string-weight-mapper.h"
#include "fstrain/util/print-fst.h"
#include "fstrain/create/trie-insert-features.h" // MyFeatureSet
#include "fstrain/create/debug.h"

#include <string>
#include <vector>

namespace fstrain { namespace create {

void NgramFsaInsertFeatures(const fst::SymbolTable& symbols,
                            fst::SymbolTable* feature_ids,
                            fstrain::create::features::ExtractFeaturesFct& extract_feats_fct,
                            const std::string prefix,
                            fst::MutableFst<fst::MDExpectationArc>* fst) {

  using namespace fst;

  FSTR_CREATE_DBG_EXEC(10, std::cerr << "Called NgramFsaInsertFeatures on FSA:" << std::endl;
                       util::printTransducer(fst, &symbols, &symbols, std::cerr));

  typedef StringArc<STRING_RIGHT> StrArc;
  typedef StrArc::Weight StrWeight;

  MapFstOptions nopts;
  nopts.gc_limit = 0;  // Cache only the last state for fastest map.
  typedef ToStringMapper<MDExpectationArc, STRING_RIGHT> Map_ES;
  MapFst<MDExpectationArc, StrArc, Map_ES> mapped(*fst, Map_ES(), nopts);

  // Finds the history of each state
  std::vector<StrWeight> alphas;
  fst::ShortestDistance(mapped, &alphas);

  for(StateIterator< Fst<MDExpectationArc> > siter(*fst); !siter.Done();
      siter.Next()) {
    MDExpectationArc::StateId s = siter.Value();
    std::stringstream state_history;
    bool first = true;
    for(StringWeightIterator<int, STRING_RIGHT> iter(alphas[s]); !iter.Done();
        iter.Next()) {
      if(!first) {
        // state_history.push_back(' ');
        state_history << " ";
      }
      // state_history.push_back(iter.Value());
      state_history << symbols.Find(iter.Value());
      first = false;
    }
    const std::string state_history_str = state_history.str();
    FSTR_CREATE_DBG_MSG(10, "history(" << s << ") = " << state_history_str << std::endl);
    for(MutableArcIterator< MutableFst<MDExpectationArc> > aiter(fst, s);
        !aiter.Done(); aiter.Next()){
      MDExpectationArc arc = aiter.Value();
      const std::string blank = (state_history_str.empty() ? "" : " ");
      const std::string window = state_history_str + blank + symbols.Find(arc.olabel);
      typedef fstrain::create::nsTrieInsertFeaturesUtil::MyFeatureSet FeatSet;
      FeatSet feat_set(feature_ids, &arc.weight.GetMDExpectations());
      feat_set.SetPrefix(prefix);
      extract_feats_fct(window, &feat_set);
      aiter.SetValue(arc);
    }
  }

}

} } // end namespaces
