// Copyright 2016-2021 L. Pickering, P Stowell, R. Terri, C. Wilkinson, C. Wret

/*******************************************************************************
 *    This file is part of NUISANCE.
 *
 *    NUISANCE is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    NUISANCE is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with NUISANCE.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/
#ifndef NUISANCE_VARIABLES_H
#define NUISANCE_VARIABLES_H

#include <vector>
#include <string>
#include <map>
#include <iostream>

#include "yaml-cpp/yaml.h"

using namespace YAML;

namespace nuis {

// Keeping old structur for now but should
// be updated to better match HISTFrame structure
// when finally agreed on.
struct Variables {
  std::vector<double> values;
  std::vector<double> low;
  std::vector<double> high;
  std::vector<double> errors;

  std::map<std::string, std::string> qualifiers;

  bool valid = 0;
  std::string name;
  std::string units;
  std::string title;
  size_t n = 0;

  inline std::string summary() {
    std::stringstream ss;

    ss << "Variables " << std::endl;
    ss << " - Name : " << name << std::endl;
    ss << " - Title : " << title << std::endl;
    ss << " - Units : " << units << std::endl;
    ss << " - Qualifiers : V " << std::endl;
    ss << " - - " << std::endl;

    ss << " - NBins : " << n << std::endl;
    ss << " - Bins : " << std::endl;
    ss << " - - i : value low high " << std::endl;

    for (size_t i = 0; i < n; i++) {
      ss << " - - " << i << " : ";

      if (i < values.size() && values[i] != -999)
        ss << values[i] << " ";
      else
        ss << "### ";

      if (i < low.size() && low[i] != -999)
        ss << low[i] << " ";
      else
        ss << "### ";

      if (i < high.size() && high[i] != -999)
        ss << high[i] << " ";
      else
        ss << "### ";

      ss << std::endl;
    }
    return ss.str();
  }

  YAML::Node yaml_template() {
    YAML::Node n;
    n["header"] = YAML::Node();
    n["header"]["name"] = "hepdata_name";
    n["header"]["units"] = "none";
    n["values"] = YAML::Node();
    return n;
  }

  inline Variables() {}

  explicit inline Variables(YAML::Node node){
    Variables& rhs = (*this);

    rhs.values.clear();
    rhs.low.clear();
    rhs.high.clear();
    rhs.valid = 0;

    // Return empty if no node
    if (!node) {
      std::cout << "No information found in YAML::Node" << std::endl;
      std::cout << "Format Required : " << std::endl;
      std::cout << yaml_template() << std::endl;
      return;
    }

    // Header required
    if (!node["header"]) {
      std::cout << "No header information found in YAML::Node" << std::endl;
      std::cout << "Format Required : " << std::endl;
      std::cout << yaml_template() << std::endl;
      return;
    }

    if (!node["header"]["name"]) {
      std::cout << "No name information found in YAML::Node" << std::endl;
      std::cout << "Format Required : " << std::endl;
      std::cout << yaml_template() << std::endl;
      return;
    }
    rhs.name = node["header"]["name"].as<std::string>();
    rhs.title = rhs.name;

    // Title optinal
    if (node["header"]["title"]) {
      rhs.title = node["header"]["title"].as<std::string>();
    }

    // Units are optional
    if (node["header"]["units"]) {
      rhs.units = node["header"]["units"].as<std::string>();
      rhs.title += " [" + rhs.units + "]";
    }

    // Qualifiers optional
    if (node["qualifiers"]) {
      YAML::Node node_qualifiers = node["qualifiers"];
      for (size_t i = 0; i < node_qualifiers.size(); i++) {
        if (node_qualifiers[i]["name"] && node_qualifiers[i]["value"]) {
        std::string name = node_qualifiers[i]["name"].as<std::string>();
        std::string value = node_qualifiers[i]["value"].as<std::string>();
        rhs.qualifiers[name] = value;
        }
      }
    }

    // Values Required
    if (!node["values"]) {
      std::cout << "No values information found in YAML::Node" << std::endl;
      std::cout << "Format Required : " << std::endl;
      std::cout << yaml_template() << std::endl;
    }
    YAML::Node xvalues = node["values"];

    // Need actual entries
    rhs.n = xvalues.size();
    if (rhs.n == 0) {
      std::cout << "Values list is empty in YAML::Node" << std::endl;
      std::cout << "Format Required : " << std::endl;
      std::cout << yaml_template() << std::endl;
    }

    rhs.values.resize(xvalues.size());
    rhs.errors.resize(xvalues.size());
    rhs.low.resize(xvalues.size());
    rhs.high.resize(xvalues.size());

    for (size_t i = 0; i < xvalues.size(); i++) {
      rhs.values[i] =
          xvalues[i]["value"] ? xvalues[i]["value"].as<double>() : -999;
      rhs.errors[i] =
          xvalues[i]["error"] ? xvalues[i]["error"].as<double>() : -999;
      rhs.low[i] =
          xvalues[i]["low"] ? xvalues[i]["low"].as<double>() : -999;
      rhs.high[i] =
          xvalues[i]["high"] ? xvalues[i]["high"].as<double>() : -999;
    }

    // Assume sqrt data if no errors, checking flag
    if (rhs.qualifiers.find("errors") != rhs.qualifiers.end()) {
      if (rhs.qualifiers["errors"] == "sumw") {
        for (size_t i = 0; i < xvalues.size(); i++) {
          if (rhs.values[i] > 0 && rhs.errors[i] == -999) {
            rhs.values[i] = sqrt(rhs.values[i]);
          }
        }
      }
    }

    rhs.valid = 1;
  }
};

};  // namespace nuis

namespace YAML {
template <> struct convert<nuis::Variables> {
  static bool decode(const Node &node, nuis::Variables &rhs) {  // NOLINT
    rhs = nuis::Variables(node);
    return rhs.valid;
  }
};
};  // namespace YAML

#endif