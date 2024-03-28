#include "nuis/record/RecordFactory.h"

#include "boost/dll/import.hpp"
#include "boost/dll/runtime_symbol_info.hpp"

#include "yaml-cpp/yaml.h"

#include "fmt/core.h"

#include "nuis/log.txx"

#include <regex>
#include <string>

namespace nuis {

RecordFactory::RecordFactory() {
  auto NUISANCE = std::getenv("NUISANCE3_ROOT");

  if (!NUISANCE) {
    log_critical("NUISANCE_ROOT environment variable not defined");
    throw NUISANCE_ROOTUndefined();
  }

  std::filesystem::path shared_library_dir{NUISANCE};
  shared_library_dir /= "lib/plugins";
  std::regex plugin_re("nuisplugin-record-.*.so");
  for (auto const &dir_entry :
       std::filesystem::directory_iterator{shared_library_dir}) {
    if (std::regex_match(dir_entry.path().filename().native(), plugin_re)) {
      log_debug("Found record plugin: {}", dir_entry.path().native());
      pluginfactories.emplace(dir_entry.path(),
                              boost::dll::import_alias<IRecord_PluginFactory_t>(
                                  dir_entry.path().native(), "Make"));
    }
  }
}

IRecordPtr RecordFactory::make(YAML::Node cfg) {

  std::string record_type =
      "nuisplugin-record-" + cfg["type"].as<std::string>() + ".so";

  for (auto &[pluginso, plugin] : pluginfactories) {
    std::string fullpath = std::string(pluginso);
    if (fullpath.find(record_type) != std::string::npos) {
      auto es = plugin(cfg);
      return es;
    }
  }
  return nullptr;
}

TablePtr RecordFactory::make_table(YAML::Node cfg){
  auto record = make(cfg);
  return record->table(cfg);
}

} // namespace nuis
