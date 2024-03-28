#pragma once

#include "nuis/record/Comparison.h"
#include <vector>

namespace nuis {

NEW_NUISANCE_EXCEPT(EmptyData);
NEW_NUISANCE_EXCEPT(EmptyMC);

namespace finalize {
// Fix these for full covariance estimates
BinnedValues FATXNormalizedByBinWidth(Comparison &fr,
                                      const double fatx) {
  auto mc = fr.mc.finalise(true);
  mc.values /= fatx;
  mc.errors /= fatx;
  fr.estimate = mc;
  return mc;
}

BinnedValues FATXNormalized(Comparison &fr,
                                      const double fatx) {
  auto mc = fr.mc.finalise(false);
  mc.values /= fatx;
  mc.errors /= fatx;

  fr.estimate = mc;
  return mc;
}


BinnedValues EventRateScaleToData(Comparison &fr,
                                  const double /*fatx_by_pdf*/) {

  auto datamc = fr[0];
  double dt_sum = datamc.data.value.sum();
  if (dt_sum == 0.0)
    throw EmptyData();

  // need to know here if we want to divide by fr.mc.binning->bin_sizes();
  double mc_sum = datamc.mc.count.sum();
  if (mc_sum == 0.0)
    throw EmptyMC();

  auto mc = fr.mc.finalise(false);
  double div = mc[0].value.sum(); // previous approach gave a factor of two out
  mc.values *= dt_sum / div;
  mc.errors *= dt_sum / div;

  fr.estimate = mc;
  return mc; 
}

BinnedValues FluxUnfoldedScaling(Comparison &fr,
  const double /*fatx_by_pdf*/,
  std::vector<double> /*flux_centers*/,
  std::vector<double> /*flux_content*/) {

  // Placeholder for flux unfolded scaling stuff....
  return fr.mc.finalise(false);
}

// FluxUnfoldedScaling is an exceptional lambda case

} // namespace finalize
} // namespace nuis