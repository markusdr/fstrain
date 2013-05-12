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
#ifndef GET_VECTOR_FST_H
#define GET_VECTOR_FST_H

#include <iostream>
#include <fstream>
#include "fst/fst.h"
#include "fst/map.h"
#include "fstrain/util/debug.h"
#include "fstrain/core/expectation-arc.h"

namespace fstrain { namespace util {

/**
 * @brief Reads FST from, converting it to the Arc (specified as
 * template) if necessary.
 */
template<class Arc>
  fst::MutableFst<Arc>* GetVectorFst(const std::string& filename) {
  fst::MutableFst<Arc>* result = NULL;
  std::istream* strm = new std::ifstream(filename.c_str(), 
					 std::ifstream::in | std::ifstream::binary);
  if (!*strm) {
    FSTR_UTIL_EXCEPTION("Could not read " << filename);
  }
  fst::FstHeader hdr;
  if(!hdr.Read(*strm, filename)) {
    FSTR_UTIL_EXCEPTION("Could not read " << filename << " (format error?)");
  }
  const std::string arc_type = hdr.ArcType();
  if(arc_type == Arc::Type()){
    result = fst::VectorFst<Arc>::Read(filename);
  }
  else {
    result = new fst::VectorFst<Arc>();    
    if(arc_type == "standard") {
      FSTR_UTIL_DBG_MSG(10, "Converting from StdArc" << std::endl);
      typedef fst::WeightConvertMapper<fst::StdArc, Arc> Mapper;
      fst::MutableFst<fst::StdArc>* tmp = fst::VectorFst<fst::StdArc>::Read(filename);    
      *result = fst::MapFst<fst::StdArc, Arc, Mapper>(*tmp, Mapper());
      delete tmp;
    }
    else if(arc_type == "log") {
      FSTR_UTIL_DBG_MSG(10, "Converting from LogArc" << std::endl);
      typedef fst::WeightConvertMapper<fst::LogArc, Arc> Mapper;
      fst::MutableFst<fst::LogArc>* tmp = fst::VectorFst<fst::LogArc>::Read(filename);    
      *result = fst::MapFst<fst::LogArc, Arc, Mapper>(*tmp, Mapper());
      delete tmp;
    }
    else if(arc_type == "expectation") {
      FSTR_UTIL_DBG_MSG(10, "Converting from fst::MDExpectationArc" << std::endl);
      typedef fst::WeightConvertMapper<fst::MDExpectationArc, Arc> Mapper;
      fst::MutableFst<fst::MDExpectationArc>* tmp = fst::VectorFst<fst::MDExpectationArc>::Read(filename);    
      *result = fst::MapFst<fst::MDExpectationArc, Arc, Mapper>(*tmp, Mapper());
      delete tmp;
    }
    else {
      FSTR_UTIL_EXCEPTION(filename << ": Unknown arc type");
    }    
  }
  delete strm;
  return result;
}

} } // end namespace fstrain::util


#endif
