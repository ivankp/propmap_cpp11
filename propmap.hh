#ifndef ivanp_propmap_hh
#define ivanp_propmap_hh

#include <unordered_map>
#include <forward_list>
#include <tuple>

#include <iostream>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

namespace ivanp {

template<typename Mapped, typename... Props>
class propmap {

  // map defs and functions -----------------------------------------

  // key tuple type
  using key_t = std::tuple<Props...>;

  // tupple hashing from boost (functional/hash):
  // see http://www.boost.org/doc/libs/1_35_0/doc/html/hash/combine.html
  // http://stackoverflow.com/questions/4948780
  // http://stackoverflow.com/questions/7110301

  template<typename T>
  static inline void hash_combine(size_t& seed, const T& v) noexcept {
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
  }

  template <size_t I, typename std::enable_if<I>::type * = nullptr>
  static inline void key_hash_impl(size_t& seed, const key_t& key) noexcept {
    hash_combine(seed, std::get<I>(key) );
    key_hash_impl<I-1>(seed, key);
  }

  template <size_t I, typename std::enable_if<!I>::type * = nullptr>
  static inline void key_hash_impl(size_t& seed, const key_t& key) noexcept {
    hash_combine(seed, std::get<0>(key) );
  }

  struct key_hash {
    size_t operator()(const key_t& key) const noexcept {
      size_t seed = 0;
      key_hash_impl<sizeof...(Props)-1>(seed, key);
      return seed;
    }
  };

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
  std::unordered_map<key_t,Mapped,key_hash> map;
  std::tuple<cont_tmpl<Props> ...> lists;

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
    container_insert(std::get<sizeof...(Props)-1>(lists), p);
  }

  template<typename P, typename... PP>
  inline void add_prop(const P& p, const PP&... pp) {
    container_insert(
      std::get<sizeof...(Props)-sizeof...(PP)-1>(lists), p
    );
    add_prop(pp...);
  }

public:
  void insert(const Mapped& x, const Props&... props) {
    add_prop(props...);
    map.emplace( std::tie(props...), x );
  }

  template<size_t I> const cont_t<I>& prop() const noexcept {
    return std::get<I>(lists);
  }

  bool get(Mapped& x, const Props&... props) const noexcept {
    auto it = map.find( std::tie(props...) );
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
