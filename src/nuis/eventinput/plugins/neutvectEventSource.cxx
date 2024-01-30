#include "nuis/eventinput/IEventSource.h"

#include "nvconv.h"
#include "nvfatxtools.h"

#include "HepMC3/GenRunInfo.h"

#include "TChain.h"
#include "TFile.h"

#include "boost/dll/alias.hpp"

#include "spdlog/spdlog.h"

#include "yaml-cpp/yaml.h"

#include <filesystem>
#include <fstream>

namespace nuis {

class neutvectEventSource : public IEventSource {

  std::vector<std::filesystem::path> filepaths;
  std::unique_ptr<TChain> chin;

  std::shared_ptr<HepMC3::GenRunInfo> gri;

  Long64_t ch_ents;
  Long64_t ient;
  TUUID ch_fuid;

  NeutVect *nv;

  void CheckAndAddPath(std::filesystem::path filepath) {
    if (!std::filesystem::exists(filepath)) {
      spdlog::warn("neutvectEventSource ignoring non-existant path {}",
                   filepath.native());
      return;
    }
    std::ifstream fin(filepath);
    char magicbytes[5];
    fin.read(magicbytes, 4);
    magicbytes[4] = '\0';
    if (std::string(magicbytes) != "root") {
      spdlog::warn(
          "neutvectEventSource ignoring non-root file {} (magicbytes: {})",
          filepath.native(), magicbytes);
      return;
    }
    filepaths.push_back(std::move(filepath));
  }

public:
  neutvectEventSource(YAML::Node const &cfg) {
    if (cfg["filepath"]) {
      CheckAndAddPath(cfg["filepath"].as<std::string>());
    } else if (cfg["filepaths"]) {
      for (auto fp : cfg["filepaths"].as<std::vector<std::string>>()) {
        CheckAndAddPath(fp);
      }
    }
  };

  std::optional<HepMC3::GenEvent> first() {

    if (!filepaths.size()) {
      return std::optional<HepMC3::GenEvent>();
    }

    chin = std::make_unique<TChain>("neuttree");

    for (auto const &ftr : filepaths) {
      if (!chin->Add(ftr.c_str(), 0)) {
        spdlog::warn("Could not find neuttree in {}", ftr.native());
        chin.reset();
        return std::optional<HepMC3::GenEvent>();
      }
    }

    ch_ents = chin->GetEntries();
    nv = nullptr;
    auto branch_status = chin->SetBranchAddress("vectorbranch", &nv);
    chin->GetEntry(0);
    int beam_pid = nv->PartInfo(0)->fPID;
    double flux_energy_to_MeV = 1E3;

    double fatx = 0;

    bool isMonoE = nvconv::isMono(*chin, nv);

    std::unique_ptr<TH1> flux_hist(nullptr);

    if (isMonoE) {
      chin->GetEntry(0);
      fatx = nv->Totcrs * 1E-2;
    } else {

      auto frpair = nvconv::GetFluxRateHistPairFromChain(*chin);
      if (frpair.second) {
        fatx = 1E-2 * (frpair.first->Integral() / frpair.second->Integral());
        flux_hist = std::move(frpair.second);
      } else {
        spdlog::warn("Couldn't get nvconv::GetFluxRateHistPairFromChain");
        abort();
      }
    }

    gri = nvconv::BuildRunInfo(ch_ents, fatx, flux_hist, isMonoE, beam_pid,
                               flux_energy_to_MeV);

    ch_fuid = chin->GetFile()->GetUUID();
    ient = 0;
    auto ge = nvconv::ToGenEvent(nv, gri);
    ge.set_event_number(ient);
    return ge;
  }

  std::optional<HepMC3::GenEvent> next() {
    ient++;

    if (ient >= ch_ents) {
      return std::optional<HepMC3::GenEvent>();
    }

    chin->GetEntry(ient);

    if (chin->GetFile()->GetUUID() != ch_fuid) {
      ch_fuid = chin->GetFile()->GetUUID();
    }

    auto ge = nvconv::ToGenEvent(nv, gri);
    ge.set_event_number(ient);
    return ge;
  }

  static IEventSourcePtr MakeEventSource(YAML::Node const &cfg) {
    return std::make_shared<neutvectEventSource>(cfg);
  }

  virtual ~neutvectEventSource() {}
};

BOOST_DLL_ALIAS(nuis::neutvectEventSource::MakeEventSource, MakeEventSource);

} // namespace nuis
