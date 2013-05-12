#ifndef FSTRAIN_CREATE_V2_CREATE_LATTICE_H
#define FSTRAIN_CREATE_V2_CREATE_LATTICE_H

#include "fst/compose.h"
#include "fst/fst.h"
#include "fst/arcsort.h"
#include "fst/project.h"
#include "fst/mutable-fst.h"

namespace fstrain { namespace create {

/**
 * @brief Creates lattice from input and output FSTs. For example,
 * input is just the line FSA "a b c", output is "x y", and all
 * possible alignments are created.
 */
template<class Arc>
void CreateLattice(
    const fst::Fst<Arc>& input,
    const fst::Fst<Arc>& output,
    const fst::Fst<Arc>& proj_up,
    const fst::Fst<Arc>& proj_down,
    fst::MutableFst<Arc>* result) {
  using namespace fst;
  ComposeFst<Arc> in(input, proj_up);
  ComposeFst<Arc> out(proj_down, output);
  typedef OLabelCompare<Arc> OCmp;
  Compose(
      ArcSortFst<Arc,OCmp>(ProjectFst<Arc>(in, PROJECT_OUTPUT), OCmp()),
      ProjectFst<Arc>(out, PROJECT_INPUT),
      result);
}

} } // end namespaces

#endif
