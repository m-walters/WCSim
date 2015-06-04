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
  //const char *filename[100];
  //std::string filename();

  void setstring(const char* c) {
    filename = c;
  };

} DRevent;


typedef struct{
  int i;
  int j;
  const char testchar[100];
} test;


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


  test mytest = {1, 2, "gee\0"};
  // test mytest;
  // mytest.testchar = "testing";
  // mytest.i = 1;
  // mytest.j = 2;

  std::string str = (const char*)mytest.testchar;
  std::cout << str << std::endl;

  // Couldn't manage to send the 'filename' string in a non-initialization
  // method for the DRevent struct. So struct init is near end
  //char filename[100];
  //std::string filename;
  char filename[100];
  double start[3], stop[3];
  double E,maxchrg,meanchrg;
  double PEthresh = 500.;
  int eventID;
  unsigned int nHits = 0;
  unsigned int nHitsThresh = 0;

  DRevent event;

  int iFirst = 2;
  int iLast = 4;

  TFile *f, *fout;
  fout = new TFile("~/work/wcsim/WCSim/DynaRange_TreeBuildMulti_Out.root","RECREATE");


  //  const char c[100] = str.c_str();
  char c[100] = "frig";

  TTree *DRtree = new TTree("DRtree","DRtree Title");
  DRtree->Branch("filename",&filename,"filename/C");
  DRtree->Branch("start",&start,"start[3]/D");
  DRtree->Branch("stop",&stop,"stop[3]/D");
  DRtree->Branch("E",&E,"E/D");
  DRtree->Branch("maxchrg",&maxchrg,"maxchrg/D");
  DRtree->Branch("meanchrg",&meanchrg,"meanchrg/D");
  DRtree->Branch("PEthresh",&PEthresh,"PEthresh/D");
  DRtree->Branch("eventID",&eventID,"eventID/I");
  DRtree->Branch("nHits",&nHits,"nHits/i");
  DRtree->Branch("nHitsThresh",&nHitsThresh,"nHitsThresh/i");


  //DRtree->Branch("event",&event.start,"start[3]/D:stop[3]:E:maxchrg:meanchrg:PEthresh:eventID/I:nHits/i:nHitsThresh:filename/C");
  DRtree->Branch("teststruct",&mytest.i,"i/I:j:testchar/C");
  DRtree->Branch("testchar",&c,"c/C");

  // Loop through files, building DRtree
  for(int iFile = iFirst; iFile < (iLast+1); iFile++) {

    sprintf(filename,"/data14b/mwalters/wcsim_eIso%i.root",iFile);

    f = new TFile(filename,"READ"); 
    if (!f->IsOpen()){
      cout << "Error, could not open input file: " << filename << endl;
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

    for(int ii = 0; ii < nevent; ii++) {

      eventID = ii;  
      if(ii%50==0)
      	std::cout << "Event " << ii << std::endl;

      wcsimT->GetEvent(ii);     
      // In the default vis.mac, only one event is run.  I suspect you could loop over more events, if they existed.
      WCSimRootTrigger *wcsimrootevent = wcsimrootsuperevent->GetTrigger(0);

      int maxhits = wcsimrootevent->GetNcherenkovhits();
      nHits = maxhits;
      nHitsThresh = 0;

      double cmean = 0.;
      double clargest = 0.;

      for(int iC = 0; iC < maxhits; iC++) {

	WCSimRootCherenkovHit *chit = (WCSimRootCherenkovHit*)wcsimrootevent->GetCherenkovHits()->At(iC);

	cmean += chit->GetTotalPe(1);

	if(chit->GetTotalPe(1) > PEthresh)
	  nHitsThresh++;

	if(chit->GetTotalPe(1) > clargest)
	  clargest = chit->GetTotalPe(1);

      }

      if(maxhits)
      	cmean /= maxhits;
      else
      	std::cout << ii << "\tNo Cherenkov hits, probably outside detector" << std::endl;

      meanchrg = cmean;
      maxchrg = clargest;

      // Point the track to the source particle. Normally would be At(0) but
      // there's this odd mirror that is produced everytime in that slot, so we want At(2)
      WCSimRootTrack *track = (WCSimRootTrack*)wcsimrootevent->GetTracks()->At(2);

      // Start and Stop vertices for source particle
      for(int q = 0; q < 3; q++) {
	start[q] = track->GetStart(q)/100;
	stop[q] = track->GetStop(q)/100;
      }

      E = track->GetE()/1000;

      //DRevent event = {start,stop,E,maxchrg,meanchrg,PEthresh,eventID,nHits,nHitsThresh,filename};
      DRtree->Fill();

    }

  }

  fout->Write();


}
