#pragma once

#include "nuis/histframe/Binning.h"

#include <iostream>

namespace nuis {
std::vector<Binning::BinExtents> unique(std::vector<Binning::BinExtents> bins);
bool bins_overlap(Binning::BinExtents const &a, Binning::BinExtents const &b);
std::vector<Binning::BinExtents>
project_to_unique_bins(std::vector<Binning::BinExtents> const &bins,
                       std::vector<size_t> const &proj_to_axes);
bool binning_has_overlaps(std::vector<Binning::BinExtents> const &bins,
                          std::vector<size_t> const &proj_to_axes = {});
bool binning_has_overlaps(std::vector<Binning::BinExtents> const &bins,
                          size_t proj_to_axis);

std::vector<std::vector<double>>
get_bin_centers(std::vector<Binning::BinExtents> const &bins);
std::vector<double>
get_bin_centers1D(std::vector<Binning::BinExtents> const &bins);
} // namespace nuis