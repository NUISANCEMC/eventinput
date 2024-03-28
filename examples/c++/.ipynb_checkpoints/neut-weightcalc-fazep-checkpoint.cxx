#include "nuis/eventinput/EventSourceFactory.h"

#include "NuHepMC/EventUtils.hxx"
#include "NuHepMC/FATXUtils.hxx"
#include "NuHepMC/ReaderUtils.hxx"

#include "nuis/log.txx"
#include "nuis/weightcalc/WeightCalcFactory.h"
#include "nuis/weightcalc/plugins/NReWeightCalc.h"
#include "nuis/weightcalc/plugins/IWeightCalcPlugin.h"

#include "TH1D.h"
#include "TH2D.h"

#include "TFile.h"


#include "NReWeight.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

using namespace nuis;

#include "NuXSecCCQE.h"

#include "NModeDefn.h"
#include "NuXSecCCQE.h"

#include "CommonBlockIFace.h"

#include "neutcrsC.h"
#include "neworkC.h"

#include <iostream>
#include "NReWeightEngineI.h"

#include "Eigen/Dense"
using Eigen::MatrixXd;
using Eigen::VectorXd;

namespace PRD_93_113015 {
Eigen::Vector4d a_14_cv{2.28, 0.25, -5.2, 2.6};
Eigen::Vector4d a_14_errors{0.08, 0.95, 2.3, 2.7};
Eigen::MatrixXd Correlation_Matrix{ {+1.000, +0.321, -0.677, +0.761}, 
                                    {+0.321, +1.000, -0.889, +0.313},
                                    {-0.677, -0.889, +1.000, -0.689},
                                    {+0.761, +0.313, -0.689, +1.000} };
} // namespace PRD_93_113015


Eigen::MatrixXd
GetPCAFromCovarianceMatrix(Eigen::MatrixXd const &Covariance_Matrix) {
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigensolver(Covariance_Matrix);
  if (eigensolver.info() != Eigen::Success)
    abort();
  Eigen::MatrixXd eigenvector_colmatrix = eigensolver.eigenvectors();
  Eigen::VectorXd eigenvalues_sqrt = (eigensolver.eigenvalues()).array().sqrt();
  for (size_t i = 0; i < 4; ++i) {
    eigenvector_colmatrix.col(i) *= eigenvalues_sqrt[i];
  }
  return eigenvector_colmatrix;
}

Eigen::VectorXd ChangeBasisBParams(std::vector<double> const &BVals,
                                   Eigen::MatrixXd const &PCA) {
  Eigen::VectorXd avals = Eigen::VectorXd::Zero(4);
  for (size_t i = 0; i < 4; ++i) {
    avals += PCA.col(i) * BVals[i];
  }
  return avals;
}

namespace neut {
namespace rew {





class NuXSecCCQEEngineModifiedZEXP : public NReWeightEngineI {
public:
  NuXSecCCQEEngineModifiedZEXP();
  ~NuXSecCCQEEngineModifiedZEXP() {}

  void Reconfigure();
  double CalcWeight();

  std::string EngineName() const { return "NuXSecCCQEEngineModifiedZEXP"; }

private:
  NSyst_t MaCCQE;
  NSyst_t AxlFFCCQE;
  NSyst_t SCCVecQE;
  NSyst_t SCCAxlQE;
  NSyst_t PsFF;
  NSyst_t FAxlCCQEAlpha;
  NSyst_t FAxlCCQEGamma;
  NSyst_t FAxlCCQEBeta;
  NSyst_t FAxlCCQETheta;
  // NSyst_t FAZExp_NTerms;
  // NSyst_t FAZExp_TCut;
  // NSyst_t FAZExp_T0;
  // NSyst_t FAZExp_Q4Cut;
  NSyst_t FAZExp_A1;
  NSyst_t FAZExp_A2;
  NSyst_t FAZExp_A3;
  NSyst_t FAZExp_A4;

