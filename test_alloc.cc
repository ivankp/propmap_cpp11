#include <iostream>

#include "propmap.hh"

using namespace std;

struct foo {
  int i, x;

  foo(int x): i(0), x(x) {
    cout << x << ": " << i << "+" << 1 << "=" << i+1 << endl;
    ++i;
  }
  ~foo() {
    cout << x << ": " << i << "-" << 1 << "=" << i-1 << endl;
    --i;
  }

  bool operator<(foo other) const { return x < other.x; }
};

namespace std {
  template<> struct hash<foo> {
    size_t operator()(const foo& f) {
      return hash<int>()(f.x);
    }
  };
}

int main()
{
  foo a(1), b(2), c(3);
  cout << "----------\n" << endl;

  propmap<int,foo,foo> pm;

  pm.insert(1,a,b);
  // pm.insert(2,b);
  pm.insert(4,c,b);
  pm.insert(4,b,c);
  cout << "----------\n" << endl;

  pm.sort<0>();

  for (auto& f : pm.prop<0>()) cout << f.x << ' ';
  cout << endl;
  for (auto& f : pm.prop<1>()) cout << f.x << ' ';
  cout << endl;
  cout << "----------\n" << endl;

  return 0;
}
