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

  TFile *f, *fout;
  fout = new TFile("~/work/wcsim/WCSim/DynaRange_TreeBuildMulti_Out.root","RECREATE");

  TTree *DRtree = new TTree("DRtree","DRtree Title");

  char filename[100];
  double start[3], stop[3]; // Location coordinates of source particle
  double E,maxchrg,meanchrg; // Initial energy of source, max PE captured, mean PE accross PMTs
  double PEthresh = 500.; // Threshold value
  int eventID;
  unsigned int nHits; // Number of PMT hits
  unsigned int nHitsThresh; // Number above threshold

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

  // Loop through files, building DRtree
  int iFirst = 2;
  int iLast = 4;
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

      for(int i = 0; i < maxhits; i++) {

	WCSimRootCherenkovHit *chit = (WCSimRootCherenkovHit*)wcsimrootevent->GetCherenkovHits()->At(i);
	double charge = chit->GetTotalPe(1);
	cmean += charge;

	if(charge > PEthresh)
	  nHitsThresh++;

	if(charge > clargest)
	  clargest = charge;

      }

      if(maxhits)
      	cmean /= (double)maxhits;
      else
      	std::cout << "Event " << ii << ":\tNo Cherenkov hits, probably outside detector" << std::endl;

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

      DRtree->Fill();

    }

  }

  fout->Write();


}
