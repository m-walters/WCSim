#include <stdio.h>     
#include <stdlib.h>    
#include <cmath>
#include <string>
#include <vector>

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
  const char *filename[100];
  // std::vector<std::string> sFname;
  // std::vector<std::string>* psFname = &sFname;
} DRevent;


typedef struct{
  int i;
  int j;
  const char testchar[100];
  //  std::string Str;
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


void DynaRange_TreeBuildMulti() {


  gROOT->Reset();
  char* wcsimdirenv;

  wcsimdirenv = getenv ("WCSIMDIR");
  if(wcsimdirenv !=  NULL){
    gSystem->Load("${WCSIMDIR}/libWCSimRoot.so");
  }else{
    gSystem->Load("${HOME}/work/wcsim/WCSim/libWCSimRoot.so");
  }
  gStyle->SetOptStat(1);


  testclass *myclass = new testclass("llhi\0");
  myclass->setint(4);
  std::cout << myclass->c << " " << myclass->n << std::endl;


  test mytest = {1, 2, "gee\0"};
  // test mytest;
  // mytest.testchar = "testing";
  // mytest.i = 1;
  // mytest.j = 2;

  std::string str = (const char*)mytest.testchar;
  std::cout << str << std::endl;


  DRevent event;
  event.PEthresh = 500.;

  char fname[100];
  int iFirst = 2;
  int iLast = 4;

  TFile *f, *fout;
  fout = new TFile("~/work/wcsim/WCSim/DynaRange_TreeBuildMulti_Out.root","RECREATE");

  std::vector<std::string> stest;
  stest.push_back("great");
  std::vector<std::string>* ptest = &stest;

  const char c[100] = str.c_str();
  //  event.filename = "NAEM";

  TTree *DRtree = new TTree("DRtree","DRtree Title");
  DRtree->Branch("event",&event.start,"start[3]/D:stop[3]:E:maxchrg:meanchrg:PEthresh:eventID/I:nHits/i:nHitsThresh:filename/C");
  //DRtree->Branch("testb",&stest,"stest/C");
  DRtree->Branch("testchar",&c,"c/C");
  DRtree->Branch("teststruct",&mytest.i,"i/I:j:testchar/C");
  DRtree->Branch("testclass",&myclass,"c/C:n/I");

  // Loop through files, building DRtree
  for(int iFile = iFirst; iFile < (iLast+1); iFile++) {

    sprintf(fname,"/data14b/mwalters/wcsim_eIso%i.root",iFile);
    //event.filename = fname;


    f = new TFile(fname,"READ"); 
    if (!f->IsOpen()){
      cout << "Error, could not open input file: " << fname << endl;
      return -1;
    }

    TTree *wcsimT = f->Get("wcsimT");

    WCSimRootEvent *wcsimrootsuperevent = new WCSimRootEvent();
    wcsimT->SetBranchAddress("wcsimrootevent",&wcsimrootsuperevent);

    // Force deletion to prevent memory leak when issuing multiple
    // calls to GetEvent()
    wcsimT->GetBranch("wcsimrootevent")->SetAutoDelete(kTRUE);


    //LOOP
    int nevent = wcsimT->GetEntries();
    //nevent = 400;
    event.PEthresh = 500.;

    for(int ii = 0; ii < nevent; ii++) {

      if(ii%50==0)
      	std::cout << "Event " << ii << std::endl;

      event.eventID = ii;  
      event.nHits = 0;
      event.nHitsThresh = 0;

      wcsimT->GetEvent(ii);     
      // In the default vis.mac, only one event is run.  I suspect you could loop over more events, if they existed.
      WCSimRootTrigger *wcsimrootevent = wcsimrootsuperevent->GetTrigger(0);

      int maxhits = wcsimrootevent->GetNcherenkovhits();
      event.nHits = maxhits;

      double cmean = 0.;
      double clargest = 0.;

      for(int iC = 0; iC < maxhits; iC++) {

	WCSimRootCherenkovHit *chit = (WCSimRootCherenkovHit*)wcsimrootevent->GetCherenkovHits()->At(iC);

	cmean += cmean;

	if(chit->GetTotalPe(1) > event.PEthresh)
	  event.nHitsThresh++;

	if(chit->GetTotalPe(1) > clargest)
	  clargest = chit->GetTotalPe(1);

      }

      if(maxhits)
      	cmean /= maxhits;
      else
      	std::cout << ii << "\tNo Cherenkov hits, probably outside detector" << std::endl;

      event.meanchrg = cmean;
      event.maxchrg = clargest;

      // Point the track to the source particle. Normally would be At(0) but
      // there's this odd mirror that is produced everytime in that slot, so we want At(2)
      WCSimRootTrack *track = (WCSimRootTrack*)wcsimrootevent->GetTracks()->At(2);

      for(int q = 0; q < 3; q++) {
	event.start[q] = track->GetStart(q)/100;
	event.stop[q] = track->GetStop(q)/100;
      }

      event.E = track->GetE()/1000;


      DRtree->Fill();

    }

  }

  fout->Write();


}

#if !defined(__CINT__)
int main(void)
{
   DynaRange_TreeBuildMulti();
   return 0;
}
#endif
