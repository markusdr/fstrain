#ifndef FSTRAIN_CREATE_V2_GET_PRUNED_ALIGNMENT_SYMS_H
#define FSTRAIN_CREATE_V2_GET_PRUNED_ALIGNMENT_SYMS_H

#include <sstream>
#include "fst/symbol-table.h"
#include "fstrain/util/data.h"
#include "fstrain/util/align-strings.h"
#include "fstrain/util/options.h"
// #include "fstrain/create/get-alignment-symbols-fct.h"
#include "fstrain/create/debug.h"

namespace fstrain { namespace create {

void AddIdentityCharacters(const fst::SymbolTable& isyms,
                           const fst::SymbolTable& osyms,
                           fst::SymbolTable* align_syms){
  fst::SymbolTableIterator sit(isyms);
  sit.Next(); // ignore eps
  for(; !sit.Done(); sit.Next()){      
    if(osyms.Find(sit.Symbol()) != fst::kNoLabel){
      std::stringstream ss;
      ss << sit.Symbol() << "|" << sit.Symbol();
      align_syms->AddSymbol(ss.str());  
    }
  }
}

template<class OutputStream>
class StreamAndSymbolTableWriter {
  OutputStream* out_;
  fst::SymbolTable* source_syms_;
  fst::SymbolTable* target_syms_;
 public:
  StreamAndSymbolTableWriter(OutputStream* out, fst::SymbolTable* target_syms) 
      : out_(out), target_syms_(target_syms) {
    target_syms->AddSymbol("eps", 0); 
  }
  void SetSourceSyms(fst::SymbolTable* source_syms) {
    source_syms_ = source_syms;
  }
  // for std::endl:
  OutputStream& operator<<(OutputStream& (*f)(OutputStream&)){ 
    return f(*out_);
  }
  OutputStream& operator<<(const std::string& str) {
    *out_ << str;
    if(str != "" && str != " " && str != "\n"){
      if(source_syms_ == NULL){
        target_syms_ ->AddSymbol(str);
      }
      else {
        int64 label = source_syms_->Find(str);
        if(label == fst::kNoLabel){
          FSTR_CREATE_EXCEPTION("Could not find symbol '" << str 
                                << "' in source alignment symbols table");
        }
        target_syms_ ->AddSymbol(str, label);      
      }
    }
    return *out_;
  }
};

template<class Arc>
void GetPrunedAlignmentSyms(const util::Data& data, 
                            const fst::SymbolTable& isyms,
                            const fst::SymbolTable& osyms,
                            const fst::Fst<Arc>& align_fst,
                            fst::SymbolTable* result) {
  std::stringstream aligned_data;
  util::AlignStringsDefaultOutputStream<std::stringstream> writer(&aligned_data, result);
  // StreamAndSymbolTableWriter<std::stringstream> writer(&aligned_data, result);
  // writer.SetSourceSyms(all_align_syms);
  util::AlignStringsOptions opts;  
  opts.n_best_alignments = 1; // one-best
  if(fstrain::util::options.has("sigma_label")){
    opts.sigma_label = fstrain::util::options.get<int>("sigma_label");
  }
  util::AlignStrings(data, isyms, osyms, align_fst, &writer, opts);  
}

} } // end namespace

#endif
