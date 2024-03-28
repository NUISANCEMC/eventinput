#pragma once

#include "nuis/record/Comparison.h"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace nuis {

using ClearFunc = std::function<void(Comparison &)>;

using ProjectFunc = std::function<double(HepMC3::GenEvent const &)>;

using FullProjectFunc =
    std::function<std::vector<double>(HepMC3::GenEvent const &)>;

using WeightFunc = std::function<double(HepMC3::GenEvent const &)>;

using SelectFunc = std::function<int(HepMC3::GenEvent const &)>;

using FinalizeFunc =
    std::function<BinnedValues(Comparison &, const double)>;

using LikelihoodFunc = std::function<double(Comparison const &)>;
} // namespace nuis