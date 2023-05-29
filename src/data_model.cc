// data_model.cc
// data model related classes and functions

#include "data_model.hh"

namespace ranktracker {
  namespace data {
    void Domain::insert_engine(const SearchEngineRef& engine) {
      _engines.insert(engine);
    }

    void Domain::erase_engine(const SearchEngineRef& engine) {
      _engines.erase(engine);
    }
  }
}
