#ifndef GENSEQ_HPP
#define GENSEQ_HPP

template<int...>
struct seq {};
template<int N, int... S>
struct genseq : genseq<N-1, N-1, S...> {};
template<int... S>
struct genseq<0, S...> {
    typedef seq<S...> type;
};

#endif
