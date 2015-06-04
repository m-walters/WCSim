#include <stdio.h>     
#include <map>     
#include <stdlib.h>    
#include <cmath>
#include <string>

// Michael Walters, June 2015


void DRtreeAnalyzer() {

  TFile *file = new TFile("~/work/wcsim/WCSim/DynaRange_TreeBuildMulti_Out.root");
  TTree *tree = (TTree*)file->Get("DRtree");


  char q[100];
  tree->SetBranchAddress("filename",&q);
  int N = tree->GetEntries();

  for(int f = 0; f < N; f++) {
    tree->GetEntry(f);
    std::cout << q << std::endl;
  }

}
