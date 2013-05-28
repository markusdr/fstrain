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
#include <fstream>
#include "fst/symbol-table.h"
#include "fst/fst.h"
#include "fstrain/train/obj-func-r-interface.h"
#include "fstrain/train/obj-func-fst-condother-lenmatch.h"
#include "fstrain/util/get-vector-fst.h"

extern "C" {

  void SetOtherVariable(char** list_file,
                        char** other_to_in_fst,
                        char** other_to_out_fst,
                        char** other_symbols) {
    std::cerr << "# Setting other variable" << std::endl;

    std::ifstream list_file_in(*list_file);
    std::vector<std::string>* list = new std::vector<std::string>();
    while (!list_file_in.eof()) {
      std::string line;
      std::getline(list_file_in, line);
      std::cerr << line << std::endl;
      list->push_back(line);
    }

    using fstrain::util::GetVectorFst;
    std::string other_to_in_fst_str(*other_to_in_fst);
    fst::Fst<fst::LogArc>* in_fst = GetVectorFst<fst::LogArc>(other_to_in_fst_str);

    std::string other_to_out_fst_str(*other_to_out_fst);
    fst::Fst<fst::LogArc>* out_fst = GetVectorFst<fst::LogArc>(other_to_out_fst_str);

    fst::SymbolTable* syms = fst::SymbolTable::ReadText(*other_symbols);

    using fstrain::train::ObjectiveFunctionFstCondotherLenmatch;
    ObjectiveFunctionFstCondotherLenmatch* obj_cast =
        dynamic_cast<ObjectiveFunctionFstCondotherLenmatch*>(obj);
    obj_cast->SetOtherVariable(list, in_fst, out_fst, syms);
  }

  void SetKbest(int* kbest) {
    std::cerr << "# Setting kbest " << *kbest << std::endl;
    using fstrain::train::ObjectiveFunctionFstCondotherLenmatch;
    ObjectiveFunctionFstCondotherLenmatch* obj_cast =
        dynamic_cast<ObjectiveFunctionFstCondotherLenmatch*>(obj);
    obj_cast->SetKbest(*kbest);
  }

}
