#pragma once

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"

#include "nuis/python/pyYAML.h"

// These have to be in every TU before any of our binding starts happening or
// you get super weird runtime crashes. Include this header in all pybind11
// NUISANCE TUs.
PYBIND11_MAKE_OPAQUE(std::vector<bool>);
PYBIND11_MAKE_OPAQUE(std::vector<int>);
PYBIND11_MAKE_OPAQUE(std::vector<double>);
PYBIND11_MAKE_OPAQUE(std::vector<uint32_t>);