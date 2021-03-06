#include "WCSimRunAction.hh"
#include "WCSimRunActionMessenger.hh"

#include "G4Run.hh"
#include "G4UImanager.hh"
#include "G4VVisManager.hh"
#include "G4ios.hh"

#include "jhfNtuple.h"

#ifdef REFLEX_DICTIONARY
#include "Cintex/Cintex.h"
#endif
#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TStreamerInfo.h"
#include "WCSimRootEvent.hh"
#include "WCSimRootGeom.hh"
#include "WCSimPmtInfo.hh"

#include <vector>

int pawc_[500000];                // Declare the PAWC common
struct ntupleStruct jhfNtuple;

WCSimRunAction::WCSimRunAction(WCSimDetectorConstruction* test)
{
  ntuples = 1;

  // Messenger to allow IO options
  wcsimdetector = test;
  messenger = new WCSimRunActionMessenger(this);

  // By default do not try and save Rootracker interaction information
  SetSaveRooTracker(0);

}

WCSimRunAction::~WCSimRunAction()
{

}

void WCSimRunAction::BeginOfRunAction(const G4Run* aRun)
{

  fSettingsOutputTree = NULL;
  fSettingsInputTree = NULL;
  for(int i = 0; i < 3; ++i){
      WCXRotation[i] = 0;
      WCYRotation[i] = 0;
      WCZRotation[i] = 0;
  }

  if(wcsimdetector->GetIsNuPrism()){
    if(GetSaveRooTracker()){
      //Setup settings tree
      fSettingsInputTree = (TTree*) gDirectory->Get("Settings");
    }
    if(fSettingsInputTree){
      fSettingsOutputTree = fSettingsInputTree->CloneTree(0);
    }
    else{
      fSettingsOutputTree = new TTree("Settings","Settings");
    }

    fSettingsOutputTree->Branch("WCXRotation", WCXRotation, "WCXRotation[3]/F");
    fSettingsOutputTree->Branch("WCYRotation", WCYRotation, "WCYRotation[3]/F");
    fSettingsOutputTree->Branch("WCZRotation", WCZRotation, "WCZRotation[3]/F");
    
  }      

//   G4cout << "### Run " << aRun->GetRunID() << " start." << G4endl;
  numberOfEventsGenerated = 0;
  numberOfTimesWaterTubeHit = 0;
  numberOfTimesCatcherHit = 0;

#ifdef REFLEX_DICTIONARY
  ROOT::Cintex::Cintex::Enable();
#endif

  // Needed for Root 4.00/04
  WCSimRootEvent::Class()->GetStreamerInfo()->Optimize(kFALSE);
  // MF, aug 2006 ... you never know...
  WCSimRootTrigger::Class()->GetStreamerInfo()->Optimize(kFALSE);

  // Create the Root file

  // Now controlled by the messenger
  
  G4String rootname = GetRootFileName();
  TFile* hfile = new TFile(rootname.c_str(),"RECREATE","WCSim ROOT file");
  hfile->SetCompressionLevel(2);

  // Event tree
  TTree* tree = new TTree("wcsimT","WCSim Tree");

  SetTree(tree);
  wcsimrootsuperevent = new WCSimRootEvent(); //empty list
  //  wcsimrootsuperevent->AddSubEvent(); // make at least one event
  wcsimrootsuperevent->Initialize(); // make at least one event
  Int_t branchStyle = 1; //new style by default
  TTree::SetBranchStyle(branchStyle);
  Int_t bufsize = 64000;

  //  TBranch *branch = tree->Branch("wcsimrootsuperevent", "Jhf2kmrootsuperevent", &wcsimrootsuperevent, bufsize,0);
  TBranch *branch = tree->Branch("wcsimrootevent", "WCSimRootEvent", &wcsimrootsuperevent, bufsize,2);

  // Geometry tree

  geoTree = new TTree("wcsimGeoT","WCSim Geometry Tree");
  SetGeoTree(geoTree);
  wcsimrootgeom = new WCSimRootGeom();
  TBranch *geoBranch = geoTree->Branch("wcsimrootgeom", "WCSimRootGeom", &wcsimrootgeom, bufsize,0);

  //Setup rooTracker tree
  if(SaveRooTracker){
    //Setup TClonesArray to store Rootracker truth info
    fVertices = new TClonesArray("NRooTrackerVtx", 10);
    fVertices->Clear();
    fNVtx = 0;
    fRooTrackerOutputTree = new TTree("fRooTrackerOutputTree","Event Vertex Truth Array");
    fRooTrackerOutputTree->Branch("NVtx",&fNVtx,"NVtx/I");
    fRooTrackerOutputTree->Branch("NRooTrackerVtx","TClonesArray", &fVertices);
  }

  FillGeoTree();

}