  Eigen::MatrixXd FAZExp_Basis;
  NSyst_t FAZExp_A1C;
  NSyst_t FAZExp_A2C;
  NSyst_t FAZExp_A3C;
  NSyst_t FAZExp_A4C;
  Eigen::VectorXd a_variations;
};


NuXSecCCQEEngineModifiedZEXP::NuXSecCCQEEngineModifiedZEXP() { // Get the parameter values at generation
                                       // time and store them as the 'default'
  CommonBlockIFace const &cbfa = CommonBlockIFace::Get();

  MaCCQE = RegisterDial("MaCCQE", cbfa.fnemdls_gen.xmaqe, 0.196,0.196);
  AxlFFCCQE = RegisterDial("AxlFFCCQE", cbfa.fnemdls_gen.mdlqeaf);
  DocumentDial(AxlFFCCQE, "Acceptable values: 1. Dipole, 2. BBBA07, 3. 2 Comp, 4. 3 Comp., 5. Z Exp. 6. Corr Z Exp.");
  SCCVecQE = RegisterDial("SCCVecQE", cbfa.fnemdls_gen.sccfv);
  SCCAxlQE = RegisterDial("SCCAxlQE", cbfa.fnemdls_gen.sccfa);
  PsFF = RegisterDial("PsFF", cbfa.fnemdls_gen.fpqe);

  FAxlCCQEAlpha = RegisterDial("FAxlCCQEAlpha", cbfa.fnemdls_gen.axffalpha,0.123,0.123);
  FAxlCCQEGamma = RegisterDial("FAxlCCQEGamma", cbfa.fnemdls_gen.axffgamma,0.121,0.121);
  FAxlCCQEBeta = RegisterDial("FAxlCCQEBeta", cbfa.fnemdls_gen.axffbeta,0.178,0.178);
  FAxlCCQETheta = RegisterDial("FAxlCCQETheta", cbfa.fnemdls_gen.axfftheta,0.031,0.031);

  // Not current used for reweighting, but could be
  // FAZExp_NTerms = RegisterDial("FAZExp_NTerms", cbfa.fnemdls_gen.axzexpnt);
  // FAZExp_TCut = RegisterDial("FAZExp_TCut", cbfa.fnemdls_gen.axzexptc);
  // FAZExp_T0 = RegisterDial("FAZExp_T0", cbfa.fnemdls_gen.axzexpt0);
  // FAZExp_Q4Cut = RegisterDial("FAZExp_Q4Cut", cbfa.fnemdls_gen.axzexpq4);

  FAZExp_A1 = RegisterDial("FAZExp_A1", cbfa.fnemdls_gen.axzexpa1,0.186,0.186);
  FAZExp_A2 = RegisterDial("FAZExp_A2", cbfa.fnemdls_gen.axzexpa2,1.559,1.559);
  FAZExp_A3 = RegisterDial("FAZExp_A3", cbfa.fnemdls_gen.axzexpa3,3.836,3.836);
  FAZExp_A4 = RegisterDial("FAZExp_A4", cbfa.fnemdls_gen.axzexpa4,4.08,4.08);

  FAZExp_A1C = RegisterDial("FAZExp_A1C", 0.0, -1.0, 1.0);
  FAZExp_A2C = RegisterDial("FAZExp_A2C", 0.0, -1.0, 1.0);
  FAZExp_A3C = RegisterDial("FAZExp_A3C", 0.0, -1.0, 1.0);
  FAZExp_A4C = RegisterDial("FAZExp_A4C", 0.0, -1.0, 1.0);


  Eigen::MatrixXd Covar = PRD_93_113015::a_14_errors * PRD_93_113015::Correlation_Matrix * PRD_93_113015::a_14_errors.transpose();
  for (int i = 0; i < 4; i++){
    for (int j = 0; j < 4; j++){
      Covar.col(i)[j] = PRD_93_113015::a_14_errors[i] * PRD_93_113015::Correlation_Matrix.col(i)[j] * PRD_93_113015::a_14_errors[j];
    }
  }
  FAZExp_Basis = GetPCAFromCovarianceMatrix(Covar);
  a_variations = Eigen::VectorXd::Zero(4);

}

void NuXSecCCQEEngineModifiedZEXP::Reconfigure() {
  ApplyTweaks();

  if (fDials[AxlFFCCQE].IsTweaked() &&
      ((fDials[AxlFFCCQE].ToValue > 10) || (fDials[AxlFFCCQE].ToValue < 1))) {
    std::cout << "ERROR: AxlFFCCQE can only go between 1 and 6s\n\t1. "
                 "Dipole\n\t2. BBBA07\n\t3. 2 Comp\n\t4. 3 Comp.\n\t5. Z Exp.\n\t6. Z Exp. Corr"
              << std::endl;
    abort();
  }
}

double NuXSecCCQEEngineModifiedZEXP::CalcWeight() {
  
  if (!NModeDefn::isCCQE(nework_.modene)) {
    return 1;
  }
  if (!AnyTweaked()) {
    return 1;
  }

  CommonBlockIFace const &cbfa = CommonBlockIFace::Get();
  cbfa.ResetGenValues();

  double old_xsec = NEUTGetXSec();

  if (old_xsec == 0) {
    std::cout << "NuXSecCCQEEngineModifiedZEXP::CalcWeight() Warning: old_xsec==0, setting "
                 "weight to 1"
              << std::endl;
    return 1;
  }

  if (fDials[AxlFFCCQE].ToValue != 6){
    nemdls_.mdlqeaf = fDials[AxlFFCCQE].ToValue;
  } else {
    nemdls_.mdlqeaf = 5;
  }

  nemdls_.xmaqe = fDials[MaCCQE].ToValue;

  nemdls_.sccfv = fDials[SCCVecQE].ToValue;
  nemdls_.sccfa = fDials[SCCAxlQE].ToValue;
  nemdls_.fpqe = fDials[PsFF].ToValue;

  nemdls_.axffalpha = fDials[FAxlCCQEAlpha].ToValue;
  nemdls_.axffgamma = fDials[FAxlCCQEGamma].ToValue;
  nemdls_.axfftheta = fDials[FAxlCCQETheta].ToValue;
  nemdls_.axffbeta = fDials[FAxlCCQEBeta].ToValue;

  if (fDials[AxlFFCCQE].ToValue != 6){
    nemdls_.axzexpa1 = fDials[FAZExp_A1].ToValue;
    nemdls_.axzexpa2 = fDials[FAZExp_A2].ToValue;
    nemdls_.axzexpa3 = fDials[FAZExp_A3].ToValue;
    nemdls_.axzexpa4 = fDials[FAZExp_A4].ToValue;
    
  } else {

    std::vector<double> bvariations = {0, 0, 0, 0};
    bvariations[0] = fDials[FAZExp_A1C].ToValue;
    bvariations[1] = fDials[FAZExp_A2C].ToValue;
    bvariations[2] = fDials[FAZExp_A3C].ToValue;
    bvariations[3] = fDials[FAZExp_A4C].ToValue;

    for (size_t i = 0; i < 4; ++i) {
      a_variations += FAZExp_Basis.col(i) * bvariations[i];
    }

    nemdls_.axzexpa1 = cbfa.fnemdls_gen.axzexpa1 * (1.0 + a_variations[0]);
    nemdls_.axzexpa2 = cbfa.fnemdls_gen.axzexpa2 * (1.0 + a_variations[1]);
    nemdls_.axzexpa3 = cbfa.fnemdls_gen.axzexpa3 * (1.0 + a_variations[2]);
    nemdls_.axzexpa4 = cbfa.fnemdls_gen.axzexpa4 * (1.0 + a_variations[3]);

    // std::cout << "TUNINGS " <<  cbfa.fnemdls_gen.axzexpa1 << " " << cbfa.fnemdls_gen.axzexpa2 << " " << cbfa.fnemdls_gen.axzexpa3 << " "  << cbfa.fnemdls_gen.axzexpa4 << std::endl;
    // std::cout << "TUNING2 " <<   nemdls_.axzexpa1 << " " <<  nemdls_.axzexpa2 << " " <<  nemdls_.axzexpa3 << " "  <<  nemdls_.axzexpa4 << std::endl;

  }
  

  NEUTSetParams();

  double new_xsec = NEUTGetXSec();

#ifdef _N_REWEIGHT_CCQE_DEBUG_
  std::cout << "differential cross section (old) = " << old_xsec << std::endl;
  std::cout << "differential cross section (new) = " << new_xsec << std::endl;
#endif

  return CheckReturnWeight(new_xsec / old_xsec);
}

} // namespace rew
} // namespace neut


