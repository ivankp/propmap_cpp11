#ifndef ivanp_propmap_hh
#define ivanp_propmap_hh

#include <unordered_map>
#include <forward_list>
#include <tuple>

#include <iostream>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

namespace __propmap {

  // from boost (functional/hash):
  // see http://www.boost.org/doc/libs/1_35_0/doc/html/hash/combine.html
  // http://stackoverflow.com/questions/4948780
  // http://stackoverflow.com/questions/7110301
  // ----------------------------------------------------------------
  template <typename T>
  inline void hash_combine(size_t& seed, const T* v) noexcept {
    seed ^= reinterpret_cast<uintptr_t>(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
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
  using key_t = std::tuple<const Props* ...>;

  // property type
  template<size_t I>
  using prop_t = typename std::tuple_element<I, std::tuple<Props...>>::type;

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
  static inline const P*
  insert_single(cont_tmpl<P>& container, const P& p) {
    auto it = container.before_begin();
    bool found = false;
    for (const auto& x : container) {
      if (x==p) {
        found = true;
        ++it;
        break;
      }
    }
    if (!found) it = container.insert_after(it,p);
    return &(*it);
  }

  template<typename P>
  inline void insert_all(key_t& key, const P& p) {
    std::get<sizeof...(Props)-1>(key) = insert_single(
      std::get<sizeof...(Props)-1>(_containers), p
    );
  }

  template<typename P, typename... PP>
  inline void insert_all(key_t& key, const P& p, const PP&... pp) {
    std::get<sizeof...(Props)-sizeof...(PP)-1>(key) = insert_single(
      std::get<sizeof...(Props)-sizeof...(PP)-1>(_containers), p
    );
    insert_all(key,pp...);
  }

public:
  void insert(const Mapped& x, const Props&... props) {
    key_t key;
    insert_all(key,props...);
    _map.emplace( key, x );
  }

  template<size_t I> const cont_t<I>& prop() const noexcept {
    return std::get<I>(_containers);
  }

  bool get(Mapped& x, const Props&... props) const noexcept {
    auto it = _map.find( std::forward_as_tuple(&props...) );
    if (it!=_map.end()) {
      x = it->second;
      return true;
    } else return false;
  }

  template<size_t I> void sort() noexcept {
    std::get<I>(_containers).sort();
  }
  template<size_t I, class Compare> void sort(Compare comp) noexcept {
    std::get<I>(_containers).sort(comp);
  }
};

#endif
