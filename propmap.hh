#ifndef ivanp_propmap_hh
#define ivanp_propmap_hh

#include <unordered_map>
#include <forward_list>
#include <tuple>

namespace ivanp {

template<typename Mapped, typename... Props>
class propmap {
public:

  // types ----------------------------------------------------------

  // key tuple type
  using key_t = std::tuple<Props...>;

  // property type
  template<size_t I>
  using prop_t = typename std::tuple_element<I, key_t>::type;

  // property list template
  template<typename P>
  using list_tmpl = std::forward_list<P>;

  // property list type
  template<size_t I>
  using list_t = list_tmpl<prop_t<I>>;

private:

  // hashing --------------------------------------------------------

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

  // member containers ----------------------------------------------

  std::unordered_map<key_t,Mapped,key_hash> map;
  std::tuple<list_tmpl<Props>...> lists;

  // private functions ----------------------------------------------

  template<typename P>
  static inline void insert_one(list_tmpl<P>& list, const P& p) {
    auto it = list.before_begin();
    bool found = false;
    for (const auto& x : list) {
      if (x==p) {
        found = true;
        break;
      }
      ++it;
    }
    if (!found) list.insert_after(it,p);
  }

  template<typename P>
  inline void insert_all(const P& p) {
    insert_one(std::get<sizeof...(Props)-1>(lists), p);
  }

  template<typename P, typename... PP>
  inline void insert_all(const P& p, const PP&... pp) {
    insert_one(
      std::get<sizeof...(Props)-sizeof...(PP)-1>(lists), p
    );
    insert_all(pp...);
  }

  // ----------------------------------------------------------------

public:
  void insert(const Mapped& x, const Props&... props) {
    insert_all(props...);
    map.emplace( std::tie(props...), x );
  }

  template<size_t I> const list_t<I>& prop() const noexcept {
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
