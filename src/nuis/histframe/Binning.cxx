#include "nuis/histframe/Binning.h"
#include "nuis/histframe/BinningUtility.h"

#include "nuis/log.txx"

#include "fmt/ranges.h"

#include <cmath>

namespace nuis {

std::vector<Binning::BinExtents>
binning_product_recursive(std::vector<Binning>::const_reverse_iterator from,
                          std::vector<Binning>::const_reverse_iterator to) {

  if (from == to) {
    return {};
  }

  auto lower_extents = binning_product_recursive(from + 1, to);

  std::vector<Binning::BinExtents> bins;
  if (lower_extents.size()) {
    for (auto bin : from->bins) {
      for (auto lbin : lower_extents) {
        bins.emplace_back();
        std::copy(lbin.begin(), lbin.end(), std::back_inserter(bins.back()));
        std::copy(bin.begin(), bin.end(), std::back_inserter(bins.back()));
      }
    }
  } else {
    return from->bins;
  }

  return bins;
}

Binning::Index Binning::operator()(std::vector<double> const &x) const {
  return find_bin(x);
}
Binning::Index Binning::operator()(double x) const {
  static std::vector<double> vect = {0};
  vect[0] = x;
  return find_bin(vect);
}

Eigen::ArrayXd Binning::bin_sizes() const {
  Eigen::ArrayXd bin_sizes = Eigen::ArrayXd::Zero(bins.size());
  size_t i = 0;
  for (auto const &bin : bins) {
    bin_sizes[i] = 1;
    for (auto const &ext : bin) {
      bin_sizes[i] *= ext.width();
    }
    i++;
  }
  return bin_sizes;
}

Binning Binning::lin_space(double min, double max, size_t nbins,
                           std::string const &label) {

  if (min >= max) {
    log_critical("lin_space({0},{1},{2}) is invalid as min={0} >= max={1}.",
                 min, max, nbins);
    throw BinningNotIncreasing();
  }

  double step = (max - min) / double(nbins);

  Binning bin_info;
  bin_info.axis_labels.push_back(label);

  for (size_t i = 0; i < nbins; ++i) {
    bin_info.bins.emplace_back();
    bin_info.bins.back().emplace_back(min + (i * step), min + ((i + 1) * step));
  }

  bin_info.find_bin = [=](std::vector<double> const &x) -> Index {
    NUIS_LOG_TRACE("[lin_space({},{},{}).find_bin] x.size() = {}, x[0] = {}",
                   min, max, nbins, x.size(), x.size() ? x[0] : 0xdeadbeef);
    if (x.size() < 1) {
      log_warn("[lin_space({},{},{}).find_bin] was passed an empty projection "
               "vector. Returning npos. Compile with "
               "CMAKE_BUILD_TYPE=Debug to make this an exception.",
               min, max, nbins);
#ifndef NUIS_NDEBUG
      throw TooFewProjectionsForBinning();
#endif
      return npos;
    }

    if ((x[0] != 0) && !std::isnormal(x[0])) {
      log_warn("[lin_space({},{},{}).find_bin] was passed an "
               "abnornmal number = {}. Returning npos. Compile with "
               "CMAKE_BUILD_TYPE=Debug to make this an exception.",
               min, max, nbins, x[0]);
#ifndef NUIS_NDEBUG
      throw UnbinnableNumber();
#endif
      return Binning::npos;
    }

    Index bin = (x[0] >= max)  ? npos
                : (x[0] < min) ? npos
                               : std::floor((x[0] - min) / step);
    NUIS_LOG_TRACE("[lin_space({},{},{}).find_bin] Found bin: {}{}", min, max,
                   nbins, bin == npos ? "npos" : std::to_string(bin),
                   bin_info.bins.size() > bin
                       ? fmt::format(", ({} -- {})", bin_info.bins[bin][0].min,
                                     bin_info.bins[bin][0].max)
                       : "");
    return bin;
  };
  return bin_info;
}

Binning
Binning::lin_spaceND(std::vector<std::tuple<double, double, size_t>> axes,
                     std::vector<std::string> labels) {

  size_t nax = axes.size();
  std::vector<size_t> nbins_in_slice = {1};
  std::vector<double> steps;

  size_t nbins = 1;
  for (size_t ax_i = 0; ax_i < nax; ++ax_i) {

    auto const &ax_min = std::get<0>(axes[ax_i]);
    auto const &ax_max = std::get<1>(axes[ax_i]);
    auto const &ax_nbins = std::get<2>(axes[ax_i]);

    if (ax_min >= ax_max) {
      log_critical(
          "lin_spaceND({}) is invalid along axis {} as min={} >= max={}.", axes,
          ax_i, ax_min, ax_max);
      throw BinningNotIncreasing();
    }

    steps.push_back((ax_max - ax_min) / double(ax_nbins));
    nbins *= ax_nbins;
    nbins_in_slice.push_back(nbins);
  }

  Binning bin_info;
  for (size_t i = 0; i < nax; ++i) {
    bin_info.axis_labels.push_back(labels.size() > i ? labels[i] : "");
  }

  std::vector<Index> dimbins(nax, 0);

  for (size_t bin_i = 0; bin_i < nbins; ++bin_i) {
    bin_info.bins.emplace_back(nax, SingleExtent{});

    size_t bin_remainder = bin_i;
    for (int ax_i = int(nax - 1); ax_i >= 0; --ax_i) {

      auto const &ax_min = std::get<0>(axes[ax_i]);

      dimbins[ax_i] = (bin_remainder / nbins_in_slice[ax_i]);
      bin_remainder = bin_remainder % nbins_in_slice[ax_i];

      bin_info.bins.back()[ax_i] = {ax_min + (dimbins[ax_i] * steps[ax_i]),
                                    ax_min +
                                        ((dimbins[ax_i] + 1) * steps[ax_i])};
    }
  }

  bin_info.find_bin = [=](std::vector<double> const &x) -> Index {
    NUIS_LOG_TRACE("[lin_spaceND({}).find_bin] x.size() = {}/{} axes, x = {}",
                   axes, x.size(), nax, x);

    if (x.size() < nax) {
      log_warn("[lin_spaceND({}).find_bin] was passed too few projections. "
               "x.size() = "
               "{} < nax = {}. Returning npos. Compile with "
               "CMAKE_BUILD_TYPE=Debug to make this an exception.",
               axes, x.size(), nax);
#ifndef NUIS_NDEBUG
      throw TooFewProjectionsForBinning();
#endif
      return npos;
    }

    Index gbin = 0;
    for (size_t ax_i = 0; ax_i < nax; ++ax_i) {

      if ((x[ax_i] != 0) && !std::isnormal(x[ax_i])) {
        log_warn(
            "[lin_spaceND({}).find_bin] was passed an "
            "abnornmal number = {} on axis {}. Returning npos. Compile with "
            "CMAKE_BUILD_TYPE=Debug to make this an exception.",
            axes, x[ax_i], ax_i);
#ifndef NUIS_NDEBUG
        throw UnbinnableNumber();
#endif
        return Binning::npos;
      }

      Index dimbin =
          (x[ax_i] >= std::get<1>(axes[ax_i])) ? npos
          : (x[ax_i] < std::get<0>(axes[ax_i]))
              ? npos
              : std::floor((x[ax_i] - std::get<0>(axes[ax_i])) / steps[ax_i]);

      NUIS_LOG_TRACE("[lin_spaceND({}).find_bin] Found bin[{}]: {}{}", axes,
                     ax_i, dimbin == npos ? "npos" : std::to_string(dimbin),
                     bin_info.bins.size() > dimbin
                         ? fmt::format(", ({} -- {})",
                                       bin_info.bins[dimbin][ax_i].min,
                                       bin_info.bins[dimbin][ax_i].max)
                         : "");

      gbin += dimbin * nbins_in_slice[ax_i];
      NUIS_LOG_TRACE("[lin_spaceND({}).find_bin] gbin after {} axes: {}", axes,
                     ax_i, gbin == npos ? "npos" : std::to_string(gbin));

      if (dimbin == npos) {
        NUIS_LOG_TRACE("[lin_spaceND({}).find_bin] out of range on axis {}. "
                       "Returning npos.",
                       axes, ax_i);
        return npos;
      }
    }

    NUIS_LOG_TRACE("[lin_spaceND({}).find_bin] returning gbin {} for x = {}",
                   axes, gbin == npos ? "npos" : std::to_string(gbin), x);

    return gbin;
  };
  return bin_info;
}

template <int base>
Binning log_space_impl(double min, double max, size_t nbins,
                       std::string const &label) {
  if (min <= 0) {
    log_critical("log{}_space({},{},{}) is invalid as min <= 0.",
                 base == 0 ? "" : std::to_string(base), min, max, nbins);
    throw InvalidBinEdgForLogarithmicBinning();
  }

  if (min >= max) {
    log_critical("log{0}_space({1},{2},{3}) is invalid as min={1} >= max={2}.",
                 base == 0 ? "" : std::to_string(base), min, max, nbins);
    throw BinningNotIncreasing();
  }

  auto logbase = [=](double v) -> double {
    return (base == 0) ? std::log(v) : (std::log(v) / std::log(base));
  };
  auto expbase = [=](double v) -> double {
    return (base == 0) ? std::exp(v) : std::exp(v * std::log(base));
  };

  auto minl = logbase(min);
  auto maxl = logbase(max);

  double step = (maxl - minl) / double(nbins);

  Binning bin_info;
  bin_info.axis_labels.push_back(label);

  for (size_t i = 0; i < nbins; ++i) {
    bin_info.bins.emplace_back();

    auto low = expbase(minl + (i * step));
    auto high = expbase(minl + ((i + 1) * step));

    bin_info.bins.back().emplace_back(low, high);
  }

  bin_info.find_bin = [=](std::vector<double> const &x) -> Binning::Index {
    NUIS_LOGGER_TRACE(
        "Binning", "[log{}_space({},{},{}).find_bin] x.size() = {}, x[0] = {}",
        base == 0 ? "" : std::to_string(base), min, max, nbins, x.size(),
        x.size() ? x[0] : 0xdeadbeef);
    if (x.size() < 1) {
      Binning::log_warn(
          "[log{0}_space({},{},{}).find_bin] was passed an empty projection "
          "vector. Returning npos. Compile with "
          "CMAKE_BUILD_TYPE=Debug to make this an exception.",
          base == 0 ? "" : std::to_string(base), min, max, nbins);
#ifndef NUIS_NDEBUG
      throw TooFewProjectionsForBinning();
#endif
      return Binning::npos;
    }

    if (!std::isnormal(x[0])) {
      Binning::log_warn("[log{0}_space({},{},{}).find_bin] was passed an "
                        "abnornmal number = {}. Returning npos. Compile with "
                        "CMAKE_BUILD_TYPE=Debug to make this an exception.",
                        base == 0 ? "" : std::to_string(base), min, max, nbins,
                        x[0]);
#ifndef NUIS_NDEBUG
      throw UnbinnableNumber();
#endif
      return Binning::npos;
    }

    if (x[0] < 0) {
      Binning::log_warn("[log{0}_space({},{},{}).find_bin] was passed an "
                        "unloggable number = {}. Returning npos. Compile with "
                        "CMAKE_BUILD_TYPE=Debug to make this an exception.",
                        base == 0 ? "" : std::to_string(base), min, max, nbins,
                        x[0]);
#ifndef NUIS_NDEBUG
      throw UnbinnableNumber();
#endif
      return Binning::npos;
    }

    auto xl = logbase(x[0]);

    return (xl >= maxl)  ? Binning::npos
           : (xl < minl) ? Binning::npos
                         : std::floor((xl - minl) / step);
  };
  return bin_info;
}

Binning Binning::log10_space(double min, double max, size_t nbins,
                             std::string const &label) {
  return log_space_impl<10>(min, max, nbins, label);
}

Binning Binning::log_space(double min, double max, size_t nbins,
                           std::string const &label) {
  return log_space_impl<0>(min, max, nbins, label);
}

Binning Binning::contiguous(std::vector<double> const &edges,
                            std::string const &label) {

  Binning bin_info;

  for (size_t i = 1; i < edges.size(); ++i) {
    if (edges[i] <= edges[i - 1]) {
      log_critical("[contiguous]: Bin edges are not unique and monotonically "
                   "increasing. edge[{}] = {}, edge[{}] = {}.",
                   i, edges[i], i - 1, edges[i - 1]);
      throw BinningUnsorted();
    }
    bin_info.bins.emplace_back();
    bin_info.bins.back().push_back({edges[i - 1], edges[i]});
  }

  bin_info.axis_labels.push_back(label);

  // binary search for bin
  bin_info.find_bin = [=](std::vector<double> const &x) -> Index {
    if (x.size() < 1) {
      log_warn("[contiguous.find_bin] was passed an empty projection "
               "vector. Returning npos. Compile with "
               "CMAKE_BUILD_TYPE=Debug to make this an exception.");
#ifndef NUIS_NDEBUG
      throw TooFewProjectionsForBinning();
#endif
      return npos;
    }

    if ((x[0] != 0) && !std::isnormal(x[0])) {
      log_warn("[contiguous.find_bin] was passed an "
               "abnornmal number = {}. Returning npos. Compile with "
               "CMAKE_BUILD_TYPE=Debug to make this an exception.",
               x[0]);
#ifndef NUIS_NDEBUG
      throw UnbinnableNumber();
#endif
      return Binning::npos;
    }

    if (x[0] < bin_info.bins.front()[0].min) {
      NUIS_LOG_TRACE("[contiguous.find_bin] x = {} < binning low edge = {}, "
                     "returning npos.",
                     x[0], bin_info.bins.front()[0].min);
      return npos;
    }
    if (x[0] >= bin_info.bins.back()[0].max) {
      NUIS_LOG_TRACE("[contiguous.find_bin] x = {} >= binning high edge = {}, "
                     "returning npos.",
                     x[0], bin_info.bins.back()[0].max);
      return npos;
    }

    size_t L = 0;
    size_t R = bin_info.bins.size() - 1;
    NUIS_LOG_TRACE("[contiguous.find_bin]: begin binary search: {{ x: {}, L: "
                   "{}, R: {} }}.",
                   x[0], L, R);
    while (L <= R) {
      size_t m = std::floor((L + R) / 2);
      NUIS_LOG_TRACE("[contiguous.find_bin]: checking {{ x: {}, m: {}, min: "
                     "{}, max: {}}}.",
                     x[0], m, bin_info.bins[m][0].min, bin_info.bins[m][0].max);
      if ((bin_info.bins[m][0].min <= x[0]) &&
          (bin_info.bins[m][0].max > x[0])) {
        NUIS_LOG_TRACE("[contiguous.find_bin]: binary search succeeded, in "
                       "bin: {} < {} < {}",
                       bin_info.bins[m][0].min, bin_info.bins[m][0].max, x[0]);
        return m;
      } else if (bin_info.bins[m][0].max <= x[0]) {
        L = m + 1;
        NUIS_LOG_TRACE("  -- {} < {}: L = {}", bin_info.bins[m][0].max, x[0],
                       L);
      } else if (bin_info.bins[m][0].min > x[0]) {
        R = m - 1;
        NUIS_LOG_TRACE("  -- {} > {}: R = {}", bin_info.bins[m][0].min, x[0],
                       R);
      }
    }
    NUIS_LOG_TRACE(
        "[contiguous.find_bin]: binary search failed, returning npos.");
    return npos;
  };
  return bin_info;
}

struct from_extentsHelper {

