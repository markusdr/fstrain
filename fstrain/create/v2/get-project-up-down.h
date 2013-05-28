#ifndef FSTRAIN_CREATE_V2_GET_PROJECT_UP_DOWN_H
#define FSTRAIN_CREATE_V2_GET_PROJECT_UP_DOWN_H

#include "fst/symbol-table.h"
#include "fst/fst.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fstrain/create/debug.h"

namespace fstrain { namespace create {

namespace nsGetProjUpDownUtil {

template<class Arc>
void InitHookMachine(fst::MutableFst<Arc>* fst) {
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  StateId s = fst->AddState();
  fst->SetStart(s);
  fst->SetFinal(s, Weight::One());
}

} // end namespace

/**
 * @brief Constructs two machines: One that projects up (from a to
 * a|x, a|y, ...) and one that projects down (from a|x to x, a|y, to
 * y, ...)
 */
template<class Arc>
void GetProjUpDown(
    const fst::SymbolTable& input_syms,
    const fst::SymbolTable& output_syms,
    const fst::SymbolTable& align_syms,
    const char sep_char,
    fst::MutableFst<Arc>* proj_up,
    fst::MutableFst<Arc>* proj_down) {
  using namespace nsGetProjUpDownUtil;
  using namespace fst;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  InitHookMachine(proj_up);
  InitHookMachine(proj_down);
  StateId proj_up_s = proj_up->Start();
  StateId proj_down_s = proj_down->Start();
  for(SymbolTableIterator ait(align_syms); !ait.Done(); ait.Next()){
    const std::string asym_string = ait.Symbol();
    // examples for special syms: eps, START, END, phi, ...
    bool is_special_symbol = asym_string.find(sep_char) == std::string::npos;
    if(!is_special_symbol){
      std::stringstream asym(asym_string);
      std::string in, out, tmp;
      std::getline(asym, in, sep_char);
      std::getline(asym, out, sep_char);
      if(std::getline(asym, tmp)) { // no more
        FSTR_CREATE_EXCEPTION("Wrong alignment symbol format: " << asym.str());
      }
      if(in == "-"){
        in = "eps";
      }
      if(out == "-"){
        out = "eps";
      }
      int64 isym = input_syms.Find(in);
      if(isym == kNoLabel){
        FSTR_CREATE_EXCEPTION("Could not find input symbol '"<<in<<"' in input_syms");
      }
      proj_up->AddArc(proj_up_s,
                      Arc(isym, ait.Value(), Weight::One(), proj_up_s));

      int64 osym = output_syms.Find(out);
      if(osym == kNoLabel){
        FSTR_CREATE_EXCEPTION("Could not find output symbol '"<<out<<"' in output_syms");
      }
      proj_down->AddArc(proj_down_s,
                        Arc(ait.Value(), osym, Weight::One(), proj_down_s));
    }
  }
}
} } // end namespaces


#endif
