#ifndef propmap_hh_
#define propmap_hh_

#include <unordered_map>
#include <tuple>
#include <set>
#include <list>

#include <iostream>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

namespace propmap_aux {

  using hash_t = size_t;

  // Hashing code between ---- from boost
  // Reciprocal of the golden ratio helps spread entropy
  //     and handles duplicates.
  // See Mike Seymour in magic-numbers-in-boosthash-combine:
  //     http://stackoverflow.com/questions/4948780
  // Adopted from:
  //     http://stackoverflow.com/questions/7110301

  // ----------------------------------------------------------------

  template<typename P>
  using hashed_tmpl = std::tuple<hash_t,const P*>;

  template<typename... P>
  using key_tmpl = std::tuple<const hashed_tmpl<P>* ...>;

  inline void hash_combine(hash_t& seed, hash_t hash) noexcept {
    seed ^= hash + 0x9e3779b9 + (seed<<6) + (seed>>2);
  }

  template <typename Key, size_t I = std::tuple_size<Key>::value - 1>
  struct key_hash_impl {
    static inline void apply(hash_t& seed, const Key& key) noexcept {
      hash_combine(seed, std::get<0>(*std::get<I>(key)) );
      key_hash_impl<Key, I-1>::apply(seed, key);
    }
  };

  template<typename Key>
  struct key_hash_impl<Key,0> {
    static inline void apply(hash_t& seed, const Key& key) noexcept {
      hash_combine(seed, std::get<0>(*std::get<0>(key)) );
    }
  };

  template<typename Key> struct key_hash;
  template<typename... P>
  struct key_hash<key_tmpl<P...>> {
    hash_t operator()(const key_tmpl<P...>& key) const noexcept {
      hash_t seed = 0;
      key_hash_impl<key_tmpl<P...>>::apply(seed, key);
      return seed;
    }
  };

  // ----------------------------------------------------------------

  template<typename P>
  struct hashed_cmp {
    bool operator() (const hashed_tmpl<P>& x,
                     const hashed_tmpl<P>& y) const noexcept {
      hash_t hash_x = std::get<0>(x),
             hash_y = std::get<0>(y);
      if ( hash_x < hash_y ) return true;
      else if ( hash_x > hash_y ) return false;
      else {
        if ( *std::get<1>(x) < *std::get<1>(y) ) return true;
        else return false;
      }
    }
  };

  // template<typename T>
  // struct sort_prt_by_ref_less {
  //   bool operator() (const T* x, const T* y) const noexcept {
  //     return (*x) < (*y);
  //   }
  // };
  //
  // template<typename T, typename Compare>
  // struct sort_prt_by_ref {
  //   Compare& comp;
  //   sort_prt_by_ref(Compare& comp): comp(comp) { }
  //   bool operator() (const T* x, const T* y) const noexcept {
  //     return comp(*x,*y);
  //   }
  // };

} // END namespace propmap_aux

// ******************************************************************

template<typename Mapped, typename... Props>
class propmap {
public:

  using hash_t = propmap_aux::hash_t;

  template<size_t I>
  using prop_t = typename std::tuple_element<I, std::tuple<Props...>>::type;

  template<typename P>
  using set_tmpl = std::set<propmap_aux::hashed_tmpl<P>,
                            propmap_aux::hashed_cmp<P>>;

  template<size_t I>
  using hashed_ptr_t = const propmap_aux::hashed_tmpl<prop_t<I>>*;

  using key_t = propmap_aux::key_tmpl<Props...>;

  template<size_t I>
  using set_t = set_tmpl<prop_t<I>>;

  template<typename P>
  using list_tmpl = std::list<P>;

  template<size_t I>
  using list_t = list_tmpl<prop_t<I>>;

private:
  std::unordered_map<key_t,Mapped,propmap_aux::key_hash<key_t>> _map;
  std::tuple<set_tmpl<Props> ...> _sets;
  std::tuple<list_tmpl<Props> ...> _lists;

  template<size_t I>
  inline hashed_ptr_t<I> insert_prop(const prop_t<I>& p) {
    hash_t hash = std::hash<prop_t<I>>()(p);

    auto& _set = std::get<I>(_sets);
    auto it = _set.find( std::forward_as_tuple(hash,&p) );

    if (it==_set.end()) {
      std::get<I>(_lists).push_back(p);
      return &(*_set.emplace( hash, &std::get<I>(_lists).back() ).first);
    } else return &(*it);
  }

  template<typename P>
  inline void add_prop(key_t& key, const P& p) {
    std::get<sizeof...(Props)-1>(key) = insert_prop<sizeof...(Props)-1>(p);
  }

  template<typename P, typename... PP>
  inline void add_prop(key_t& key, const P& p, const PP&... pp) {
    std::get<sizeof...(Props)-sizeof...(PP)-1>(key) =
      insert_prop<sizeof...(Props)-sizeof...(PP)-1>(p);
    add_prop(key, pp...);
  }

  template<typename P>
  inline bool find_in_sets(key_t& key, const P& p) const noexcept {
    auto& _set = std::get<sizeof...(Props)-1>(_sets);

    auto it = _set.find(
      std::forward_as_tuple( std::hash<P>()(p), &p )
    );

    if (it!=_set.end()) {
      std::get<sizeof...(Props)-1>(key) = &(*it);
      return true;
    } else return false;
  }

  template<typename P, typename... PP>
  inline bool find_in_sets(key_t& key, const P& p, const PP&... pp) const noexcept {
    auto& _set = std::get<sizeof...(Props)-sizeof...(PP)-1>(_sets);

    auto it = _set.find(
      std::forward_as_tuple( std::hash<P>()(p), &p )
    );

    if (it!=_set.end()) {
      std::get<sizeof...(Props)-sizeof...(PP)-1>(key) = &(*it);
      return find_in_sets(key, pp...);
    } else return false;
  }

public:
  void insert(const Mapped& x, const Props&... props) {
    key_t key;
    add_prop(key, props...);
    _map.emplace( key, x );
  }

  template<size_t I> const list_t<I>& prop() const noexcept {
    return std::get<I>(_lists);
  }

  bool get(Mapped& x, const Props&... props) const noexcept {
    key_t key;
    if (!find_in_sets(key, props...)) return false;
    auto it = _map.find(key);
    if (it!=_map.end()) {
      x = it->second;
      return true;
    } else return false;
  }

  bool get(Mapped const*& x, const Props&... props) const noexcept {
    key_t key;
    if (!find_in_sets(key, props...)) return false;
    auto it = _map.find(key);
    if (it!=_map.end()) {
      x = &(it->second);
      return true;
    } else return false;
  }

  bool get(Mapped*& x, const Props&... props) noexcept {
    key_t key;
    if (!find_in_sets(key, props...)) return false;
    auto it = _map.find(key);
    if (it!=_map.end()) {
      x = &(it->second);
      return true;
    } else return false;
  }

  // want to return vectors of vectors
  //template<size_t... I>
  //void optimize() {
  //  static_assert(sizeof...(I)==sizeof...(Props));
  //}

  template<size_t I> void sort() noexcept {
    std::get<I>(_lists).sort();
  }
  template<size_t I, class Compare> void sort(Compare comp) noexcept {
    std::get<I>(_lists).sort(comp);
  }
};

#endif
