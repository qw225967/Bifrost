#pragma once

// The type of the iterator given by begin(c) (possibly std::begin(c)).
// ContainerIter<const vector<T>> gives vector<T>::const_iterator,
// while ContainerIter<vector<T>> gives vector<T>::iterator.
template <typename C>
using ContainerIter = decltype(begin(std::declval<C&>()));

template <typename C>
ContainerIter<C> c_begin(C& c) { return begin(c); }

template <typename C>
ContainerIter<C> c_end(C& c) { return end(c); }

// c_any_of()
//
// Container-based version of the <algorithm> `std::any_of()` function to
// test if any element in a container fulfills a condition.
template <typename C, typename Pred>
bool c_any_of(const C& c, Pred&& pred) {
    return std::any_of(c_begin(c),
        c_end(c),
        std::forward<Pred>(pred));
}