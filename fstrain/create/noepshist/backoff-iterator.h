#ifndef FSTRAIN_CREATE_NOEPSHIST_BACKOFF_ITERATOR_H
#define FSTRAIN_CREATE_NOEPSHIST_BACKOFF_ITERATOR_H

#include <set>
#include <queue>
#include <string>

namespace fstrain { namespace create { namespace noepshist {

/**
 * @brief Iterates over increasingly more general backoffs for given
 * window.
 */
class BackoffIterator {
  typedef std::pair<std::string,std::string> QueueEntry;
  std::string::size_type histlen_; // length of the left (right) history
  std::queue<QueueEntry> queue_;   // TODO: store pointers?
  std::set<QueueEntry> is_on_queue_;
  const char sep_char_;

  inline bool IsOnQueue(const QueueEntry& e) {
    return is_on_queue_.find(e) != is_on_queue_.end();
  }

  void PushOnQueue(const std::string& left, const std::string& right) {
    QueueEntry entry(left, right);
    if (!IsOnQueue(entry)) {
      queue_.push(entry);
      is_on_queue_.insert(entry);
    }
  }

 public:

  /**
   * @param str The window to back off from. Examples: "-b|-y", or
   * "ab|-y".
   */
  BackoffIterator(const std::string& str,
                  const char sep_char = '|')
      : sep_char_(sep_char)
  {
    std::stringstream ss(str);
    std::string left, right;
    std::getline(ss, left, sep_char_);
    std::getline(ss, right, sep_char_);
    PushOnQueue(left, right);
    histlen_ = left.length();
  }

  bool Done() const {
    return queue_.empty();
  }

  /**
   * @brief Returns the current backoff value; the first call returns
   * the original window, the last call returns the complete backoff,
   * where every character is removed.
   */
  std::string Value() const {
    const QueueEntry& s = queue_.front();
    std::stringstream ss;
    ss << s.first << sep_char_ << s.second;
    FSTR_CREATE_DBG_MSG(100, "Returning '"<<ss.str()<<"'" << std::endl);
    return ss.str();
  }

  void Next() {
    const QueueEntry& s = queue_.front();
    const std::string left = s.first;
    const std::string right = s.second;
    std::string left0 = left;
    if (left0.length() > 0) {
      left0 = left0.substr(1);
    }
    std::string right0 = right;
    if (right0.length() > 0) {
      right0 = right0.substr(1);
    }
    queue_.pop();
    if (left0.length() + right.length() >=
       left.length() + right0.length()) {
      PushOnQueue(left0, right);
      PushOnQueue(left, right0);
    }
    else {
      PushOnQueue(left, right0);
      PushOnQueue(left0, right);
    }
  }
}; // end class

} } } // end namespaces

#endif
