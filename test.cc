#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <chrono>

#include "propmap.hh"

using namespace std;
using namespace std::chrono;
using namespace ivanp;

int main()
{
  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  // create a map
  propmap<
    pair<double,double>,
    prop_type<set,string>,
    prop_type<set,string>
  > pm;

/*
  // read data
  ifstream dat("test.dat");
  int num = 0;
  string name, id;
  double a, b;
  while (dat >> name >> id >> a >> b) {
    pm.insert(make_pair(a,b),name,id);
    ++num;
  }
  cout << "Entries: " << num << endl;

  // sort names alphabetically
  pm.sort<0>();

  // print collected data
  for (auto& name : pm.prop<0>()) {
    cout << endl << name << ':' << endl;
    for (auto& id : pm.prop<1>()) {

      static pair<double,double> val;
      if ( pm.get(val,name,id) ) {
        cout << "  " << id << ": "
             << val.first << " " << val.second << endl;
      }

    }
  }
*/
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
  cout << "\nTime elapsed: " << time_span.count() << " s" << endl;

  return 0;
}
