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

#include "fst/fst.h"
#include "fst/map.h"
#include "fst/arcsort.h"
#include "fst/mutable-fst.h"
#include "fst/symbol-table.h"
#include "fst/vector-fst.h"

#include "fstrain/util/memory-info.h"

#include "fstrain/core/expectation-arc.h"

#include "fstrain/create/create-ngram-fst-from-best-align.h"
#include "fstrain/create/create-ngram-fst-from-best-align-r2191.h" // old version
#include "fstrain/create/create-ngram-fst.h"
#include "fstrain/create/features/extract-features.h"
#include "fstrain/create/features/feature-set.h"
#include "fstrain/create/noepshist/noepshist-create-scoring-fst.h"
#include "fstrain/create/backoff-symbols.h"
#include "fstrain/create/backoff-util.h" // ParseBackoffDescription
#include "fstrain/create/prune-fct.h"

#include "fstrain/glue/debug.h"

#include "fstrain/train/obj-func-fst-factory.h"
#include "fstrain/train/obj-func-fst-conditional.h" // TEST

#include "fstrain/util/get-vector-fst.h"
// #include "fstrain/util/load-library.h"
#include "fstrain/util/data.h"
#include "fstrain/util/misc.h"
#include "fstrain/util/options.h"
#include "fstrain/util/symbols-mapper-in-out-align.h"

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

#include <stdexcept>
#include <limits>
#include <string>
#include <cmath>

// for experimental scoring FSA using phi etc.:
//#include "fstrain/create/v2/create-scoring-fsa-from-data.h"
//#include "fstrain/create/v2/concat-start-and-end-labels.h"
//#include "fstrain/util/compose-fcts.h"
//#include "fstrain/util/compose-fcts-experimental.h"

// Include other parts of C interface:
#include "fstrain/train/obj-func-fst-r-interface.cc"
#include "fstrain/create/r-interface.h"

void InitFeatureNamesTable() {
  feature_names =
      boost::shared_ptr<fst::SymbolTable>(new fst::SymbolTable("feature-names"));
}

void PopulateFeatureNamesTable(const std::string& filename) {
  if(!feature_names){
    // exception bad if called from R?
    FSTR_GLUE_EXCEPTION("Please call InitFeatureNamesTable first");
  }
  std::cerr << "InitFeatureNamesTable("<<filename<<")" << std::endl;
  if(filename.length()){
    std::ifstream namesfile(filename.c_str());
    std::string featname;
    while(std::getline(namesfile, featname)){
      // int64 id =
      feature_names->AddSymbol(featname);
      // std::cerr << "featid("<<featname<<") = " << id << std::endl;
    }
  }
}

template<class Arc>
fst::Fst<Arc>* ReadFst(const std::string& name) {
  fst::Fst<Arc>* ret = fstrain::util::GetVectorFst<Arc>(name);
  if(ret == NULL){
    FSTR_GLUE_EXCEPTION("Could not find " << name);
  }
  return ret;
}

template<class Arc>
fst::Fst<Arc>* GetSigmaFst(const typename Arc::Label sigma_label) {
  using namespace fst;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Weight Weight;
  MutableFst<Arc>* ret = new VectorFst<Arc>();
  StateId s = ret->AddState();
  ret->SetStart(s);
  ret->SetFinal(s, Weight::One());
  ret->AddArc(s, Arc(sigma_label, sigma_label, Weight::One(), s));

  //ret->SetProperties(ret->Properties() | kNotAcceptor);
  //ret->SetProperties(ret->Properties() & ~kAcceptor);
  // workaround so that it has transducer properties:
  StateId t = ret->AddState();
  ret->AddArc(t, Arc(1, 2, Weight::One(), t));

  return ret;
}


