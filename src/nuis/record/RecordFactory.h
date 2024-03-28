#pragma once

#include "boost/function.hpp"

#include "yaml-cpp/yaml.h"

#include <filesystem>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "nuis/record/IRecord.h"
#include "nuis/record/Table.h"


#include "nuis/log.h"

namespace nuis {
using IRecordPtr = std::shared_ptr<IRecord>;
using TablePtr = std::shared_ptr<Table>;
class RecordFactory : public nuis_named_log("Record") {

  using IRecord_PluginFactory_t = IRecordPtr(YAML::Node const &);
  std::map<std::filesystem::path, boost::function<IRecord_PluginFactory_t>>
      pluginfactories;

public:
  RecordFactory();
  IRecordPtr make(YAML::Node cfg);
  TablePtr make_table(YAML::Node cfg);
};
} // namespace nuis
