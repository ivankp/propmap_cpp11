#ifndef __propmap_hh__
#define __propmap_hh__

#include <unordered_map>
#include <forward_list>
#include <tuple>
#include <memory>

#include <iostream>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

namespace __propmap {

  // Hashing code between ---- from boost
  // Reciprocal of the golden ratio helps spread entropy
  //     and handles duplicates.
  // See Mike Seymour in magic-numbers-in-boosthash-combine:
  //     http://stackoverflow.com/questions/4948780
  // Adopted from:
  //     http://stackoverflow.com/questions/7110301

  // ----------------------------------------------------------------
  template <typename T>
  inline void ptr_hash_combine(size_t& seed, const std::shared_ptr<T>& v) noexcept {
    static std::hash<T> hasher;
    seed ^= hasher(*v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
  }

  template <typename Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
  struct key_hash_impl {
    static void apply(size_t& seed, const Tuple& tuple) noexcept {
      key_hash_impl<Tuple, Index-1>::apply(seed, tuple);
      ptr_hash_combine(seed, std::get<Index>(tuple));
    }
  };

  template <typename Tuple>
  struct key_hash_impl<Tuple,0> {
    static void apply(size_t& seed, const Tuple& tuple) noexcept {
      ptr_hash_combine(seed, std::get<0>(tuple));
    }
  };

  template<typename Tuple> struct tuple_hash;
  template<typename... T>
  struct key_hash<std::tuple<T...>> {
    size_t operator()(const std::tuple<T...>& tuple) const noexcept {
      size_t seed = 0;
      key_hash_impl<std::tuple<T...>>::apply(seed, tuple);
      return seed;
    }
  };
  // ----------------------------------------------------------------
}

// ******************************************************************

template<typename Mapped, typename... Props>
class propmap {
public:
  // property pointer template
  template<typename P>
  using ptr_tmpl = std::shared_ptr<P>;

  // key tuple type
  using key_t = std::tuple<ptr_tmpl<Props>...>;

  // property pointer type
  template<size_t I>
  using ptr_t = typename std::tuple_element<I, key_t>::type;

  // property type
  template<size_t I>
  using prop_t = typename ptr_t<I>::element_type;

  // property container template
  template<typename P>
  using cont_tmpl = std::forward_list<P>;

  // property container type
  template<size_t I>
  using cont_t = cont_tmpl<ptr_t<I>>;

private:
  std::unordered_map<key_t,Mapped,__propmap::key_hash<key_t>> _map;
  std::tuple<cont_tmpl<Props> ...> _containers;

  template<typename P>
  static inline const ptr_tmpl<P>& container_insert(cont_tmpl<P>& container, const P& p) {
    auto it = container.before_begin();
    bool found = false;
    for (const auto& x : container) {
      if ( (*x) == p ) {
        found = true;
        break;
      }
      ++it;
    }
    if (!found) it = container.insert_after(it, new P(p));
    return *it;
  }

  template<typename P>
  inline void add_prop(key_t& key, const P& p) {
    std::get<sizeof...(Props)-1>(key) =
      container_insert(std::get<sizeof...(Props)-1>(_containers), p);
  }

  template<typename P, typename... PP>
  inline void add_prop(key_t& key, const P& p, const PP&... pp) {
    std::get<sizeof...(Props)-sizeof...(PP)-1>(key) =
      container_insert(
        std::get<sizeof...(Props)-sizeof...(PP)-1>(_containers), p
      );
    add_prop(pp...);
  }

public:
  void insert(const Mapped& x, const Props&... props) {
    key_t key;
    add_prop(key, props...);
    _map.emplace( key, x );
  }

  template<size_t I> const cont_t<I>& prop() const noexcept {
    return std::get<I>(_containers);
  }

  bool get(Mapped& x, const Props&... props) const noexcept {
    auto it = _map.find( std::forward_as_tuple(props...) );
    if (it!=_map.end()) {
      x = it->second;
      return true;
    } else return false;
  }

  bool get(Mapped const*& x, const Props&... props) const noexcept {
    auto it = _map.find( std::forward_as_tuple(props...) );
    if (it!=_map.end()) {
      x = &(it->second);
      return true;
    } else return false;
  }

  bool get(Mapped*& x, const Props&... props) noexcept {
    auto it = _map.find( std::forward_as_tuple(props...) );
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
    std::get<I>(_containers).sort();
  }
  template<size_t I, class Compare> void sort(Compare comp) noexcept {
    std::get<I>(_containers).sort(comp);
  }
};

#endif
