#include "nuis/record/Comparison.h"
#include "nuis/record/IRecord.h"
#include "nuis/record/RecordFactory.h"
#include "nuis/record/Table.h"
#include "nuis/record/Utility.h"

#include "nuis/python/pyNUISANCE.h"

#include "HepMC3/GenEvent.h"

#include "pybind11/eigen.h"
#include "pybind11/functional.h"

#include <string>

namespace py = pybind11;
using namespace nuis;

namespace nuis {
using TablePtr = std::shared_ptr<Table>;
using IRecordPtr = std::shared_ptr<IRecord>;

struct pyRecord {

  pyRecord() {}

  // explicit pyRecord(YAML::Node const &cfg = {}) {}

  TablePtr table(std::string name) { return rec->table(name); }

  TablePtr operator[](std::string name) { return rec->table(name); }

  IRecordPtr rec;
};

struct pyRecordFactory {

  pyRecordFactory() {}

  pyRecord make(YAML::Node const &cfg = {}) {
    auto calcs = rfact.make(cfg);
    auto pyrec = pyRecord();
    pyrec.rec = calcs;
    return pyrec;
  }

  TablePtr make_table(YAML::Node const &cfg = {}) {
    return rfact.make_table(cfg);
  }

  RecordFactory rfact;
};
} // namespace nuis

void pyRecordInit(py::module &m) {

// py::enum_<YAML::NodeType::value>(m, "NodeType")
//     .value("Undefined", YAML::NodeType::Undefined)
//     .value("Null", YAML::NodeType::Null)
//     .value("Scalar", YAML::NodeType::Scalar)
//     .value("Sequence", YAML::NodeType::Sequence)
//     .value("Map", YAML::NodeType::Map);

    py::class_<YAML::Node>(m, "YAMLNode")
        .def(py::init<const std::string &>())
        .def("__getitem__",
            [](const YAML::Node node, const std::string& key){
              return node[key];
            })
        .def("__setitem__",
            [](YAML::Node node, const std::string& key, const double& val){
              return node[key] = val;
            })
        .def("__iter__",
            [](const YAML::Node &node) {
              return py::make_iterator(node.begin(), node.end());},
             py::keep_alive<0, 1>())
        .def("__str__",
             [](const YAML::Node& node) {
               YAML::Emitter out;
               out << node;
               return std::string(out.c_str());
             })
        .def("type", &YAML::Node::Type)
        .def("__len__", &YAML::Node::size)
        ;

//     py::class_<YAML::detail::iterator_value, YAML::Node>(m, "YamlDetailIteratorValue")
//         .def(py::init<>())
//         .def("first", [](YAML::detail::iterator_value& val) { return val.first;})
//         .def("second", [](YAML::detail::iterator_value& val) { return val.second;})
//         ;

  py::class_<Comparison, std::shared_ptr<Comparison>>(m, "Comparison")
      .def_readwrite("metadata", &Comparison::metadata)
      .def_readwrite("mc", &Comparison::mc)
      .def_readwrite("data", &Comparison::data)
      .def_readwrite("estimate", &Comparison::estimate);

  py::class_<pyRecordFactory>(m, "RecordFactory")
      .def(py::init<>())
      .def("make_record", &pyRecordFactory::make)
      .def("make_table", &pyRecordFactory::make_table);


  py::class_<pyRecord>(m, "Record")
      .def(py::init<>())
      .def("table", &pyRecord::table)
      .def("__getattr__", &pyRecord::table);

  py::class_<Table,TablePtr>(m, "Table")
      .def(py::init<>())
      .def("metadata", [](const Table &t) { return t.metadata; })
      .def("get_metadata", [](const Table &t) { return t.metadata; })
      .def("set_metadata", [](Table &t, YAML::Node node) { t.metadata = node; })
      .def_readwrite("blueprint", &Table::blueprint)
      .def_readwrite("clear", &Table::clear)
      .def_readwrite("select", &Table::select)
      .def_readwrite("project", &Table::project)
      .def_readwrite("weight", &Table::weight)
      .def_readwrite("finalize", &Table::finalize)
      .def("comparison", &Table::comparison)
      .def_readwrite("likelihood", &Table::likelihood);

  //      .def_readwrite("likeihood", &Table::likeihood)
  //      .def("add_column", &Table::add_column)
  //      .def("find_column_index", &Table::find_column_index);
}