double Q2_GeV(HepMC3::GenEvent const &ev) {
  auto beamp = NuHepMC::Event::GetBeamParticle(ev);
  if (!beamp) return -1.0;
  auto beam_pid = beamp->pid();
  auto cc_lep_pid = (beam_pid > 0) ? (beam_pid - 1) : (beam_pid + 1);

  auto lep = NuHepMC::Event::GetParticle_First(
      ev, NuHepMC::ParticleStatus::UndecayedPhysical, {cc_lep_pid});

  if (!lep) return -1.0;

  return -(beamp->momentum() - lep->momentum()).m2() * 1E-6;
}



using mylogger = nuis_named_log("mylogger");

int main(int argc, char const *argv[]) {

  EventSourceFactory fact;

  auto [gri, evs] = fact.make(argv[1]);

  nuis::WeightCalcFactory wfact;
  auto wgt = wfact.make(evs, YAML::Load("{neut_cardname: neut.card}"));
  
  auto nwght = wgt->as<NReWeightCalc>();
  nwght->fNEUTRW = std::unique_ptr<neut::rew::NReWeight>(new neut::rew::NReWeight());
  nwght->fNEUTRW->AdoptWeightEngine("CCQE_MOD_ZEXP", std::unique_ptr<neut::rew::NReWeightEngineI>(new neut::rew::NuXSecCCQEEngineModifiedZEXP));
  
  std::map<std::string, double> dial_vals = {
    {"AxlFFCCQE", 6},
    {"FAZExp_A1C", 0.0},
    {"FAZExp_A2C", 0.0},
    {"FAZExp_A3C", 0.0},
    {"FAZExp_A4C", 0.0}
  };

  // wgt->set_parameters(dial_vals);

  std::vector<TH1D*> hists;
  std::vector<TH2D*> responses;
  std::vector<std::string> dials = {"FAZExp_A1C","FAZExp_A2C", "FAZExp_A3C", "FAZExp_A4C"};
  for (auto const &dial_choice : dials) {

    responses.push_back(new TH2D(("dial_res_" + dial_choice).c_str(),"dial_res",21,-1.0,1.0,100,-0.5,10.0));

    for (int i = 0; i < 21; i++) {
      double dial_value = -1.0 + static_cast<float>(i)*2.0/20.0;
      std::cout << "PROC DIAL : " << dial_choice << " " << dial_value << std::endl;

      hists.push_back(
        new TH1D(("hist_" + dial_choice + "_" + std::to_string(dial_value)).c_str(),
        ";Q2:Weighted Dist;", 30, 0.0, 4.0));

      dial_vals[dial_choice] = dial_value;
      // wgt->set_parameters(dial_vals);

      int count = 0;
      auto evcwpair = evs->first();
      for  (auto const &[ev, w] : evs) {
        count += 1;
        if (count % 10000 == 0) std::cout << "COUNT : " << count << std::endl;
        auto proc_id = NuHepMC::ER3::ReadProcessID(ev);
        if ((proc_id >= 200) && (proc_id < 250)) {
          double w = wgt->calc_weight(ev);
          double Q2 = Q2_GeV(ev);
          hists[hists.size()-1]->Fill(Q2, w);
          responses[responses.size()-1]->Fill(dial_value, w);
        }
      }
      break;
      dial_vals[dial_choice] = 0.0;
    }
    break;
  }

  auto f = new TFile("hists_Q2.root","RECREATE");
  for (size_t i = 0; i < hists.size(); i++){
    hists[i]->Write();
  }
  for (size_t i = 0; i < responses.size(); i++){
    responses[i]->Write();
  }
  f->Close();
}
