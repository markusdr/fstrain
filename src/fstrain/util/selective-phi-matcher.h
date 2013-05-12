#ifndef FSTRAIN_UTIL_SELECTIVE_PHI_MATCHER_H
#define FSTRAIN_UTIL_SELECTIVE_PHI_MATCHER_H

namespace fst {

/**
 * @brief A phi matcher which has a specified set of symbols that will
 * match with phi. (If the set is empty the SelectivePhiMatcher will
 * work like a regular PhiMatcher.)
 *
 * Example: The set contains only (c|z, i|e), and at a given state we
 * have arcs with symbols a|x, c|z and phi. Now the symbols a|x, c|z
 * and i|e can match (the latter one through phi).
 */
template <class M>
class SelectivePhiMatcher {
 public:
  typedef typename M::FST FST;
  typedef typename M::Arc Arc;
  typedef typename Arc::StateId StateId;
  typedef typename Arc::Label Label;
  typedef typename Arc::Weight Weight;
  typedef std::set<Label> LabelSet;

  SelectivePhiMatcher(const FST &fst,
                      MatchType match_type,
                      Label phi_label = kNoLabel,
                      bool phi_loop = true,
                      bool rewrite_both = false,
                      M *matcher = 0)
      : matcher_(matcher ? matcher : new M(fst, match_type)),
        match_type_(match_type),
        phi_label_(phi_label),
        rewrite_both_(rewrite_both ? true : fst.Properties(kAcceptor, true)),
        state_(kNoStateId),
        phi_loop_(phi_loop) {
    if (match_type == MATCH_BOTH)
      LOG(FATAL) << "SelectivePhiMatcher: bad match type";
    if (phi_label == 0)
      LOG(FATAL) << "SelectivePhiMatcher: 0 cannot be used as phi_label";
  }

  SelectivePhiMatcher(const SelectivePhiMatcher<M> &matcher)
      : matcher_(new M(*matcher.matcher_)),
        match_type_(matcher.match_type_),
        phi_label_(matcher.phi_label_),
        rewrite_both_(matcher.rewrite_both_),
        state_(kNoStateId),
        phi_loop_(matcher.phi_loop_) {}

  ~SelectivePhiMatcher() {
    delete matcher_;
  }

  void SetPhiMatchingSymbols(typename LabelSet::const_iterator first,
                             typename LabelSet::const_iterator last) { 
    phi_match_syms_.insert(first, last);
  }

  MatchType Type(bool test) const { return matcher_->Type(test); }

  void SetState(StateId s) {
    matcher_->SetState(s);
    state_ = s;
    has_phi_ = phi_label_ != kNoLabel;
  }

  bool Find(Label match_label) {
    if (match_label == phi_label_ && phi_label_ != kNoLabel) {
      LOG(FATAL) << "SelectivePhiMatcher::Find: bad label (phi)";
    }
    matcher_->SetState(state_);
    phi_match_ = kNoLabel;
    phi_weight_ = Weight::One();
    if (!has_phi_ || match_label == 0 || match_label == kNoLabel)
      return matcher_->Find(match_label);
    StateId state = state_;
    while (!matcher_->Find(match_label)) {
      if (!matcher_->Find(phi_label_))
        return false;
      if (phi_loop_ && matcher_->Value().nextstate == state) {
        phi_match_ = match_label;
        return true;
      }
      if(!phi_match_syms_.empty() 
         && phi_match_syms_.find(match_label) == phi_match_syms_.end()) {
        return false;
      }
      phi_weight_ = Times(phi_weight_, matcher_->Value().weight);
      state = matcher_->Value().nextstate;
      matcher_->SetState(state);
    }
    return true;
  }

  bool Done() const { return matcher_->Done(); }

  const Arc& Value() const {
    if ((phi_match_ == kNoLabel) && (phi_weight_ == Weight::One())) {
      return matcher_->Value();
    } else {
      phi_arc_ = matcher_->Value();
      phi_arc_.weight = Times(phi_weight_, phi_arc_.weight);
      if (phi_match_ != kNoLabel) {
        if (rewrite_both_) {
          if (phi_arc_.ilabel == phi_label_)
            phi_arc_.ilabel = phi_match_;
          if (phi_arc_.olabel == phi_label_)
            phi_arc_.olabel = phi_match_;
        } else if (match_type_ == MATCH_INPUT) {
          phi_arc_.ilabel = phi_match_;
        } else {
          phi_arc_.olabel = phi_match_;
        }
      }
      return phi_arc_;
    }
  }

  void Next() { matcher_->Next(); }


  uint64 Properties(uint64 props) const {
    if (match_type_ == MATCH_NONE)
      return props;
    else if (rewrite_both_)
      return props & ~(kIDeterministic | kNonIDeterministic |
                       kODeterministic | kNonODeterministic | kString);
    else //
      return props & ~kString;
  }

 private:
  M *matcher_;
  MatchType match_type_;  // Type of match requested
  Label phi_label_;       // Label that represents the phi transition
  bool rewrite_both_;     // Rewrite both sides when both are 'phi_label_'
  bool has_phi_;          // Are there possibly phis at the current state?
  Label phi_match_;       // Current label that matches phi loop
  mutable Arc phi_arc_;   // Arc to return
  StateId state_;         // State where looking for matches
  Weight phi_weight_;     // Product of the weights of phi transitions taken
  bool phi_loop_;         // When true, phi self-loop are allowed and treated
  // as rho (required for Aho-Corasick)

  // Only symbols from this set will match with phi 
  LabelSet phi_match_syms_;

  void operator=(const SelectivePhiMatcher<M> &);  // disallow
};

} // end namespace

#endif
