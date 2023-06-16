/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/16 10:55 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : sequence_manager.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_SEQUENCE_MANAGER_H
#define WORKER_SEQUENCE_MANAGER_H

#include <limits>  // std::numeric_limits
#include <set>

#include "common.h"

namespace bifrost {
template <typename T>
class SeqManager {
 public:
  static constexpr T MaxValue = std::numeric_limits<T>::max();

 public:
  struct SeqLowerThan {
    bool operator()(const T lhs, const T rhs) const;
  };

  struct SeqHigherThan {
    bool operator()(const T lhs, const T rhs) const;
  };

 private:
  static const SeqLowerThan isSeqLowerThan;
  static const SeqHigherThan isSeqHigherThan;

 public:
  static bool IsSeqLowerThan(const T lhs, const T rhs);
  static bool IsSeqHigherThan(const T lhs, const T rhs);

 public:
  SeqManager() = default;

 public:
  void Sync(T input);
  void Drop(T input);
  void Offset(T offset);
  bool Input(const T input, T& output);
  T GetMaxInput() const;
  T GetMaxOutput() const;

 private:
  T base{0};
  T maxOutput{0};
  T maxInput{0};
  std::set<T, SeqLowerThan> dropped;
};
}  // namespace bifrost

#endif  // WORKER_SEQUENCE_MANAGER_H
