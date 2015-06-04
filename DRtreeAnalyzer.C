#include <stdio.h>     
#include <map>     
#include <stdlib.h>    
#include <cmath>
#include <string>

/* Michael Walters, June 2015
 *
 * Builds tree of certain data for
 * dynamic range analysis.
 * Takes WCSim .root files.
 *
 */



typedef struct{
  double start[3], stop[3];  // x,y,z location in meters
  double E,maxchrg,meanchrg; // initial energy of beam particle, max PE hit, mean PE of hits
  double PEthresh;
  int eventID; // event number
  unsigned int nHits, nHitsThresh;
  char *filename[100];
} DRevent;


typedef struct{
  int i;
  int j;
  const char testchar[100];
} test;


class testclass
{

public:
  char *c;
  int n;
  testclass() {};
  testclass(const char* d) : c(d){}
  ~testclass() {};

  void setchar(const char *b) {
    c = b;
  }

  void setint(int a) {
    n = a;
  }

};



void DRtreeAnalyzer() {


  DRevent myevent;
  test mytest;
  testclass *myclass = new testclass();

  TFile *file = new TFile("~/work/wcsim/WCSim/DynaRange_TreeBuildMulti_Out.root");
  TTree *tree = (TTree*)file->Get("DRtree");

  std::string filename;
  char q[100];
  tree->SetBranchAddress("filename",&q);
  int N = tree->GetEntries();
  for(int f = 0; f < N; f++) {
    tree->GetEntry(f);
    std::cout << q << std::endl;
  }





  //tree->SetBranchAddress("event",&myevent.start);

  // for(int i = 0; i < 1; i++) {

  //   tree->GetEntry(i);
  //   std::cout << myevent.start[0] << std::endl
  // 	      << myevent.maxchrg << std::endl
  // 	      << "filename: " << myevent.filename << std::endl
  // 	      << myevent.eventID << std::endl
  // 	      << myevent.PEthresh << std::endl
  // 	      << myevent.E << std::endl
  // 	      << myevent.nHits << std::endl
  // 	      << myevent.nHitsThresh << std::endl
  // 	      << std::endl;


  // }


  //char c[10];
  //std::string s();
  char d[100];
  tree->SetBranchAddress("testchar",d);

  for(int l = 0; l < 10; l++) {
    tree->GetEntry(l);
    std::cout << d << std::endl;
  }


  tree->SetBranchAddress("teststruct",&mytest.i);


  for(int p = 0; p < 10; p++) {
    tree->GetEntry(p);

    std::string stringgg = (const char*)mytest.testchar;

    std::cout << mytest.i << " "
    	      << mytest.j << " "
    	      << stringgg << std::endl;
  }



  // tree->SetBranchAddress("testclass",&myclass);
  // for(int m = 0; m < 10; m++) {
  //   tree->GetEntry(m);
  //   std::cout << myclass->n << " " << myclass->c << std::endl;
  // }



}
