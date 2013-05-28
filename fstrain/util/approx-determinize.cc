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
#ifndef FSTRAIN_UTIL_APPROX_DETERMINIZE_H
#define FSTRAIN_UTIL_APPROX_DETERMINIZE_H

#include "fstrain/util/approx-determinize.h"
#include "fst/determinize.h"
#include "fst/map.h"
#include "fst/shortest-path.h"
#include "fst/vector-fst.h"
#include "fst/rmepsilon.h"

namespace fstrain { namespace util {

void approxDeterminize(const fst::Fst<fst::StdArc>& ifst, fst::MutableFst<fst::StdArc>* ofst, int k, float delta){
  using namespace fst;
  VectorFst<StdArc> bestpaths;
  ShortestPath(ifst, &bestpaths, k);
  MapFst<StdArc, LogArc, StdToLogMapper > bestPathsMappedToLog(bestpaths, StdToLogMapper());
  VectorFst<LogArc> determinized;
  Determinize<LogArc>(RmEpsilonFst<LogArc>(bestPathsMappedToLog),
                      &determinized);
  //DeterminizeOptions(CacheOptions(true, 0), delta));
  Map<LogArc, StdArc, LogToStdMapper >(determinized, ofst, LogToStdMapper());
}

void approxDeterminize(const fst::Fst<fst::LogArc>& ifst, fst::MutableFst<fst::LogArc>* ofst, int k, float delta){
  using namespace fst;

  LogToStdMapper toStdMapper;
  VectorFst<StdArc> bestpaths;
  ShortestPath(MapFst<LogArc, StdArc, LogToStdMapper >(ifst, toStdMapper), &bestpaths, k);

  StdToLogMapper toLogMapper;
  *ofst = DeterminizeFst<LogArc>(RmEpsilonFst<LogArc>(MapFst<StdArc, LogArc, StdToLogMapper >(bestpaths, toLogMapper)),
                                 DeterminizeFstOptions<LogArc>(CacheOptions(true, 0), delta));

}

} } // end namespace fstrain::util

#endif