  std::vector<Binning::BinExtents> bins;
  std::vector<std::pair<Binning::Index, Binning::BinExtents>> sorted_bins;

  from_extentsHelper(std::vector<Binning::BinExtents> const &bi) : bins(bi) {

    for (Binning::Index bi_it = 0; bi_it < Binning::Index(bins.size());
         ++bi_it) {
      sorted_bins.emplace_back(bi_it, bins[bi_it]);
    }
    std::stable_sort(
        sorted_bins.begin(), sorted_bins.end(),
        [](std::pair<Binning::Index, Binning::BinExtents> const &a,
           std::pair<Binning::Index, Binning::BinExtents> const &b) {
          return a.second < b.second;
        });
  }

  std::string tostr() const {
    std::stringstream ss;
    ss << "bins : " << bins;
    ss << "sorted_bins: [" << std::endl;

    size_t id = 0;
    for (auto const &a : sorted_bins) {
      ss << "  " << id++ << ": { Index: " << a.first << ", extent: " << a.second
         << " }." << std::endl;
    }

    ss << "]" << std::endl;
    return ss.str();
  }

  std::pair<size_t, size_t> get_axis_bin_range(std::vector<double> const &x,
                                               size_t from, size_t to,
                                               size_t ax) const {
    NUIS_LOG_TRACE("[get_axis_bin_range]: x = {}, from = {}, to "
                   "= {}, ax = {}",
                   x[ax], from, to, ax);

    if (from == Binning::npos) {
      return std::pair<size_t, size_t>{Binning::npos, Binning::npos};
    }

    if (sorted_bins[from].second[ax].min >
        x[ax]) { // below any bins in this slice
      NUIS_LOG_TRACE("[get_axis_bin_range]: x = {} < "
                     "bins[from][ax].min = {}",
                     x[ax], sorted_bins[from].second[ax].min);
      return std::pair<size_t, size_t>{Binning::npos, Binning::npos};
    }

    auto stop = std::min(size_t(sorted_bins.size()), to);

    if (sorted_bins[stop - 1].second[ax].max <=
        x[ax]) { // above any bins in this slice
      NUIS_LOG_TRACE("[get_axis_bin_range]: x = {} < bins[stop - "
                     "1][ax].max = {}",
                     x[ax], sorted_bins[stop - 1].second[ax].max);
      return std::pair<size_t, size_t>{Binning::npos, Binning::npos};
    }

    size_t from_this_ax = Binning::npos;
    size_t to_this_ax = stop;
    for (size_t bi_it = from; bi_it < stop; ++bi_it) {
      if (sorted_bins[bi_it].second[ax].contains(x[ax])) {
        if (from_this_ax == Binning::npos) {
#if (NUIS_ACTIVE_LEVEL <= NUIS_LEVEL_TRACE)
          std::stringstream ss;
          ss << sorted_bins[bi_it].second;
          NUIS_LOG_TRACE("[get_axis_bin_range]: first bin in ax[{}]: {} = {}",
                         ax, bi_it, ss.str());
#endif
          from_this_ax = bi_it;
        }
#if (NUIS_ACTIVE_LEVEL <= NUIS_LEVEL_TRACE)
        else {
          std::stringstream ss;
          ss << sorted_bins[bi_it].second;
          NUIS_LOG_TRACE("    {} bin in ax[{}]: {} = {}",
                         (bi_it - from_this_ax), ax, bi_it, ss.str());
        }
#endif
      } else if (from_this_ax != Binning::npos) {
        to_this_ax = bi_it;
        break;
      }
    }
    // if (to_this_ax < stop) {
    //   std::stringstream ss;
    //   ss << sorted_bins[to_this_ax].second;
    //   NUIS_LOG_TRACE("[get_axis_bin_range]: first bin not in ax[{}]: {} =
    //   {}", ax,
    //                to_this_ax, ss.str());
    // } else {
    //   NUIS_LOG_TRACE("[get_axis_bin_range]: end of range for ax[{}] = {} >=
    //   {},
    //   "
    //                "the size of the binning array.",
    //                ax, to_this_ax, stop);
    // }

    return (ax == 0) ? std::pair<size_t, size_t>{from_this_ax, to_this_ax}
                     : get_axis_bin_range(x, from_this_ax, to_this_ax, ax - 1);
  }