void WCSimRunAction::EndOfRunAction(const G4Run*)
{
//G4cout << "Number of Events Generated: "<< numberOfEventsGenerated << G4endl;
//G4cout << "Number of times MRD hit: " << numberOfTimesMRDHit << G4endl;
//G4cout << "Number of times FGD hit: "    << numberOfTimesFGDHit << G4endl;
//G4cout << "Number of times lArD hit: "  << numberOfTimeslArDHit << G4endl;
//G4cout<<"Number of times waterTube hit: " << numberOfTimesWaterTubeHit<<G4endl;
//   G4cout << ((float(numberOfTimesMRDHit)+float(numberOfTimesFGDHit))/float(numberOfEventsGenerated))*100.
// 	 << "% hit FGD or MRD" << G4endl;
//   G4cout << "Number of times Catcher hit: " << numberOfTimesCatcherHit<<G4endl;
//   G4cout << "Number of times Rock hit: " << numberOfTimesRockHit<<G4endl;
//  G4cout << (float(numberOfTimesCatcherHit)/float(numberOfEventsGenerated))*100.
//        << "% through-going (hit Catcher)" << G4endl;




  // Close the Root file at the end of the run
  TFile* hfile = WCSimTree->GetCurrentFile();
  hfile->Close();

  // Clean up stuff on the heap; I think deletion of hfile and trees
  // is taken care of by the file close

  delete wcsimrootsuperevent; wcsimrootsuperevent=0;
  delete wcsimrootgeom; wcsimrootgeom=0;

}

void WCSimRunAction::FillGeoTree(){

  // Fill the geometry tree
  G4int geo_type;
  G4double cylinfo[3];
  G4double pmtradius;
  G4int numpmt;
  G4int orientation;
  Float_t offset[3];
 
  Float_t rotation[3];

  Int_t tubeNo;
  Float_t pos[3];
  Float_t rot[3];
  Int_t cylLoc;

  if (wcsimdetector->GetIsHyperK()) {
      geo_type = 2;
  }
  else {
      geo_type = 0;
  }
  // geo_type 1 is for defunct mailbox design

  wcsimrootgeom-> SetGeo_Type(geo_type);

  if (geo_type == 0) {
      //cylinder
      cylinfo[1] = wcsimdetector->GetGeo_Dm(3);
      cylinfo[2] = wcsimdetector->GetGeo_Dm(2);
      wcsimrootgeom-> SetWCCylRadius(cylinfo[1]);
      wcsimrootgeom-> SetWCCylLength(cylinfo[2]);
  }


  pmtradius = wcsimdetector->GetPMTSize1();
  numpmt = wcsimdetector->GetTotalNumPmts();
  orientation = 0;
  
  wcsimrootgeom-> SetWCPMTRadius(pmtradius);
  wcsimrootgeom-> SetOrientation(orientation);
  
  G4ThreeVector offset1= wcsimdetector->GetWCOffset();
  offset[0] = offset1[0];
  offset[1] = offset1[1];
  offset[2] = offset1[2];
  wcsimrootgeom-> SetWCOffset(offset[0],offset[1],offset[2]);

  if(wcsimdetector->GetIsNuPrism()){ 
      G4ThreeVector rotation1= wcsimdetector->GetWCXRotation();
      WCXRotation[0] = rotation1[0];
      WCXRotation[1] = rotation1[1];
      WCXRotation[2] = rotation1[2];
    
      G4ThreeVector rotation2= wcsimdetector->GetWCYRotation();
      WCYRotation[0] = rotation2[0];
      WCYRotation[1] = rotation2[1];
      WCYRotation[2] = rotation2[2];
    
      G4ThreeVector rotation3= wcsimdetector->GetWCZRotation();
      WCZRotation[0] = rotation3[0];
      WCZRotation[1] = rotation3[1];
      WCZRotation[2] = rotation3[2];
      if(fSettingsInputTree) fSettingsInputTree->GetEntry(0); 
      fSettingsOutputTree->Fill();
  }

  std::vector<WCSimPmtInfo*> *fpmts = wcsimdetector->Get_Pmts();
  WCSimPmtInfo *pmt;
  for (int i=0;i!=fpmts->size();i++){
    pmt = ((WCSimPmtInfo*)fpmts->at(i));
    pos[0] = pmt->Get_transx();
    pos[1] = pmt->Get_transy();
    pos[2] = pmt->Get_transz();
    rot[0] = pmt->Get_orienx();
    rot[1] = pmt->Get_orieny();
    rot[2] = pmt->Get_orienz();
    tubeNo = pmt->Get_tubeid();
    cylLoc = pmt->Get_cylocation();
    wcsimrootgeom-> SetPMT(i,tubeNo,cylLoc,rot,pos);
  }
  if (fpmts->size() != numpmt) {
    G4cout << "Mismatch between number of pmts and pmt list in geofile.txt!!"<<G4endl;
    G4cout << fpmts->size() <<" vs. "<< numpmt <<G4endl;
  }
  
  wcsimrootgeom-> SetWCNumPMT(numpmt);
  
  geoTree->Fill();
  TFile* hfile = geoTree->GetCurrentFile();
  hfile->Write(); 
  if(wcsimdetector->GetIsNuPrism()) fSettingsOutputTree->Write();
      
}

NRooTrackerVtx* WCSimRunAction::GetRootrackerVertex(){
  NRooTrackerVtx* currRootrackerVtx = new((*fVertices)[fNVtx])NRooTrackerVtx();
  fNVtx += 1;
  return currRootrackerVtx;
}
