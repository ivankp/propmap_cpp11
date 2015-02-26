#ifndef __propmap_hh__
#define __propmap_hh__

#include <unordered_map>
// #include <vector>
#include <forward_list>
#include <tuple>
// #include <stdexcept>
// #include <sstream>

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
  /*
  template<typename T, typename... TT>
  struct to_str: public to_str<TT...> {
    to_str(const T& x, const TT&... xx): to_str<TT...>(xx...) {
      to_str<TT...>::ss << ", " << x;
    }
  };

  template<typename T>
  struct to_str<T> {
    to_str(const T& x): ss() { ss << x; }
    std::string str() { return std::move(ss.str()); }
  protected:
    std::stringstream ss;
  };
  */
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
  std::tuple<typename cont_tmpl<Props>::iterator ...> _backs;

  template<
    size_t I = sizeof...(Props) - 1,
    typename std::enable_if<I!=0>::type* = nullptr>
  void init_backs() noexcept {
    std::get<I>(_backs) = std::get<I>(_containers).before_begin();
    init_backs<I-1>();
  }

  template<
    size_t I = sizeof...(Props) - 1,
    typename std::enable_if<I==0>::type* = nullptr>
  void init_backs() noexcept {
    std::get<0>(_backs) = std::get<0>(_containers).before_begin();
  }

  template<typename P>
  void add_prop(const P& p) {
    // test(p)
    cont_t<0>& container = std::get<0>(_containers);
    std::get<0>(_backs) =
      container.insert_after( std::get<0>(_backs), p );
    auto it = container.before_begin();
    bool found = false;
    for (auto& x : container) {
      if (x==p) {
        found = true;
        break;
      }
      ++it;
    }
    if (!found) container.insert_after( std::get<0>(_backs), p );
  }

  template<typename P, typename... PP>
  void add_prop(const P& p, const PP&... pp) {
    using num = std::integral_constant<size_t, sizeof...(PP)>;
    cont_t<num::value>& container = std::get<num::value>(_containers);
    std::get<num::value>(_backs) =
      container.insert_after( std::get<num::value>(_backs), p );
    add_prop(pp...);
  }

public:
  // constructor
  propmap() { init_backs<>(); }

  void insert(const Mapped& x, const Props&... props) {
    add_prop(props...);/
    _map.emplace( std::forward_as_tuple(props...), x );
  }

  template<size_t I> const cont_t<I>& prop() {
    return std::get<I>(_containers);
  }

  bool get(Mapped& x, const Props&... props) const {
    auto it = _map.find( std::forward_as_tuple(props...) );
    if (it!=_map.end()) {
      x = it->second;
      return true;
    } else return false;
  }

  bool get(Mapped const*& x, const Props&... props) const {
    auto it = _map.find( std::forward_as_tuple(props...) );
    if (it!=_map.end()) {
      x = &(it->second);
      return true;
    } else return false;
  }

  bool get(Mapped*& x, const Props&... props) {
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

  template<size_t I> void sort() {
    std::get<I>(_containers).sort();
  }
  template<size_t I, class Compare> void sort(Compare comp) {
    std::get<I>(_containers).sort(comp);
  }
};


#endif
