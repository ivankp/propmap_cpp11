#ifndef __propmap_hh__
#define __propmap_hh__

#include <unordered_map>
#include <forward_list>
#include <tuple>

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
  inline void hash_combine(size_t& seed, const T& v) noexcept {
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
  }

  template <typename Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
  struct tuple_hash_impl {
    static void apply(size_t& seed, const Tuple& tuple) noexcept {
      tuple_hash_impl<Tuple, Index-1>::apply(seed, tuple);
      hash_combine(seed, std::get<Index>(tuple));
    }
  };

  template <typename Tuple>
  struct tuple_hash_impl<Tuple,0> {
    static void apply(size_t& seed, const Tuple& tuple) noexcept {
      hash_combine(seed, std::get<0>(tuple));
    }
  };

  template<typename Tuple> struct tuple_hash;
  template<typename... T>
  struct tuple_hash<std::tuple<T...>> {
    size_t operator()(const std::tuple<T...>& tuple) const noexcept {
      size_t seed = 0;
      tuple_hash_impl<std::tuple<T...>>::apply(seed, tuple);
      return seed;
    }
  };
  // ----------------------------------------------------------------
}

// ******************************************************************

template<typename Mapped, typename... Props>
class propmap {
public:
  // key tuple type
  using key_t = std::tuple<Props...>;

  // property type
  template<size_t I>
  using prop_t = typename std::tuple_element<I, key_t>::type;

  // property container template
  template<typename P>
  using cont_tmpl = std::forward_list<P>;

  // property container type
  template<size_t I>
  using cont_t = cont_tmpl<prop_t<I>>;

private:
  std::unordered_map<key_t,Mapped,__propmap::tuple_hash<key_t>> _map;
  std::tuple<cont_tmpl<Props> ...> _containers;

  template<typename P>
  static inline void container_insert(cont_tmpl<P>& container, const P& p) {
    auto it = container.before_begin();
    bool found = false;
    for (const auto& x : container) {
      if (x==p) {
        found = true;
        break;
      }
      ++it;
    }
    if (!found) container.insert_after(it,p);
  }

  template<typename P>
  inline void add_prop(const P& p) {
    container_insert(std::get<sizeof...(Props)-1>(_containers), p);
  }

  template<typename P, typename... PP>
  inline void add_prop(const P& p, const PP&... pp) {
    container_insert(
      std::get<sizeof...(Props)-sizeof...(PP)-1>(_containers), p
    );
    add_prop(pp...);
  }

public:
  void insert(const Mapped& x, const Props&... props) {
    add_prop(props...);
    _map.emplace( std::forward_as_tuple(props...), x );
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
