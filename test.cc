#include <iostream>
#include <fstream>
#include <string>

#include "propmap.hh"

using namespace std;

int main(int argc, char **argv)
{
  // program options
  if (argc!=2) {
    cout << "Usage: " << argv[0] << "file.dat" << endl;
    return 0;
  }

  // create a map
  propmap<pair<double,double>,string,string> pm;

  // read data
  ifstream dat(argv[1]);
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
