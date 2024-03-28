#pragma once

#include "nuis/histframe/BinnedValues.h"
#include "nuis/histframe/HistFrame.h"

#include <string>
#include <utility>

namespace nuis {

struct Comparison {

  struct Finalised {
    BinnedValues data;
    BinnedValues estimate;
    Eigen::ArrayXXd correlation;
    YAML::Node metadata;
  };

  // std::string normalisation_type;
  // bool by_bin_width;

  YAML::Node metadata;
  Eigen::ArrayXXd correlation;
  BinnedValues data;
  BinnedValues estimate;
  HistFrame mc;

  Comparison(BinningPtr bindef) {
    data = BinnedValues(bindef, "data");
    mc = HistFrame(bindef, "mc");
  }

  // Redirect most function calls down to mc as data is static
  template <typename... TS> auto add_column(TS &&...args) {
    return mc.add_column(std::forward<TS>(args)...);
  }

  template <typename... TS> auto find_column_index(TS &&...args) {
    return mc.find_column_index(std::forward<TS>(args)...);
  }

  template <typename... TS> void fill(TS &&...args) {
    return mc.fill(std::forward<TS>(args)...);
  }

  template <typename... TS> void fill_if(TS &&...args) {
    return mc.fill_if(std::forward<TS>(args)...);
  }

  template <typename... TS> void fill_column(TS &&...args) {
    return mc.fill_column(std::forward<TS>(args)...);
  }

  template <typename... TS> void fill_column_if(TS &&...args) {
    return mc.fill_column_if(std::forward<TS>(args)...);
  }

  template <typename... TS> auto find_bin(TS &&...args) {
    return mc.find_bin(std::forward<TS>(args)...);
  }

  void reset() { return mc.reset(); }

  template <typename T> auto get_data_column(T &&args) const {
    return data[std::forward<T>(args)];
  }
  template <typename T> auto get_mc_column(T &&args) const {
    return mc[std::forward<T>(args)];
  }

  struct comparison_view {
    BinnedValues::column_view data;
    HistFrame::column_view mc;
  };

  struct comparison_view_const {
    BinnedValues::column_view_const data;
    HistFrame::column_view_const mc;
  };

  template <typename T> auto operator[](T &&args) {
    return comparison_view{data[std::forward<T>(args)],
                           mc[std::forward<T>(args)]};
  }

  template <typename T> auto operator[](T &&args) const {
    return comparison_view_const{data[std::forward<T>(args)],
                                 mc[std::forward<T>(args)]};
  }
};
} // namespace nuis
