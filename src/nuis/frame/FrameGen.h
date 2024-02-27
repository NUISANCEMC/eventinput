#pragma once

#include "nuis/frame/Frame.h"

#include <functional>
#include <numeric>

namespace nuis {

class FrameGen {

public:
  using FilterFunc = std::function<int(HepMC3::GenEvent const &)>;
  using ProjectionFunc = std::function<double(HepMC3::GenEvent const &)>;
  using ProjectionsFunc =
      std::function<std::vector<double>(HepMC3::GenEvent const &)>;

  // PS An option to input a vector of functions is also needed (instead of
  // requiring a lambda to build it) LP Is add_column("name",
  // func).add_column("name", func).add_column("name", func) not okay?

  FrameGen(INormalizedEventSourcePtr evs, size_t block_size = 50000);

  FrameGen filter(FilterFunc filt);
  FrameGen add_columns(std::vector<std::string> col_names,
                       ProjectionsFunc proj);
  FrameGen add_column(std::string col_name, ProjectionFunc proj);
  FrameGen limit(size_t nmax);
  FrameGen progress(size_t every = 100000);

  Frame evaluate();

private:
  INormalizedEventSourcePtr source;

  std::vector<FilterFunc> filters;

  struct HeadedColumnProjectors {
    std::vector<std::string> Head;
    ProjectionsFunc Proj;
  };

  std::vector<HeadedColumnProjectors> projections;

  size_t chunk_size;
  std::vector<Eigen::ArrayXXd> chunks;

  size_t max_events_to_loop;
  size_t counter;
  size_t nevents;

  size_t GetNCols();
};

} // namespace nuis