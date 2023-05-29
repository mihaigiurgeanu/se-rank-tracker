/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// engines.hh
// defiinitions related to search engines and the
// code to perform rank queries

#ifndef ENGINES_HH
#define ENGINES_HH

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <curl/curl.h>

#include "entity.hh"
#include "progress.hh"

namespace ranktracker {
  namespace engine {

    using ranktracker::data::Entity;
    using ranktracker::data::AbstractEntity;
    using ranktracker::data::Entity_hasher;
    using ranktracker::data::UnknownSerializationVersion;
    using ranktracker::progress::progress_updater;

    typedef int rank_result_type;

    class SearchEngineRef;

    typedef std::unordered_map<Entity::id_type, SearchEngineRef, boost::hash<Entity::id_type>> engines_map;
    typedef std::unordered_set<SearchEngineRef, Entity_hasher> engines_set;

    /**
     * get the map of search engines
     */
    const engines_map& search_engines();

    class SearchEngine : public Entity {
      std::string _name;
      std::string _description;
      std::string _url;

    public:
      SearchEngine(AbstractEntity::id_type id,
                   std::string name,
                   std::string description,
                   std::string url) :
        Entity(id),
        _name(name),
        _description(description),
        _url(url)
      {}

      const std::string& name() const { return _name; }
      const std::string& description() const { return _description; }
      const std::string& url() const { return _url; }

      /**
       * rank computation performing; connects to the search engine
       * perform the query on the keywords string and verfies the rank
       * of the given domain name
       */
      virtual rank_result_type perform_rank_query(std::string domain,
                                                  std::string keywords,
                                                  progress_updater& p,
                                                  std::string *page_url = nullptr) const = 0;
    };

    /**
     * SearchEngine implementation for Google
     */
    class GoogleEngine : public SearchEngine {
    public:
      using SearchEngine::SearchEngine;

      rank_result_type perform_rank_query(std::string domain,
                                          std::string keywords,
                                          progress_updater& p,
                                          std::string* page_url = nullptr) const override;
    };

    class SearchEngineRef : public AbstractEntity {
      friend boost::serialization::access;

      template<class Archive>
      void serialize(Archive &ar, unsigned int version) {
        boost::serialization::split_member(ar, *this, version);
      }

      template<class Archive>
      void save(Archive &ar, unsigned int version) const {
        if(version > 0) {
          throw UnknownSerializationVersion(version);
        }

        assert(_engine != NULL);
        ar << _engine->id();
      }

      template<class Archive>
      void load(Archive &ar, unsigned int version) {
        if(version > 0) {
          throw UnknownSerializationVersion(version);
        }
        AbstractEntity::id_type theid;
        ar >> theid;
        _engine = search_engines().at(theid)._engine;
      }

      SearchEngine* _engine;
    public:
      SearchEngineRef() : _engine(NULL) {}

      SearchEngineRef(SearchEngine * engine) :
        _engine(engine)
      {
        assert(engine != NULL);
      }

      const AbstractEntity::id_type& id() const { assert(_engine != NULL); return _engine->id(); }
      const std::string& name() const { assert(_engine != NULL); return _engine->name(); }
      const std::string& description() const { assert(_engine != NULL); return _engine->description(); }
      const std::string& url() const { assert(_engine != NULL); return _engine->url(); }
      const SearchEngine* engine() const { return _engine; }
      const SearchEngine& operator* () const { assert(_engine != NULL); return *_engine;}

      rank_result_type perform_rank_query(std::string domain,
                                          std::string keywords,
                                          progress_updater& p,
                                          std::string *page_url = nullptr) const {
        assert(_engine != NULL);
        return _engine->perform_rank_query(domain, keywords, p, page_url);
      }
    };

    /**
     * initialize the global map of search engines used by the app.
     */
    void init_search_engines();

    class search_exception {};

    class http_exception: public search_exception {
    };

    class http_init_exception : public http_exception {
    };

    class http_request_failed_exception : public http_exception {
      CURLcode _result_code;
    public:
      http_request_failed_exception(CURLcode result_code) : _result_code(result_code) {}

      CURLcode result_code() const {return _result_code; }
    };

    class parse_exception: public search_exception {};
    class parse_init_exception : public parse_exception {};
    class parse_end_exception : public parse_exception {};

    class dom_exception : public search_exception {};

    class next_link_not_found : public search_exception {};
  }
}

#endif
