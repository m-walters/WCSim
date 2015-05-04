#include <stdio.h>     
#include <map>     
#include <stdlib.h>    
#include <cmath>


void check_michelsContained(char *filename=NULL) {
  /* Adapted version of check_michels by Michael Walters
   * for dynamic range testing
   *
   *
   * 
   * A simple script to plot aspects of phototube hits 
   * This code is rather cavalier; I should be checking return values, etc.
   * First revision 6-24-10 David Webber
   * 
   * I like to run this macro as 
   * $ root -l -x 'read_PMT.C("../wcsim.root")'
   */

  gROOT->Reset();
  char* wcsimdirenv;
  wcsimdirenv = getenv ("WCSIMDIR");
  if(wcsimdirenv !=  NULL){
    gSystem->Load("${WCSIMDIR}/libWCSimRoot.so");
  }else{
    gSystem->Load("../libWCSimRoot.so");
  }
  gStyle->SetOptStat(1);


  TFile *f, *fhistos;
  char fhistname[100];
  strcpy(fhistname,"plots_");
  strcat(fhistname,filename);
  std::cout << filename << " " << fhistname << std::endl;
  if (filename==NULL){
    f = new TFile("../wcsim.root");
  }else{
    f = new TFile(filename,"UPDATE"); 
    fhistos = new TFile(fhistname,"RECREATE");
  }
  if (!f->IsOpen()){
    cout << "Error, could not open input file: " << filename << endl;
    return -1;
  }

  TTree  *wcsimT = f->Get("wcsimT");

  WCSimRootEvent *wcsimrootsuperevent = new WCSimRootEvent();
  wcsimT->SetBranchAddress("wcsimrootevent",&wcsimrootsuperevent);

  // Initializing
  TH1D *PE = new TH1D("PEmult","Photoelectron multiplicty", 16,-0.5,15.5);
  PE->SetXTitle("Photoelectrons");

  TH1D *PMT_hits = new TH1D("PMT_hits","Hits vs PMT detector number", 120000,-0.5,120000-0.5);

  TH2D *QvsT = new TH2D("QvsT","charge vs. time", 40, 900, 1400, 800, -0.5, 300.5);
  QvsT->SetXTitle("time");
  QvsT->SetYTitle("charge");

  // TH1D *oldmchrg = ((TH1D*)(gROOT->FindObject("maxcharge")));
  // if(oldmchrg) {
  //   std::cout << "deleting oldmchrg\n";
  //   delete oldmchrg;
  // }


  TH1D *charge = new TH1D("charge","charge per PMT", 400, -0.5, 999.5);
  TH1D *maxcharge = new TH1D("maxcharge","Maximum charge per event", 50, -0.5, 999.5);
  TH1D *ccharge = new TH1D("ccharge","charge per PMT", 400, -0.5, 999.5);
  TH1D *cmaxcharge = new TH1D("cmaxcharge","Maximum charge per event", 100, -0.5, 999.5);

  TH1D *cherenkov_hittime = new TH1D("cherenkov_hittime","cherenkov_hittime", 200, 50, 140);
  TH1D *cherenkov_hits_per_tube = new TH1D("cherenkov_hits_per_tube","cherenkov_hits_per_tube", 20, -0.5, 19.5);
  TH1D *htdiff = new TH1D("tdiff","tdiff", 200, 850, 900);
  TH1D *htdiff_large = new TH1D("tdiff_large","tdiff_large", 200, 850, 900);

  TH1D *htdiff_arr[40];
  for (int i = 0; i < 40; i++){
    char name[100];
    sprintf(name,"tdiff_%i",i);
    htdiff_arr[i] = new TH1D(name,name, 200, 850, 900);
  }

 
  TH1F *michel_times = new TH1F("michel_times","Time of C-photons from decay e",1000,0,1200);
  TH1F *michel_n = new TH1F("michel_n","Number of Michel Photons",200,1,10000);
  TH1F *photons_n = new TH1F("photons_n","Number of Photons",200,1,10000);

  TH1F *michelDetected_n = new TH1F("michelDetected_n","Number of Michels Successfully Detected",50,1,800);
  //TH1F *michelTubes_n = new TH1F("michelTubes_n","Number of PMTs with In-Gate Decay Electron PEs",50,1,800);

  //TH1F *michel_frac = new TH1F("michel_frac","Fraction of Decay-e PEs that were detected",120,-0.1,1.1);



  TH2D *highPE_XYloc = new TH2D("highPE_XYloc","High PE Particle Origins",20,-40,40,20,-40,40);
  highPE_XYloc->SetXTitle("Location x(m)");
  highPE_XYloc->SetYTitle("Location y(m)");

  TH1D *highPE_mom = new TH1D("highPE_mom","High PE Particle Initial Momentum",110,-0.1,10.9);
  highPE_mom->SetXTitle("Initial Momentum (GeV)");

  TH1D *initEnrg = new TH1D("initEnrg","Initial Source Particle Energy",110,-0.1,10.9);
  initEnrg->SetXTitle("Initial Energy (GeV)");



  // Force deletion to prevent memory leak when issuing multiple
  // calls to GetEvent()
  wcsimT->GetBranch("wcsimrootevent")->SetAutoDelete(kTRUE);

  typedef std::map<int, int>::iterator it_type;

  double last = 0;

  double aspread = 0.0, aspreadn = 0.0;

  int nevent = wcsimT->GetEntries();
  int total_with_ingate_michel = 0;
  int total_with_subevent_michel = 0;
  //nevent = 100;

  int nOB = 0; //number out of bounds

  for(int ii = 0; ii < nevent; ii++){

    if(ii%50==0) 
      std::cout << "Event " << ii << std::endl;
  
    wcsimT->GetEvent(ii); 
    
    // In the default vis.mac, only one event is run.  I suspect you could loop over more events, if they existed.
    WCSimRootTrigger *wcsimrootevent = wcsimrootsuperevent->GetTrigger(0);

    if(0){

      std::cout << "Subevent: " << wcsimrootsuperevent->HasSubEvents()
    	      << " N triggers: " << wcsimrootsuperevent->GetNumberOfEvents() << std::endl;



      // Look at true tracks
      std::cout << "particles  : " << wcsimrootevent->GetNpar() << " " << wcsimrootevent->GetNtrack()
		<< std::endl;
    }

    bool hasElectron = false;
    int ElectronID = -1;
    double electronTime = 999999990.0;
    bool OB = false; // indicator for if particle started or ended out of bounds

    for(int i = 0; i < wcsimrootevent->GetNtrack(); i++){

      WCSimRootTrack *track = (WCSimRootTrack*)wcsimrootevent->GetTracks()->At(i);

      if(i == 2 && // source particle
	 ( fabs(track->GetStart(2))/100 > 49.5 
	   || fabs(track->GetStart(0))/100 > 34.0
	   || fabs(track->GetStart(1))/100 > 34.0
	   || fabs(track->GetStop(2))/100 > 49.5 
	   || fabs(track->GetStop(0))/100 > 34.0
	   || fabs(track->GetStop(1))/100 > 34.0)) 
	{
	
	OB = true;
	std::cout << ii << " OB event ommitted from analysis" << std::endl;

	}

      
      if(0)      std::cout << "particle type: " <<  track->GetIpnu()
		<< " " << track->GetFlag()
		<< " " << track->GetM()
		<< " " << track->GetP()
		<< " " << track->GetE()		
		<< " " << track->GetParenttype() 	
		<< " " << track->GetTime()		
		<< " " << track->GetId() << std::endl;

      if (track->GetIpnu() == 11){
	hasElectron = true;
	ElectronID = track->GetId();
	electronTime = track->GetTime();
      }	
    }

    if(OB == true) {
      nOB++;
      continue;
    }    

    if(hasElectron){
      total_with_ingate_michel++;
    }else{
      total_with_subevent_michel++;
    }
      

    /*
    if(wcsimrootsuperevent->GetNumberOfEvents() > 1){
      for(int j = 1; j < wcsimrootsuperevent->GetNumberOfEvents(); j++){
	WCSimRootTrigger *wcsimrootevent = wcsimrootsuperevent->GetTrigger(j);
	if(0)std::cout << "Sub " << j << std::endl;
	for(int i = 0; i < wcsimrootevent->GetNtrack(); i++){
	  WCSimRootTrack *track = (WCSimRootTrack*)wcsimrootevent->GetTracks()->At(i);

	  if(0)	  std::cout << "particle type: " <<  track->GetIpnu()
		    << " " << track->GetFlag()
		    << " " << track->GetM()
		    << " " << track->GetP()
		    << " " << track->GetE()		
		    << " " << track->GetParenttype() 	
		    << " " << track->GetTime()		
		    << " " << track->GetId() << std::endl;
	}
      }
    }
    */
    
    //-----------------------
    
    std::map<int, double > cthits_per_tube;
    std::map<int, int > chits_per_tube;
    std::map<int, int > cherenkov_id_per_tube;
    std::map<int, double> cparent;
    int max=wcsimrootevent->GetNcherenkovhits();

    double clargest = 0;
    for (int i = 0; i<max; i++){
      //cout << i << std::endl;
      WCSimRootCherenkovHit *chit = (WCSimRootCherenkovHit*)wcsimrootevent->GetCherenkovHits()->At(i);

      ccharge->Fill(chit->GetTotalPe(1));
      if(chit->GetTotalPe(1) > clargest)
        clargest = chit->GetTotalPe(1);


      continue;
      PMT_hits->Fill(chit->GetTubeID());
      //WCSimRootCherenkovHit has methods GetTubeId(), GetTotalPe(int)
      PE->Fill(chit->GetTotalPe(1));


      cherenkov_hits_per_tube->Fill(chit->GetTotalPe(1));
      chits_per_tube[chit->GetTubeID()] = chit->GetTotalPe(1) ;
      cthits_per_tube[chit->GetTubeID()] = 9999999.0;
      cherenkov_id_per_tube[chit->GetTubeID()] = i;

      // Find the minimum hit time
      for(int j = 0; j< chit->GetTotalPe(1); j++){
	
	WCSimRootCherenkovHitTime *cthit =(WCSimRootCherenkovHitTime*) wcsimrootevent->GetCherenkovHitTimes()->At(chit->GetTotalPe(0) + j);
	cherenkov_hittime->Fill(cthit->GetTruetime());
	//	if(last == cthit->GetTruetime())
	// std::cout << "! " << cthit->GetTruetime() << std::endl;
	
	last = cthit->GetTruetime();
	if(cthit->GetTruetime() < cthits_per_tube[chit->GetTubeID()])
	  cthits_per_tube[chit->GetTubeID()] =	(double)cthit->GetTruetime();
	

      }      

      // Make average of all hits within 4ns of first hit time.
      double avg = 0.0;
      double tot = 0.0;
      double min_time = cthits_per_tube[chit->GetTubeID()];
      for(int j = 0; j< chit->GetTotalPe(1); j++){
       
	WCSimRootCherenkovHitTime *cthit =(WCSimRootCherenkovHitTime*) wcsimrootevent->GetCherenkovHitTimes()->At(chit->GetTotalPe(0) + j);
	
	if(cthit->GetTruetime() - min_time < 4.0){
	  avg += cthit->GetTruetime();
	  tot += 1.0;
	}
      }      
      avg /= tot;
      //cthits_per_tube[chit->GetTubeID()] = avg;

      if(chit->GetTotalPe(1) > 10){
	for(int j = 0; j< chit->GetTotalPe(1); j++){
	  WCSimRootCherenkovHitTime *cthit = (WCSimRootCherenkovHitTime*)wcsimrootevent->GetCherenkovHitTimes()->At(chit->GetTotalPe(0) + j);
	  //std::cout << "Time " << cthit->GetTruetime() << std::endl;
	}
	//	return;
      }
    }
    cmaxcharge->Fill(clargest);










    // Point the track to the source particle. Normally would be At(0) but
    // there's this odd muon that is produced everytime in that slot, so we want At(2)
    track = (WCSimRootTrack*)wcsimrootevent->GetTracks()->At(2); 
    initEnrg->Fill(track->GetE()/1000);

    if(clargest > 200) {
      
      if (0) std::cout << "\nTop Track"
		       << "\nipnu " << track->GetIpnu()
		       << "\nID " << track->GetId()
		       << "\np " << track->GetP()
		       << "\nE " << track->GetE()
		       << "\nstart " << track->GetStart(0)
		       << "\nstop " << track->GetStop(0)
		       << "\nparent " << track->GetParenttype() << std::endl;
	
      highPE_XYloc->Fill(track->GetStart(0)/100,track->GetStart(1)/100);
      highPE_mom->Fill(track->GetP()/1000);


    }


    if(ii%50 == 0 && ii != 0)
      std::cout << "Percent OB: " << 100*(double)nOB/(double)ii << "%" << std::endl;

    if(ii == (nevent-1))
      std::cout << "Number of events excluded from 'High PE' plots analysis (out of bounds): " << nOB << std::endl;













    // List of how many tubes have Michel photons
    int nMichels=0;
    int nMichelsDetected=0;
    int nMichelPhotons=0;
    int nPhotons=0;

    std::map<int, int > list_of_digits;    
    double totcharge = 0;
    max = wcsimrootevent->GetNcherenkovdigihits();
    double largest = 0;
    for (int i = 0; i<max; i++){   // Digit loop
      WCSimRootCherenkovDigiHit *cDigiHit = (WCSimRootCherenkovDigiHit *)wcsimrootevent->GetCherenkovDigiHits()->At(i);
      //WCSimRootChernkovDigiHit has methods GetTubeId(), GetT(), GetQ()
      QvsT->Fill(cDigiHit->GetT(), cDigiHit->GetQ());
      charge->Fill(cDigiHit->GetQ());
      if(cDigiHit->GetQ() > largest)
        largest = cDigiHit->GetQ();

      continue;
      if(list_of_digits[cDigiHit->GetTubeId()] !=0){
	std::cout << "argh, already have hits for tube " << cDigiHit->GetTubeId()
		  << " " << list_of_digits[cDigiHit->GetTubeId()] 
		  << std::endl;
      }

      list_of_digits[cDigiHit->GetTubeId()] += 1;

      totcharge += cDigiHit->GetQ();

      double tdiff = cDigiHit->GetT() - cthits_per_tube[cDigiHit->GetTubeId()];
      //std::cout << "tdiff " << tdiff << std::endl;
      htdiff->Fill(tdiff);
      if(chits_per_tube[cDigiHit->GetTubeId()] > 3)
	htdiff_large->Fill(tdiff);

      if( chits_per_tube[cDigiHit->GetTubeId()] < 40){
	htdiff_arr[chits_per_tube[cDigiHit->GetTubeId()]]->Fill(tdiff);
      }else{
	htdiff_arr[39]->Fill(tdiff);

      }
      
      if(chits_per_tube[cDigiHit->GetTubeId()] == 0 && cDigiHit->GetQ() > 4){
	std::cout << "What?  hit with no photons? " << cDigiHit->GetQ() << " "
		  << wcsimrootevent->GetNcherenkovhits() << std::endl;
	for(int j=0; j <  wcsimrootevent->GetNcherenkovhits(); j++){
	  WCSimRootCherenkovHit *chit = (WCSimRootCherenkovHit*)wcsimrootevent->GetCherenkovHits()->At(j);
	  //	  std::cout << cDigiHit->GetTubeId() << " " << chit->GetTubeID() << std::endl;
	}
      }




      if(chits_per_tube[cDigiHit->GetTubeId()] > 0){
      

	WCSimRootCherenkovHit *chit = ( WCSimRootCherenkovHit *)wcsimrootevent->GetCherenkovHits()->At(cherenkov_id_per_tube[cDigiHit->GetTubeId()]);

	nPhotons += chit->GetTotalPe(1);
	//	std::cout << chit->GetTotalPe(1) << " " << nPhotons << std::endl;

	double least = 900000.0;
	double most = 0;
	bool isElectronHit = false;


	bool HasMichelPhotons = false;
	for(int j = 0; j< chit->GetTotalPe(1); j++){
	  
	  WCSimRootCherenkovHitTime *cthit = (WCSimRootCherenkovHitTime *) wcsimrootevent->GetCherenkovHitTimes()->At(chit->GetTotalPe(0) + j);
	  if (most < cthit->GetTruetime() 
	      && cthit->GetTruetime() - cthits_per_tube[cDigiHit->GetTubeId()] < 6.0)
	    most = cthit->GetTruetime();
	  if(least > cthit->GetTruetime())
	    least = cthit->GetTruetime();

	  //	  std::cout << cthit->GetTruetime() << " ";
	  // Identify photons from decay electrons.
	  if(fabs(cthit->GetTruetime()-electronTime-80) < 100.0 
	     && cthit->GetTruetime() > 350){
	    isElectronHit = true;
	    michel_times->Fill(cthit->GetTruetime());
	    	nMichels++;
	    HasMichelPhotons = true;
	    nMichelPhotons++;
	  }

	}

	if(HasMichelPhotons){
	  nMichels++;
	  if(cDigiHit->GetT() > 1100)  // Did we detect this delayed photon?
	    nMichelsDetected++;
	}

	//std::cout << "Etime: " << electronTime << "\n";
	
	if(isElectronHit && 0){
	  std::cout << "_______________________________________" << std::endl;
	  std::cout << "hit time: " << cDigiHit->GetT() << " " << chit->GetTotalPe(1) << " " 
		    << chits_per_tube[cDigiHit->GetTubeId()] << " " 
		    << cthits_per_tube[cDigiHit->GetTubeId()] << " " << tdiff <<  " " 
		    << cDigiHit->GetQ()<< std::endl;
	  for(int j = 0; j< chit->GetTotalPe(1); j++){
	    
	    WCSimRootCherenkovHitTime *cthit = (WCSimRootCherenkovHitTime *) wcsimrootevent->GetCherenkovHitTimes()->At(chit->GetTotalPe(0) + j);
	    //    std::cout << "! " << cthit->GetTruetime() << " " << cthit->GetParentID() <<  std::endl;
	    if(cthit->GetParentID()==ElectronID){
	      std::cout << "Found electron c-photon" << std::endl;	      
	    }
	    
	  }
	  	 
	}

	//if(isElectronHit) return;


      }
    }

    maxcharge->Fill(largest);

    
    michel_n->Fill(nMichelPhotons);
    photons_n->Fill(nPhotons);

    //michelTubes_n->Fill(nMichels);
    michelDetected_n->Fill(nMichelsDetected);

    if(nMichels > 0){
      //      std::cout << "Frac: " << (double)nMichelsDetected/(double)nMichels << std::endl;
      //michel_frac->Fill((double)nMichelsDetected/(double)nMichels);
    }
    //--------------------------
    // As you can see, there are lots of ways to get the number of hits.
    //    cout << "Number of tube hits " << wcsimrootevent->GetNumTubesHit() << endl;
    if(0){
      cout << "Number of Cherenkov tube hits " << wcsimrootevent->GetNcherenkovhits() << endl;
      cout << "Number of Cherenkov time hits " <<  wcsimrootevent->GetCherenkovHitTimes()->GetEntries() << endl;
      // cout << "Number of tube hits " << wcsimrootevent->GetCherenkovHits()->GetEntries() << endl;
      
      cout << "Number of digitized tube hits " << wcsimrootevent->GetNumDigiTubesHit() << endl;
      cout << "Number of digitized tube hits " << wcsimrootevent->GetCherenkovDigiHits()->GetEntries() << endl;
      
      //cout << "Number of digitized Cherenkov tube hits " << wcsimrootevent->GetNcherenkovdigihits() << endl;
      //cout << "Number of digitized Cherenkov tube hits " << wcsimrootevent->GetCherenkovDigiHits()->GetEntries() << endl;
      //cout << "Number of photoelectron hit times" << wcsimrootevent->GetCherenkovHitTimes()->GetEntries() << endl;
      std::cout <<"Tot charge: " << totcharge << std::endl;
    }
  }

  

  TH1 *temp;
  float win_scale=0.75;
  int n_wide=2;
  int n_high=3;
  TCanvas *c1 = new TCanvas("c1","c1",700*n_wide*win_scale,500*n_high*win_scale);
  c1->Divide(n_wide,n_high);
  c1->cd(1);
  QvsT->Draw("colz");

  c1->cd(2);
  temp=QvsT->ProjectionY();
  temp->SetTitle("charge");
  temp->Draw();
  c1->GetPad(2)->SetLogy();

  c1->cd(3);
  temp=QvsT->ProjectionX();
  temp->SetTitle("hits vs time");
  temp->Draw();
  c1->GetPad(3)->SetLogy();

  c1->cd(4);
  temp=QvsT->ProfileX();
  temp->SetTitle("average charge vs time");
  temp->Draw();

  c1->cd(5);
  temp=PE;
  temp->Draw();
  c1->GetPad(5)->SetLogy();



  TCanvas *cLoc = new TCanvas("cloc","cLoc",1000,1000);
  highPE_XYloc->Draw("colz");

  TCanvas *cMom = new TCanvas("cMom","cMom",1100,720);
  highPE_mom->Draw();

  TCanvas *iEnrg = new TCanvas("iEnrg","iEnrg",1100,720);
  initEnrg->Draw();

//   TCanvas *c2 = new TCanvas("c2","c2");

//   cherenkov_hittime->Draw();


//   TCanvas *c3 = new TCanvas("c3","c3");
//   cherenkov_hits_per_tube->Draw();

//   TCanvas *c3 = new TCanvas("c4","c4");
//   htdiff->Draw();
//   htdiff->SetTitle("Time Difference ");
//   htdiff->SetXTitle("tdiff between digi time and first photon time (ns)");
//   htdiff_arr[1]->Draw("SAME");
//   htdiff_arr[1]->SetLineColor(2);
//   htdiff_arr[3]->Draw("SAME");
//   htdiff_arr[3]->SetLineColor(3);
//   htdiff_arr[5]->Draw("SAME");
//   htdiff_arr[5]->SetLineColor(4);
//   htdiff_arr[10]->Draw("SAME");
//   htdiff_arr[10]->SetLineColor(5);
//   htdiff_arr[14]->Draw("SAME");
//   htdiff_arr[14]->SetLineColor(6);

//   TLegend *legg = new TLegend(0.11,0.6,0.4,0.89);
//   legg->AddEntry(htdiff_arr[1],"2PE hits");
//   legg->AddEntry(htdiff_arr[3],"4PE hits");
//   legg->AddEntry(htdiff_arr[5],"6PE hits");
//   legg->AddEntry(htdiff_arr[10],"11PE hits");
//   legg->AddEntry(htdiff_arr[14],"15PE hits");
  
//   gPad->SetLogy();
//   legg->Draw("SAME");

//   TCanvas *c4 = new TCanvas("c5","c5");
  
//   TGraphErrors *gr = new TGraphErrors();
  
//   for(int i = 0; i < 40; i++){

//     if(htdiff_arr[i]->GetEntries() < 50) continue;
    
//     TF1 func("f1","gaus",864,890);
//     htdiff_arr[i]->Fit("f1","R");
    
//     std::cout << i << " " <<  func.GetParameter(0) 
// 	      << "  "<< func.GetParameter(1)
// 	      << "  "<< func.GetParameter(2) << std::endl;
    
//     gr->SetPoint(i,i,func.GetParameter(2));
//     gr->SetPointError(i,0,func.GetParError(2));
    

//   }

//   gr->Draw("AP*");
//   gr->GetXaxis()->SetTitle("Hit Charge (pe)");
//   gr->GetYaxis()->SetTitle("Timing Resolution (ns)");
//   TF1 *inv = new TF1("inv","3.16/sqrt(x) + 0.33",0.5,40);
//   inv->Draw("SAME");

//   TCanvas *c5 = new TCanvas("C5");  
//   photons_n->Draw();
//   photons_n->SetTitle("Number of True PE per event");
//   photons_n->SetXTitle("Number of True PE");

//   michel_n->Draw("SAME");
//   michel_n->SetLineColor(2);
  
//   TLegend *leg5 = new TLegend(0.3,0.55,0.6,0.85);
//   leg5->AddEntry(photons_n,"All PEs");
//   leg5->AddEntry(michel_n,"PEs from in-gate decay-e");
//   leg5->Draw("SAME");

// //  TCanvas *c6 = new TCanvas("C6");  
// //  TF1 *f1 = new TF1("f1","[0]*exp(-x/2000)",400,1000);
// //  michel_times->Fit("f1","R");
// //  michel_times->Draw();
// //  michel_times->SetTitle("Times of In-gate Delayed PE");
// //  michel_times->SetXTitle("Delayed PE Time (ns)");

  

//   //TCanvas *c7 = new TCanvas("C7");  
//   //michel_frac->Draw();
//   //michel_frac->SetXTitle("Fraction of PMTs where decay-e PE was detected");


//   //TCanvas *c8 = new TCanvas("C8");  
//   //michelTubes_n->Draw();
//   //michelTubes_n->SetXTitle("#PMTs with decay-e PE");
//   //michelDetected_n->Draw("SAME");
//   //michelDetected_n->SetLineColor(2);

//   TLegend *leg8 = new TLegend(0.6,0.55,0.89,0.85);
//   //leg8->AddEntry(michelTubes_n,"All PMTs with decay-e PEs");
//   leg8->AddEntry(michelDetected_n,"PMTs with accurate decay-e time");
//   leg8->Draw("SAME");

//   std::cout<< "Events: " << total_with_subevent_michel
// 	   << " " << total_with_ingate_michel
// 	   << " " << nevent << std::endl;


  TCanvas *c9 = new TCanvas("C9","chg",1600,900);  
  charge->SetXTitle("Charge (PE)");
  charge->Draw();
  gPad->SetLogy();
  //ccharge->Draw("SAME");
  //ccharge->SetLineColor(2);;


  TCanvas *c10 = new TCanvas("C10","max_c",1600,900);  
  maxcharge->SetXTitle("Max PMT Charge Per Event (PE)");
  maxcharge->Draw();
  gPad->SetLogy();
  //cmaxcharge->Draw("SAME");
  //cmaxcharge->SetLineColor(2);;

  charge->Write();
  maxcharge->Write();


  fhistos->Write();


}
