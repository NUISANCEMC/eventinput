#include "NReWeightCalc.h"

#include "nuis/weightcalc/plugins/IWeightCalcPlugin.h"

#include "nuis/eventinput/plugins/neutvectEventSource.h"

#include "CommonBlockIFace.h"
#include "NReWeightFactory.h"

#include "boost/dll/alias.hpp"

#include "nuis/log.txx"

#include "yaml-cpp/yaml.h"

#include <filesystem>

namespace nuis {

  double NReWeightCalc::calc_weight(HepMC3::GenEvent const &ev) {
    neut::CommonBlockIFace::ReadVect(nevs->neutvect(ev));
    return fNEUTRW->CalcWeight();
  };

  void NReWeightCalc::set_parameters(std::map<std::string, double> const &params) {
    std::cout << "SETTING PARAMS" << std::endl;
    for (auto &[p, v] : params) {
      std::cout << "SET " << p << " " << v << std::endl;
      fNEUTRW->SetDial_To_Value(fNEUTRW->DialFromString(p), v);
    }
    fNEUTRW->Reconfigure();
  };
  bool NReWeightCalc::good() const { return bool(nevs); }

  NReWeightCalc::NReWeightCalc(IEventSourcePtr evs, YAML::Node const &cfg) {
    nevs = std::dynamic_pointer_cast<neutvectEventSource>(evs);
    if (!nevs) {
      log_warn("NReWeightCalc: Passed in IEventSourcePtr not instance of "
                   "neutvectEventSource.");
      return;
    }
    log_info("Initializing from card");
    log_info(cfg["neut_cardname"].as<std::string>());
std::cout << "LLOADING" << std::endl;
    neut::CommonBlockIFace::Initialize(cfg["neut_cardname"].as<std::string>(),
                                       false);
    log_info("Making RW Instance");

std::cout << "DONE LLOADING" << std::endl;

    fNEUTRW = neut::rew::MakeNReWeightInstance();
std::cout << "MADE RW" << std::endl;

    log_info("Finished");

std::cout << "DONE" << std::endl;


  };

 

  NReWeightCalc::~NReWeightCalc() {}

namespace neut {
static IWeightCalcPluginPtr MakeWeightCalc(IEventSourcePtr evs,
                                                    YAML::Node const &cfg) {
return std::make_shared<NReWeightCalc>(evs, cfg);
}
}

BOOST_DLL_ALIAS(nuis::neut::MakeWeightCalc,
                MakeWeightCalc);

} // namespace nuis
