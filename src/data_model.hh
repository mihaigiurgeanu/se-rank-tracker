/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// data_model.hh
// definitions for the data_model used in the application

#ifndef RANKTRACKER_DATA_MODEL_HH
#define RANKTRACKER_DATA_MODEL_HH

#include <string>
#include <unordered_set>
#include <vector>
#include <utility>
#include <functional>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/unordered_set.hpp>

#include "entity.hh"
#include "engines.hh"

namespace ranktracker {
  namespace data {
    class Category : public Entity {
      friend boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, unsigned int version) {
        if(version > 0)
          throw UnknownSerializationVersion(version);
        ar & boost::serialization::base_object<Entity>(*this);
        ar & _name;
      }

      std::string _name;

    public:
      Category() {}

      Category(std::string name) :
        _name(name)
      {}

      Category(id_type id, std::string name) :
        Entity(id),
        _name(name)
      {}

      const std::string& name() const { return _name; };
      void name(std::string name) { _name.swap(name); };
    };

    class Keyword : public Entity {
      friend boost::serialization::access;

      template<class Archive>
      void serialize(Archive &ar, unsigned int version) {
        if(version > 0) {
          throw UnknownSerializationVersion(version);
        }
        ar & boost::serialization::base_object<Entity>(*this);
        ar & _value;
      }

      std::string _value;
    public:
      Keyword() {}

      Keyword(std::string value) : _value(value)
      {}
      Keyword(id_type id, std::string value) :
        Entity(id),
        _value(value)
      {}

      const std::string& value() const { return _value; }
      void value(std::string value) {
        _value.swap(value);
      }
    };

    using ranktracker::engine::GoogleEngine;
    using ranktracker::engine::engines_set;
    using boost::posix_time::ptime;
    using boost::posix_time::second_clock;
    using ranktracker::engine::SearchEngineRef;

    typedef std::unordered_set<Keyword, Entity_hasher> keywords_set;

    class Domain : public Entity {
      friend boost::serialization::access;

      template<class Archive>
      void serialize(Archive &ar, unsigned int version) {
        if(version > 0)
          throw UnknownSerializationVersion(version);

        ar & boost::serialization::base_object<Entity>(*this);
        ar & _name;
        ar & _engines;
        ar & _createdDate;
        ar & _lastUpdateDate;
      }

      std::string _name;
      engines_set _engines;
      ptime _createdDate;
      ptime _lastUpdateDate;
    public:
      Domain() : _createdDate(second_clock::local_time()) {}

      Domain(std::string name) :
        _name(name),
        _createdDate(second_clock::local_time())
      {}

      Domain(std::string name,
             engines_set engines) :
        _name(name),
        _engines(engines),
        _createdDate(second_clock::local_time())
      {}

      Domain(id_type id,
             std::string name,
             engines_set engines,
             ptime createdDate,
             ptime lastUpdateDate
             ) :
        Entity(id),
        _name(name),
        _engines(engines),
        _createdDate(createdDate),
        _lastUpdateDate(lastUpdateDate)
      {}

      const std::string& name() const { return _name; }
      void name(std::string new_name) { std::swap(_name, new_name); }

      const engines_set& engines() const { return _engines; }
      const ptime& createdDate() const { return _createdDate; }
      const ptime& lastUpdateDate() const { return _lastUpdateDate; }

      void insert_engine(const SearchEngineRef& engine);
      void erase_engine(const SearchEngineRef& engine);
    };

    struct KeywordEngine {
      AbstractEntity::id_type _kwdid, _engid; // the ids of the keyword and engine
    private:
      friend boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, unsigned int) {
        ar & _kwdid;
        ar & _engid;
      }
    };

    struct RankingKey {
      AbstractEntity::id_type _kwdid, _engid; // the ids of the keyword and engine
      ptime _ranking_date; // the timestamp of the ranking
    private:
      friend boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, unsigned int) {
        ar & _kwdid;
        ar & _engid;
        ar & _ranking_date;
      }
    };

    struct Ranking {
      ptime _ranking_date;
      ranktracker::engine::rank_result_type _rank;
      std::string _page_url;
    private:
      friend boost::serialization::access;
      template<class Archive>
      void serialize(Archive &ar, unsigned int) {
        ar & _ranking_date;
        ar & _rank;
        ar & _page_url;
      }
    };
  }
}


#endif
