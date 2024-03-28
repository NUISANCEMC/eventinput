#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "nuis/record/Comparison.h"
#include "nuis/record/hook_types.h"

namespace nuis {

// The point in a table is to map the hist frame associated with a given
// RecordFactory output with the functions needed to project
struct Table {

  Table()
      : blueprint{}, clear{}, select{}, weight{}, projections{}, project{},
        finalize{}, likelihood{} {}

  YAML::Node metadata;
    
  // Here we purposefully keep a full template so that people can change the
  // binning/data on the fly and all others get propagated.

  // This also allows Table.blueprint->add_column() And all spawned objects get
  // the same. Don't love the . -> requirement
  std::shared_ptr<Comparison> blueprint;
  Comparison comparison() { return *blueprint; }

  ClearFunc clear;
  SelectFunc select;
  WeightFunc weight;

  std::vector<ProjectFunc> projections;
  FullProjectFunc project;

  FinalizeFunc finalize;
  LikelihoodFunc likelihood;

  // I also want the ability to do table.add_column for direct blueprint
  // updates.
  template <typename... TS> auto add_column(TS &&...args) {
    return blueprint->add_column(std::forward<TS>(args)...);
  };

  template <typename... TS> auto find_column_index(TS &&...args) {
    return blueprint->find_column_index(std::forward<TS>(args)...);
  };

  ~Table() {}
};
} // namespace nuis
