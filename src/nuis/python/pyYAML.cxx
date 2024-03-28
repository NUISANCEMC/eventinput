#include "nuis/python/pyYAML.h"

// Python dictionary to YAML conversion
namespace PYBIND11_NAMESPACE {
namespace detail {
YAML::Node type_caster<YAML::Node>::PyListToYAML(PyObject *pList) {
  YAML::Node n;

  for (Py_ssize_t i = 0; i < PyList_Size(pList); ++i) {

    PyObject *pVal = PyList_GetItem(pList, i);

    if (PyUnicode_Check(pVal)) {
      n[i] = std::string(PyUnicode_AsUTF8(pVal));

    } else if (PyLong_Check(pVal)) {
      n[i] = static_cast<int>(PyLong_AsLong(pVal));

    } else if (PyFloat_Check(pVal)) {
      n[i] = static_cast<double>(PyFloat_AsDouble(pVal));

    } else if (PyDict_Check(pVal)) {
      n[i] = PyDictToYAML(pVal);

    } else if (PyList_Check(pVal)) {
      n[i] = PyListToYAML(pVal);

    } else {
      std::cout << "Only Concrete Python instances allowed as "
                << "dict inputs to YAML conversion!" << std::endl;

      std::cout << "Index [" << i << "] is problematic!" << std::endl;
    }
  }

  return n;
}

YAML::Node type_caster<YAML::Node>::PyDictToYAML(PyObject *pDict) {
  YAML::Node n;

  PyObject *pKeys = PyDict_Keys(pDict);
  PyObject *pValues = PyDict_Values(pDict);

  for (Py_ssize_t i = 0; i < PyDict_Size(pDict); ++i) {
    PyObject *pKey = PyList_GetItem(pKeys, i);

    if (!PyUnicode_Check(pKey)) {
      std::cout << "YAML Conversion can only handle string keys!" << std::endl;
    }

    std::string key = PyUnicode_AsUTF8(PyList_GetItem(pKeys, i));

    PyObject *pVal = PyList_GetItem(pValues, i);

    if (PyUnicode_Check(pVal)) {
      n[key] = std::string(PyUnicode_AsUTF8(pVal));

    } else if (PyLong_Check(pVal)) {
      n[key] = static_cast<int>(PyLong_AsLong(pVal));

    } else if (PyFloat_Check(pVal)) {
      n[key] = static_cast<double>(PyFloat_AsDouble(pVal));

    } else if (PyDict_Check(pVal)) {
      n[key] = PyDictToYAML(pVal);

    } else if (PyList_Check(pVal)) {
      n[key] = PyListToYAML(pVal);

    } else {
      std::cout << "Only Concrete Python instances allowed "
                << " as dict inputs to YAML conversion!" << std::endl;

      std::cout << "KEY [" << key << "] is problematic!" << std::endl;
    }
  }

  return n;
}

bool type_caster<YAML::Node>::load(handle src, bool) {
  /* Extract PyObject from handle */
  PyObject *source = src.ptr();

  if (PyUnicode_Check(source)) {
    value = std::string(PyUnicode_AsUTF8(source));
  } else if (PyLong_Check(source)) {
    value = static_cast<int>(PyLong_AsLong(source));
  } else if (PyFloat_Check(source)) {
    value = static_cast<double>(PyFloat_AsDouble(source));
  } else if (PyDict_Check(source)) {
    value = PyDictToYAML(source);
  } else if (PyList_Check(source)) {
    value = PyListToYAML(source);
  } else {
    std::cout << "Only Concrete Python instances allowed as "
              << "dict inputs to YAML conversion!" << std::endl;
  }

  return 1;
}

handle type_caster<YAML::Node>::cast(YAML::Node src,
                                     return_value_policy /* policy */,
                                     handle /*parent*/) {

            std::stringstream ss;
            ss << src << std::endl;

            // Importing yaml module
            object yaml_module = module_::import("yaml");

            // Getting the safe_load function
            object safe_load = yaml_module.attr("safe_load");

            // Invoking safe_load function with YAML content
            object result = safe_load(ss.str().c_str());

            // Check if the result is valid
            if (result.is_none()) {
              return none();
            }
            return result.release();
        }

} // namespace detail
} // namespace PYBIND11_NAMESPACE