extern "C" {

  using namespace fst;

  bool use_noepshist_topology = false;
  bool use_old_fst_creation_code = false;

  int num_threads = 1;

  // e.g. "vc,tlm/collapsed,id), or "vc/la,tlm/la",
  // where "vc" is a fct that backs off single
  // symbols, and "la" is a fct that extracts
  // features from a (possibly backed-off) ngram,
  // e.g. from "V/C V/C"
  std::string backoff_names_str;

  void UseTopology_noepshist() {
    use_noepshist_topology = true;
  }

  void UseOldFstCreationCode() {
    use_old_fst_creation_code = true;
  }

  void SetBackoffFunctions(char** names) {
    backoff_names_str = std::string(*names);
  }

  void SetMemoryLimitInGb(double* limit_gb) {
    if(*limit_gb > 0.0) {
      std::cerr << "# Setting memory limit " << *limit_gb << " GB" << std::endl;
      fstrain::util::options["memory-limit-bytes"] = *limit_gb * pow(1024.0, 3.0); // GB -> B
    }
  }

  // options for CheckConvergence of FSTs
  void SetEigenvalueMaxiter(int* maxiter) {
    fstrain::util::options["eigenvalue-maxiter"] = *maxiter;
  }
  void SetEigenvalueConvergenceTol(double* tol) {
    fstrain::util::options["eigenvalue-convergence-tol"] = *tol;
  }

  void SetSymCondProbThreshold(double* threshold) {
    fstrain::util::options["sym-cond-prob-threshold"] = *threshold;
  }

  void SetUseMatrixDistance() {
    std::cerr << "# Will use matrix distance method on timeout FSTs";
    fstrain::util::options["use-matrix-distance"] = true;
  }

  /**
   * @brief Constructs objective function object after creating an
   * n-gram scoring FST.
   *
   * @param function_type The function type, see
   * objective-function-fst-factory.h
   */
  void Init_Ngram(int* function_type,
                  int* ngram_order,
                  int* max_insertions,
                  char** data_file,
                  char** isymbols_file, char** osymbols_file,
                  char** extract_features_libname,
                  char** features_init_filestem,
                  double* variance) {
    using namespace fstrain;
    using boost::shared_ptr;
    std::string isymbols_file_str(*isymbols_file);
    std::string osymbols_file_str(*osymbols_file);
    std::string features_init_filestem_str(*features_init_filestem);
    MutableFst<MDExpectationArc>* fst = new VectorFst<MDExpectationArc>();
    InitFeatureNamesTable();
    if(features_init_filestem_str.length()) {
      PopulateFeatureNamesTable(features_init_filestem_str);
    }
    else {
      feature_names->AddSymbol("*LENGTH_IN*");
      feature_names->AddSymbol("*LENGTH_OUT*"); // to give them Ids 0 and 1
    }
    // e.g. "simple"
    const std::string extract_features_libname_str(*extract_features_libname);
    create::features::ExtractFeaturesFctPlugin extract_features_fct(extract_features_libname_str);
    create::CreateNgramFst(*ngram_order, *max_insertions,
                           isymbols_file_str, osymbols_file_str,
                           extract_features_fct,
                           feature_names.get(), fst);

    std::string data_file_str(*data_file);
    train::ObjectiveFunctionType type =
        static_cast<train::ObjectiveFunctionType>(*function_type);
    std::cerr << "Obj. function type: " << type << std::endl;

    // will delete the fst
    obj =  train::CreateObjectiveFunctionFst(type,
                                             fst,
                                             data_file_str,
                                             isymbols_file_str,
                                             osymbols_file_str,
                                             *variance);
  }

  /**
   * @brief Aligns some text under given alignment FST, then
   * constructs ngram scoring machine using ngrams from aligned text,
   * then constructs objective function.
   *
   * @param function_type The function type, see
   * objective-function-fst-factory.h
   */
  void Init_FromAlignment(int* function_type,
                          int* ngram_order,
                          int* max_insertions,
                          int* num_conjugations,
                          int* num_change_regions,
                          char** align_fst,
                          int* do_prune,
                          double* prune_weight_threshold,
                          double* prune_state_factor,
                          char** data_file,
                          char** isymbols_file, char** osymbols_file,
                          char** extract_features_libname,
                          char** features_init_filestem,
                          double* variance) {
    using namespace fstrain;
    using namespace create;
    std::string data_file_str(*data_file);
    std::string align_fst_file_str(*align_fst);
    std::string isymbols_file_str(*isymbols_file);
    std::string osymbols_file_str(*osymbols_file);
    std::string features_init_filestem_str(*features_init_filestem);

    Fst<StdArc>* align_fst_obj = NULL;
    if(align_fst_file_str == "simple"){
      std::cerr << "Using simple alignments with no epsilons" << std::endl;
      const StdArc::Label kSigmaLabel = -2;
      util::options["sigma_label"] = kSigmaLabel;
      align_fst_obj = GetSigmaFst<StdArc>(kSigmaLabel);
    }
    else {
      align_fst_obj = ReadFst<StdArc>(align_fst_file_str);
    }

    std::cerr << "# Using " << util::MemoryInfo::instance().getSizeInMB()
              << " MB." << std::endl;

    InitFeatureNamesTable();
    if(features_init_filestem_str.length()) {
      PopulateFeatureNamesTable(features_init_filestem_str);
      std::cerr << "# Using " << util::MemoryInfo::instance().getSizeInMB()
                << " MB after storing feature names." << std::endl;
    }
    else {
      feature_names->AddSymbol("*LENGTH_IN*");
      feature_names->AddSymbol("*LENGTH_OUT*"); // to give them Ids 0 and 1
    }
    MutableFst<MDExpectationArc>* fst = new VectorFst<MDExpectationArc>();
    util::ThrowExceptionIfFileNotFound(isymbols_file_str);
    util::ThrowExceptionIfFileNotFound(osymbols_file_str);
    const SymbolTable* isymbols = SymbolTable::ReadText(isymbols_file_str);
    const SymbolTable* osymbols = SymbolTable::ReadText(osymbols_file_str);

    const bool new_scoring_fsa = false; // experimental new FSA that
    // uses phi and needs projup
    // etc. during training
    if(use_noepshist_topology) {
      std::cerr << "WARNING: Using experimental noepshist scoring machine" << std::endl;
      noepshist::CreateNgramFstFromBestAlign(*ngram_order,
                                             *max_insertions, *num_conjugations, *num_change_regions,
                                             *align_fst_obj,
                                             data_file_str,
                                             isymbols, osymbols,
                                             *variance,
                                             feature_names.get(),
                                             fst);
    }

    //    else if(new_scoring_fsa) {
    //      std::cerr << "WARNING: Using new experimental proj up/down scoring FSA"
    //                << std::endl;
    //      using namespace fstrain::util;
    //      typedef MDExpectationArc Arc;
    //      const char sep_char = '|';
    //      bool add_identity_chars = true; // always add a|a, b|b, ...
    //      MutableFst<Arc>* proj_up = new VectorFst<Arc>();
    //      MutableFst<Arc>* proj_down = new VectorFst<Arc>();
    //      SymbolTable* pruned_align_syms = new SymbolTable("pruned-align-syms"); // TODO: ok to create on stack?
    //      const std::string extract_features_libname_str(*extract_features_libname); // e.g. "simple"
    //      features::ExtractFeaturesFctPlugin extract_features_fct(
    //          extract_features_libname_str);
    //      Data* data = new Data(data_file_str);
    //
    //      fstrain::CreateScoringFsaFromData(*data, *isymbols, *osymbols,
    //                                                *align_fst_obj,
    //                                                *ngram_order,
    //                                                extract_features_fct,
    //                                                sep_char,
    //                                                pruned_align_syms,
    //                                                add_identity_chars,
    //                                                feature_names.get(),
    //                                                proj_up, proj_down,
    //                                                fst);
    //      const int64 kPhiLabel = pruned_align_syms->Find("phi");
    //      if(kPhiLabel == kNoLabel){
    //        throw std::runtime_error("phi not found");       // TODO: prob. not ok to throw?
    //      }
    //      MutableFst<StdArc>* prune_fst = new VectorFst<StdArc>();
    //      typedef fstrain::util::SymbolsMapper_InOutToAlign<StdArc> AlignMapper;
    //      typedef MapFst<StdArc,StdArc,AlignMapper> AlignMapFst;
    //      *prune_fst = AlignMapFst(*align_fst_obj, AlignMapper(*isymbols, *osymbols,
    //                                                           *pruned_align_syms));
    //      const int64 kStartLabel = pruned_align_syms->Find("START");
    //      const int64 kEndLabel = pruned_align_syms->Find("END");
    //      ConcatStartAndEndLabels(kStartLabel, kEndLabel, false, false,
    //                                      prune_fst); // START:START
    //      ArcSort(prune_fst, ILabelCompare<StdArc>());
    //      TropicalWeight weight_threshold(-log(0.1)); // TEST
    //
    //      // compose_input is a fct obj which composes (input o proj_up) * model), where model can have phi syms
    //      typedef boost::shared_ptr<const Fst<Arc> > ConstFstPtr;
    //      typedef ComposePhiRightFct<Arc> MyPhiComposeRightFct;
    //      typedef ProjUpComposeFct<Arc, MyPhiComposeRightFct> MyProjUpComposeFct;
    //      // typedef ProjUpPruneComposeFct<Arc, MyPhiComposeRightFct> MyProjUpComposeFct; // prune
    //      // typedef ProjUpKbestComposeFct<Arc, MyPhiComposeRightFct> MyProjUpComposeFct; // kbest
    //      MyPhiComposeRightFct phi_compose_right_fct(kPhiLabel);
    //      ComposeFct<Arc>* compose_input =
    //          new MyProjUpComposeFct(ConstFstPtr(proj_up), phi_compose_right_fct);
    //      // new MyProjUpComposeFct(ConstFstPtr(proj_up), *prune_fst, weight_threshold, phi_compose_right_fct); // prune
    //      // new MyProjUpComposeFct(ConstFstPtr(proj_up), *prune_fst, 50, phi_compose_right_fct); // kbest
    //      // dynamic_cast<MyProjUpComposeFct*>(compose_input)->SetAlignSymbols(pruned_align_syms);
    //
    //      // compose_output is a fct obj which composes model o (proj_down o output), where model can have phi syms
    //      typedef ComposePhiLeftFct<Arc> MyPhiComposeLeftFct;
    //      typedef ProjUpComposeFct<Arc, MyPhiComposeLeftFct> MyProjDownComposeFct;
    //      MyPhiComposeLeftFct phi_compose_left_fct(kPhiLabel);
    //      ComposeFct<Arc>* compose_output =
    //          new MyProjDownComposeFct(ConstFstPtr(proj_down), phi_compose_left_fct);
    //
    //      util::options["eigenvalue-maxiter-checkvalue"] = true;
    //      obj = new fstrain::train::ObjectiveFunctionFstConditional(fst,
    //                                                                data,
    //                                                                isymbols, osymbols,
    //                                                                *variance,
    //                                                                compose_input, compose_output);
    //      delete align_fst_obj;
    //      //delete isymbols;
    //      //delete osymbols;
    //
    //      return; // TEST
    //    }

    else {
      PruneFct* prune_fct = NULL;
      if(*do_prune){
        prune_fct = new DefaultPruneFct(fst::StdArc::Weight(-log(*prune_weight_threshold)),
                                        *prune_state_factor);
      }

      std::vector<boost::tuple<BackoffSymsFct::Ptr, features::ExtractFeaturesFct::Ptr, std::size_t> > backoff_fcts;
      ParseBackoffDescription(backoff_names_str, *ngram_order, &backoff_fcts);

      const std::string extract_features_libname_str(*extract_features_libname); // e.g. "simple"
      features::ExtractFeaturesFctPlugin extract_features_fct(extract_features_libname_str);

      if (use_old_fst_creation_code) {
        CreateNgramFstFromBestAlign_r2191(*ngram_order,
                                          *max_insertions, *num_conjugations,
                                          *num_change_regions,
                                          *align_fst_obj,
                                          prune_fct,
                                          data_file_str,
                                          isymbols, osymbols,
                                          extract_features_fct,
                                          backoff_fcts,
                                          feature_names.get(),
                                          fst);
      }
      else {
        CreateNgramFstFromBestAlign(*ngram_order,
                                    *max_insertions, *num_conjugations,
                                    *num_change_regions,
                                    *align_fst_obj,
                                    prune_fct,
                                    data_file_str,
                                    isymbols, osymbols,
                                    extract_features_fct,
                                    backoff_fcts,
                                    feature_names.get(),
                                    fst);

      }
      delete prune_fct;
    }

    delete align_fst_obj;
    delete isymbols;
    delete osymbols;

    train::ObjectiveFunctionType type = static_cast<train::ObjectiveFunctionType>(*function_type);
    std::cerr << "Obj. function type: " << type << std::endl;

    // This option actually is dangerous:
    // util::options["eigenvalue-maxiter-checkvalue"] = true; It means
    // that if eigenvalue test has reached max iterations we will not
    // automatically return divergence.

    std::cerr << "# Using " << util::MemoryInfo::instance().getSizeInMB()
              << " MB before creating obj function." << std::endl;

    // will delete the fst
    obj = train::CreateObjectiveFunctionFst(type,
                                            fst,
                                            data_file_str,
                                            isymbols_file_str,
                                            osymbols_file_str,
                                            *variance);
    std::cerr << "# Using " << util::MemoryInfo::instance().getSizeInMB()
              << " MB after creating obj function." << std::endl;
  }


} // end extern "C"
