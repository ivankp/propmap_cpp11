#include <iostream>
#include <fstream>
#include <string>

#include "propmap.hh"

using namespace std;

int main()
{
  // create a map
  propmap<pair<double,double>,string,string> pm;

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

  return 0;
}
