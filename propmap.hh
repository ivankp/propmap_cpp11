#ifndef ivanp_propmap_hh
#define ivanp_propmap_hh

#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <list>

#include <iostream>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

namespace ivanp {

template<typename Mapped, typename... Props>
class propmap {

  template<size_t I>
  using prop_t = typename std::tuple_element<I, std::tuple<Props...>>::type;

  template<typename T> using hashed = std::tuple<size_t,T>;
  template<typename T> using hashed_ptr = hashed<const T*>;

  using key_t = hashed<std::tuple<const Props*...>>;

  template<typename T>
  struct hash_given {
    inline size_t operator()(const hashed<T>& key) const noexcept {
      return std::get<0>(key);
    }
  };

  template<typename T>
  struct set_eq {
    inline size_t operator()(const hashed_ptr<T>& lhs,
                             const hashed_ptr<T>& rhs) const noexcept
    {
      if ( std::get<0>(lhs) != std::get<0>(rhs) ) return false;
      else if ( &std::get<1>(lhs) != &std::get<1>(rhs) ) return false;
      else return true;
    }
  };

  template<typename T> using list_tmpl = std::list<T>;
  template<size_t I> using list_t = list_tmpl<prop_t<I>>;

  std::tuple<list_tmpl<Props>...> lists;

  std::tuple<
    std::unordered_set<
      hashed_ptr<Props>, hash_given<const Props*>, set_eq<Props>
    >...
  > sets;

  std::unordered_map<
    key_t, Mapped, hash_given<std::tuple<const Props*...>>
  > map;

  std::tuple<std::hash<Props>...> prop_hash;

  // from boost (functional/hash):
  // see http://www.boost.org/doc/libs/1_35_0/doc/html/hash/combine.html
  inline static void hash_combine(size_t& seed, size_t hash) noexcept {
    seed ^= hash + 0x9e3779b9 + (seed<<6) + (seed>>2);
  }

  template<size_t I>
  inline void insert_prop(key_t& key, const prop_t<I>& p) {
    const size_t hash = std::get<I>(prop_hash)(p);

    auto& set = std::get<I>(sets);
    auto it = set.find( std::make_tuple(hash,&p) );

    if (it==set.end()) {
      std::get<I>(lists).push_back(p);
      it = set.emplace( hash, &std::get<I>(lists).back() ).first;
    }
    std::get<I>(std::get<1>(key)) = std::get<1>(*it);

    hash_combine(std::get<0>(key), hash);
  }

  template<typename P>
  inline void add_props(key_t& key, const P& p) {
    std::get<0>(key) = 0;
    insert_prop<sizeof...(Props)-1>(key,p);
  }

  template<typename P, typename... PP>
  inline void add_props(key_t& key, const P& p, const PP&... pp) {
    add_props(key, pp...);
    insert_prop<sizeof...(Props)-sizeof...(PP)-1>(key,p);
  }

  template<typename P>
  inline bool find_in_sets(key_t& key, const P& p) const noexcept {
    const size_t hash = std::get<sizeof...(Props)-1>(prop_hash)(p);

    auto& set = std::get<sizeof...(Props)-1>(sets);

    auto it = set.find( std::make_tuple(hash,&p) );

    if (it!=set.end()) {
      std::get<0>(key) = hash;
      std::get<sizeof...(Props)-1>(std::get<1>(key))
        = std::get<1>(*it);
      return true;
    } else return false;
  }

  template<typename P, typename... PP>
  inline bool find_in_sets(key_t& key, const P& p, const PP&... pp) const noexcept {
    const size_t hash = std::get<sizeof...(Props)-sizeof...(PP)-1>(prop_hash)(p);

    auto& set = std::get<sizeof...(Props)-sizeof...(PP)-1>(sets);

    auto it = set.find( std::make_tuple(hash,&p) );

    if (it!=set.end()) {
      hash_combine(std::get<0>(key), hash);
      std::get<sizeof...(Props)-sizeof...(PP)-1>(std::get<1>(key))
        = std::get<1>(*it);
      return find_in_sets(key, pp...);
    } else return false;
  }

public:
  void insert(const Mapped& x, const Props&... props) {
    key_t key;
    add_props(key, props...);
    map.emplace(key, x);
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
  template<size_t I, typename Compare> void sort(Compare comp) noexcept {
    std::get<I>(lists).sort(comp);
  }

};

} // end namespace ivanp

#endif
