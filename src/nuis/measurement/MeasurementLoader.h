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
#pragma once
#include <vector>
#include <string>

#include "yaml-cpp/yaml.h"
#include "HepMC3/GenEvent.h"
#include "ProSelecta/ProSelecta.h"

#include "nuis/measurement/Record.h"
#include "nuis/measurement/Variables.h"
#include "nuis/measurement/Document.h"

namespace nuis {
namespace measurement {

class MeasurementLoader {
public:

  MeasurementLoader() {
  }

  explicit MeasurementLoader(YAML::Node config) {
  }

  inline virtual ~MeasurementLoader() {
  }

  inline virtual std::vector<double>
    ProjectEvent(const HepMC3::GenEvent& event) {
    return std::vector<double>();
  }

  inline virtual bool
    FilterEvent(const HepMC3::GenEvent& event) {
    return false;
  }

  inline virtual double
    WeightEvent(const HepMC3::GenEvent& event) {
      return 1.0;
  }

  virtual Record CreateRecord(const std::string label = "MC") {
      return Record();
  }

  virtual bool FillRecordFromEvent(
    Record& h, const HepMC3::GenEvent& event, const double weight) {
  }

  virtual bool FillRecordFromProj(
    Record& h, const std::vector<double>& val, const double weight) {
  }

  virtual void FinalizeRecord(Record& h, double scaling) {
  }

  virtual std::string summary() {
  }

  inline virtual void print() {
    std::cout << summary() << std::endl;
  }

  std::string measurement_name;
  std::string filter_symname;
  std::string weight_symname;
  // std::string finalize_symname;
  std::vector<std::string> proj_funcnames;

  ProSelecta_ftypes::sel filter_func;
  ProSelecta_ftypes::wgt weight_func;
  // ProSelecta_ftypes::fin finalize_func;
  std::vector<ProSelecta_ftypes::pro> proj_funcs;

  Document measurement_document;
  std::vector<Variables> independent_variables;
  std::vector<Variables> dependent_variables;

  std::vector<Variables> independent_covariances;
  std::vector<Variables> dependent_covariances;
};

}  // namespace measurement
}  // namespace nuis