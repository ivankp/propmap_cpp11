#ifndef propmap_hh_
#define propmap_hh_

#include <unordered_map>
#include <tuple>
#include <list>
#include <set>

#include <iostream>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

namespace ivanp {

template<typename Mapped, typename... Props>
class propmap {

  template<size_t I>
  using prop_t = typename std::tuple_element<I, std::tuple<Props...>>::type;

  // map defs and functions -----------------------------------------

  using key_t = std::tuple<const Props*...>;

  static inline void hash_combine(size_t& seed, size_t hash) noexcept {
    seed ^= hash + 0x9e3779b9 + (seed<<6) + (seed>>2);
  }

  template <size_t I, typename std::enable_if<I>::type * = nullptr>
  static inline void key_hash_impl(size_t& seed, const key_t& key) noexcept {
    hash_combine(seed, reinterpret_cast<uintptr_t>(std::get<I>(key)) );
    key_hash_impl<I-1>(seed, key);
  }

  template <size_t I, typename std::enable_if<!I>::type * = nullptr>
  static inline void key_hash_impl(size_t& seed, const key_t& key) noexcept {
    hash_combine(seed, reinterpret_cast<uintptr_t>(std::get<0>(key)) );
  }

  struct key_hash {
    size_t operator()(const key_t& key) const noexcept {
      size_t seed = 0;
      key_hash_impl<sizeof...(Props)-1>(seed, key);
      return seed;
    }
  };

  // set defs and functions -----------------------------------------

  template<typename P> struct set_hash {
    size_t operator()(const P* p) const noexcept {
      return reinterpret_cast<uintptr_t>(p);
    }
  };
  /*
  template<typename P> struct set_eq {
    bool operator()(const P* a, const P* b) const noexcept {
      std::cout << "*a["<<*a<<"] == *b["<<*b<<"]" << " = " << (*a == *b) << std::endl;
      return *a == *b;
    }
  };
  */
  template<typename P> struct set_less {
    bool operator()(const P* a, const P* b) const noexcept {
      return *a < *b;
    }
  };

  template<typename P>
  using set_tmpl = std::set<const P*, set_less<P>>;
  // using set_tmpl = std::unordered_set<const P*, set_hash<P>, set_eq<P>>;

  template<size_t I> using set_t = set_tmpl<prop_t<I>>;

  // list defs and functions ----------------------------------------

  template<typename P> using list_tmpl = std::list<P>;

  template<size_t I> using list_t = list_tmpl<prop_t<I>>;

  // member containers ----------------------------------------------

  std::unordered_map<key_t,Mapped,key_hash> map;
  std::tuple< set_tmpl<Props> ...> sets;
  std::tuple<list_tmpl<Props> ...> lists;

  // private functions ----------------------------------------------

  template<size_t I>
  inline const prop_t<I>* insert_single(const prop_t<I>& p) {
    auto& set = std::get<I>(sets);
    auto it = set.find(&p);

    if (it==set.end()) {
      std::get<I>(lists).push_back(p);
      it = set.insert(&std::get<I>(lists).back()).first;
    }
    return *it;
  }

  template<typename P>
  inline void insert_all(key_t& key, const P& p) {
    std::get<sizeof...(Props)-1>(key) = insert_single<sizeof...(Props)-1>(p);
  }

  template<typename P, typename... PP>
  inline void insert_all(key_t& key, const P& p, const PP&... pp) {
    std::get<sizeof...(Props)-sizeof...(PP)-1>(key) =
      insert_single<sizeof...(Props)-sizeof...(PP)-1>(p);
    insert_all(key, pp...);
  }

  template<size_t I>
  inline const prop_t<I>* find_in_single(const prop_t<I>& p) {
    const auto& set = std::get<I>(sets);
    const auto it = set.find(&p);

    if (it!=set.end()) return *it;
    else return nullptr;
  }

  template<typename P>
  inline bool find_in_sets(key_t& key, const P& p) const noexcept {
    const auto& set = std::get<sizeof...(Props)-1>(sets);
    const auto it = set.find(&p);

    if (it!=set.end()) {
      std::get<sizeof...(Props)-1>(key) = *it;
      return true;
    } else return false;
  }

  template<typename P, typename... PP>
  inline bool find_in_sets(key_t& key, const P& p, const PP&... pp) const noexcept {
    const auto& set = std::get<sizeof...(Props)-sizeof...(PP)-1>(sets);
    const auto it = set.find(&p);

    if (it!=set.end()) {
      std::get<sizeof...(Props)-sizeof...(PP)-1>(key) = *it;
      return find_in_sets(key, pp...);
    } else return false;
  }

  // ----------------------------------------------------------------

public:
  void insert(const Mapped& x, const Props&... props) {
    key_t key;
    insert_all(key, props...);
    map.emplace( key, x );
  }

  template<size_t I> const list_t<I>& prop() const noexcept {
    return std::get<I>(lists);
  }

  bool get(Mapped& x, const Props&... props) const noexcept {
    key_t key;
    if (!find_in_sets(key, props...)) return false;
    auto it = map.find(key);
    if (it!=map.end()) {
      x = it->second;
      return true;
    } else return false;
  }

  template<size_t I> void sort() noexcept {
    std::get<I>(lists).sort();
  }
  template<size_t I, class Compare> void sort(Compare comp) noexcept {
    std::get<I>(lists).sort(comp);
  }
};

} // end namespace ivanp

#endif