  Binning::Index operator()(std::vector<double> const &x) const {
    // NUIS_LOG_TRACE("[from_extentsHelper]: x = {}", x);
    if (x.size() < sorted_bins.front().second.size()) {
      log_critical("[from_extentsHelper]: projections passed in: {} is "
                   "smaller than the number of axes in a bin: {}",
                   x, sorted_bins.front().second.size());
      throw MismatchedAxisCount();
    }

    auto contains_range = get_axis_bin_range(
        x, 0, sorted_bins.size(), sorted_bins.front().second.size() - 1);
    if ((contains_range.first == Binning::npos) ||
        (contains_range.second == Binning::npos)) {
      NUIS_LOG_TRACE(
          "[from_extentsHelper]: Search yielded npos. Returning npos.\n{}",
          tostr());
      return Binning::npos;
    }

    if ((contains_range.second - contains_range.first) != 1) {
      log_critical("[from_extentsHelper]: When searching for bin, failed to "
                   "find a unique bin. Either this is a bug in "
                   "NUISANCE or the binning is not unique.");
      std::stringstream ss;
      ss << "REPORT INFO:\n>>>----------------------------\ninput "
            "bins: \n"
         << bins << "\n";
      log_critical(ss.str());
      log_critical("searching for x: {}", x);
      log_critical("get_axis_bin_range: {}", contains_range);
      throw CatastrophicBinningFailure();
    }
    // return the original Index
    NUIS_LOG_TRACE("[from_extentsHelper]: Found original bin: {}\n",
                   sorted_bins[contains_range.first].first);
    return sorted_bins[contains_range.first].first;
  }
};

Binning Binning::from_extents(std::vector<BinExtents> bins,
                              std::vector<std::string> const &labels) {

  Binning bin_info;
  bin_info.axis_labels = labels;
  for (size_t i = bin_info.axis_labels.size(); i < bins.front().size(); ++i) {
    bin_info.axis_labels.push_back("");
  }
  bin_info.bins = bins;

  auto const &sorted_unique_bins = unique(bins);

  if (bin_info.bins.size() != sorted_unique_bins.size()) {
    log_critical("[from_extents]: When building Binning from vector of "
                 "BinExtents, the list of unique bins was {} long, while "
                 "the original list was {}. Binnings must be unique.");
    std::stringstream ss("");
    ss << bins;
    log_critical("Bins: {}", ss.str());
    throw BinningNotUnique();
  }

  if (binning_has_overlaps(bin_info.bins)) {
    log_critical("[from_extents]: When building Binning from vector of "
                 "BinExtents, the list of bins appears to contain "
                 "overlaps. Binnings must be non-overlapping.");
    std::stringstream ss("");
    ss << bins;
    log_critical("Bins: {}", ss.str());
    throw BinningHasOverlaps();
  }

  from_extentsHelper bin_finder(bins);

  bin_info.find_bin = [bin_finder](std::vector<double> const &x) -> Index {
    return bin_finder(x);
  };
  return bin_info;
}

Binning Binning::product(std::vector<Binning> const &binnings) {

  Binning bin_info_product;

  size_t nax = 0;
  size_t nbins = 1;
  std::vector<size_t> nbins_in_op_slice = {1};
  std::vector<size_t> nax_in_op = {};

  for (auto const &bin_info : binnings) {
    auto ndims_in_op = bin_info.bins.front().size();
    nax += ndims_in_op;
    nax_in_op.push_back(ndims_in_op);

    auto nbins_in_op = bin_info.bins.size();
    nbins *= nbins_in_op;
    nbins_in_op_slice.push_back(nbins);

    // combining all bin_info labels
    std::copy(bin_info.axis_labels.begin(), bin_info.axis_labels.end(),
              std::back_inserter(bin_info_product.axis_labels));
  }
  bin_info_product.bins =
      binning_product_recursive(binnings.rbegin(), binnings.rend());

  bin_info_product.find_bin = [=](std::vector<double> const &x) -> Index {
    if (x.size() < nax) {
      return npos;
    }

    Index gbin = 0;
    size_t nax_consumed = 0;
    for (size_t op_it = 0; op_it < nax_in_op.size(); ++op_it) {

      Index op_bin = binnings[op_it].find_bin(
          std::vector<double>(x.begin() + nax_consumed,
                              x.begin() + nax_consumed + nax_in_op[op_it]));

      gbin += op_bin * nbins_in_op_slice[op_it];
      nax_consumed += nax_in_op[op_it];

      if (op_bin == npos) {
        return npos;
      }
    }

    return gbin;
  };
  return bin_info_product;
}

bool operator<(Binning::BinExtents const &a, Binning::BinExtents const &b) {
  if (a.size() != b.size()) {
    log_critical(
        "[operator<(Binning::BinExtents const &a, Binning::BinExtents const "
        "&b)]: Tried to sort multi-dimensional binning with "
        "bins of unequal dimensionality: {} != {}",
        a.size(), b.size());
    throw MismatchedAxisCount();
  }
  for (size_t i = a.size(); i > 0; i--) {
    if (!(a[i - 1] == b[i - 1])) {
      return a[i - 1] < b[i - 1];
    }
  }
  // if we've got here the bin is identical in all dimensions
  return false;
}

bool operator==(Binning::SingleExtent const &a,
                Binning::SingleExtent const &b) {
  return (a.min == b.min) && (a.max == b.max);
}
bool operator<(Binning::SingleExtent const &a, Binning::SingleExtent const &b) {
  return (a.min != b.min) ? (a.min < b.min) : (a.max < b.max);
}

} // namespace nuis

std::ostream &operator<<(std::ostream &os,
                         nuis::Binning::SingleExtent const &sext) {
  return os << fmt::format("({:.2f} - {:.2f})", sext.min, sext.max);
}
std::ostream &operator<<(std::ostream &os,
                         nuis::Binning::BinExtents const &bext) {
  os << "[";
  for (size_t j = 0; j < bext.size(); ++j) {
    os << bext[j] << ((j + 1) == bext.size() ? "]" : ", ");
  }
  return os;
}
std::ostream &operator<<(std::ostream &os,
                         std::vector<nuis::Binning::BinExtents> const &bins) {
  os << "[" << std::endl;
  for (size_t i = 0; i < bins.size(); ++i) {
    os << "  " << i << ": " << bins[i] << std::endl;
  }
  return os << "]" << std::endl;
}

std::ostream &operator<<(std::ostream &os, nuis::Binning const &bi) {
  os << fmt::format("Axis lables: {}\nBins: ", bi.axis_labels);
  return os << bi.bins;
}