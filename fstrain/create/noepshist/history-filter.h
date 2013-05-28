#ifndef FSTRAIN_CREATE_NOEPSHIST_HISTORY_FILTER_H
#define FSTRAIN_CREATE_NOEPSHIST_HISTORY_FILTER_H

#include <string>
#include <cmath>
#include <set>

namespace fstrain { namespace create { namespace noepshist {

class HistoryFilter {
 public:
  virtual ~HistoryFilter() {}
  virtual bool operator()(const std::string& hist) = 0;
}; // end class

class HistoryFilter_Length : public HistoryFilter {
  int maxlen_left_;
  int maxlen_right_;
  const char sep_char_;
 public:
  HistoryFilter_Length(int maxlen_left, int maxlen_right, const char sep_char)
      : maxlen_left_(maxlen_left), maxlen_right_(maxlen_right),
        sep_char_(sep_char)
  {}
  virtual ~HistoryFilter_Length() {}
  bool operator()(const std::string& hist) {
    typedef std::string::size_type stype;
    stype sep_pos = hist.find(sep_char_);
    if (sep_pos == std::string::npos) {
      FSTR_CREATE_EXCEPTION("HistoryFilter_Length: No separator '"<< sep_char_
                            <<"' in " << hist);
    }
    stype left_len = sep_pos;
    stype right_len = hist.length() - left_len - 1;
    bool is_okay = left_len <= maxlen_left_
        && right_len <= maxlen_right_;
    return is_okay;
  }
}; // end class

/**
 * @brief Good for filtering features: E.g., fire only features where
 * one side is up to 1 longer than the other.
 */
class HistoryFilter_LengthDiff : public HistoryFilter {
  int maxdiff_;
  const char sep_char_;
 public:
  HistoryFilter_LengthDiff(int maxdiff, const char sep_char)
      : maxdiff_(maxdiff), sep_char_(sep_char)
  {}
  virtual ~HistoryFilter_LengthDiff() {}
  bool operator()(const std::string& hist) {
    typedef std::string::size_type stype;
    stype sep_pos = hist.find(sep_char_);
    if (sep_pos == std::string::npos) {
      FSTR_CREATE_EXCEPTION("HistoryFilter_LengthDiff: No separator '"<< sep_char_
                            <<"' in " << hist);
    }
    stype left_len = sep_pos;
    stype right_len = hist.length() - left_len - 1;
    bool is_okay = std::abs(left_len - right_len + 0.0) <= maxdiff_;
    return is_okay;
  }
}; // end class

class ObservedHistoriesFilter : public HistoryFilter {
  const std::set<std::string>& allowed_hists_;
 public:
  ObservedHistoriesFilter(const std::set<std::string>& hists)
      : allowed_hists_(hists) {}
  virtual ~ObservedHistoriesFilter() {}
  bool operator()(const std::string& hist) {
    return hist == "|" ||
        allowed_hists_.find(hist) != allowed_hists_.end();
  }
};


} } } // end namespaces

#endif
