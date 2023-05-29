/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// data_provider.hh
// Manges the data access

#ifndef RANKTRACKER_DATA_PROVIDER_HH
#define RANKTRACKER_DATA_PROVIDER_HH

#include "data_model.hh"
#include <lmdb.h>
#include <boost/thread/tss.hpp>

#define RT_DB_MAPSIZE 10485760

namespace ranktracker {
  namespace persistence {

    using namespace ranktracker::data;

    struct DataProviderException {
      enum db_call_t {
        CREATE_ENV,
        SET_MAP_SIZE,
        SET_MAXDBS,
        OPEN_ENV,
        OPEN_DB,
        GET_ENV_INFO,
        BEGIN_TRANS,
        COMMIT,
        ROLLBACK,
        PUT,
        GET,
        DEL,
        OPEN_CURSOR,
        CLOSE_CURSOR,
        CURSOR_GET,
        CURSOR_DEL,
      } db_call;
      int db_call_result;

      DataProviderException(db_call_t db_call, int db_call_result) :
        db_call(db_call),
        db_call_result(db_call_result)
      {}
    };

    struct NotFoundException : DataProviderException {
      NotFoundException() : DataProviderException(GET, MDB_NOTFOUND) {}
    };

    struct DatabaseMapFullException : DataProviderException {
      DatabaseMapFullException(db_call_t db_call) : DataProviderException(db_call, MDB_MAP_FULL) {}
    };

    class create_transaction;
    class db_txn;

    class DataProvider {
      friend create_transaction;
      friend db_txn;

      MDB_env *db_env;
      boost::thread_specific_ptr<db_txn> txn_ptr;

      MDB_dbi dbi_categories;
      MDB_dbi dbi_domains;
      MDB_dbi dbi_keywords;
      MDB_dbi dbi_ranking;
      MDB_dbi dbi_categorydomains;
      MDB_dbi dbi_domainkeywords;
      MDB_dbi dbi_keywordranking;

      void create_env();
      void set_mapsize(unsigned int mapsize = RT_DB_MAPSIZE);
      void set_maxdbs();
      void env_open();
      void open_db(const char *dbname,
                   unsigned int flags,
                   MDB_dbi *dbi);

      void begin_trans(unsigned int flags = 0);
      void commit();
      void rollback();

      void put(MDB_dbi dbi, MDB_val *k, MDB_val *v);
      void del(MDB_dbi dbi, MDB_val *k, MDB_val *v);
      MDB_val get(MDB_dbi dbi, const MDB_val &k) const;

      template<class E, class K = AbstractEntity::id_type>
      void put(MDB_dbi dbi, const K& k, const E &e);

      template<class E, class K = AbstractEntity::id_type>
      E get(MDB_dbi dbi, const K &k) const;

      template<class E, class K = AbstractEntity::id_type>
      void del(MDB_dbi dbi, const K& k, const E &e);

      template<class K = AbstractEntity::id_type>
      void del(MDB_dbi dbi, const K& k);

    public:
      void increase_mapsize();

    public:
      DataProvider();
      virtual ~DataProvider();

    public:
      /**
       * Gets all the defined categories. The list of categories
       * is expected to be rather small, so reading it all in
       * memory sims to be a good choice.
       */
      std::vector<Category> categories() const;

      /**
       * Persist a category definition on the database.
       */
      void storeCategory(const Category&);

      /**
       * Delete a category and associated domains.
       */
      void deleteCategory(const Category&);

      /**
       * Retrieve all domains defined in the database.
       */
      std::vector<Domain> domains() const;

      /**
       * Retrieve the domains in a certain category.
       */
      std::vector<Domain> domains(const Category&) const;

      /**
       * update the definition of a domain, without modifiyng the
       * category under the domain is stored
       */
      void storeDomain(const Domain& domain);

      /**
       * persist a domain and add it to some category, by adding a
       * new record to categorydomains index; use this function only
       * to add a new domain
       */
      void storeDomain(const Domain& domain, const Category& category);

      /**
       * Delete a domain from the database.
       */
      void deleteDomain(const Domain& domain);

      /**
       * loads the keywords for a domain from db.
       */
      std::vector<Keyword> keywords(const Domain&) const;

      std::size_t countKeywords(const Domain& domain) const;

      /**
       * stores a keyword in the database, adding it to a domain by
       * inserting a new record in the `domainkeywords` index; use
       * this call only to add a new keyword
       */
      void storeKeyword(const Keyword& keyword, const Domain& domain);

      /**
       * updates the definition of a keyword without modifying the
       * `domainkeywords` index
       */
      void storeKeyword(const Keyword& keyword);

      /**
       * Add bulk keywords to the database.
       */
      void store_bulk_keywords(const Domain& domain, const std::string& kwds);

      /**
       * Delete a keyword from db.
       */
      void deleteKeyword(const Keyword& keyword, const Domain& domain);

      /**
       * stores a new ranking into the database
       */
      void storeRanking(const Keyword& k,
                        ranktracker::engine::SearchEngine const &e,
                        const Ranking& rank_info);

      /**
       * get a sorted vector of rankings for given keyword
       */
      std::vector<Ranking> rankings(const Keyword& k, ranktracker::engine::SearchEngine const &e) const;

      /**
       * delete all ranking history related to a keyword and a search engine
       */
      void delete_rankings(const Keyword& k, ranktracker::engine::SearchEngine const &e);

      /**
       * get last ranking for a keyword
       */
      Ranking last_ranking(const Keyword& k, ranktracker::engine::SearchEngine const &e) const;

      /**
       * get best ranking
       */
      Ranking best_ranking(const Keyword& k, ranktracker::engine::SearchEngine const &e) const;

      /**
       * get prev ranking
       */
      Ranking prev_ranking(const Keyword& k, ranktracker::engine::SearchEngine const &e, const Ranking& r);

      /**
       * get diff ranking for a keyword
       */
       ranktracker::engine::rank_result_type
       diff_ranking(const Keyword& k,
                    ranktracker::engine::SearchEngine const &e,
                    Ranking *last = nullptr,
                    Ranking *prev = nullptr) const;
    };

    class db_txn {
      MDB_txn *_txn;
    public:
      db_txn(DataProvider &db, unsigned int flags = 0);
      void commit();
      void rollback();
      ~db_txn();
      MDB_txn *get() { return _txn; }
    };

    class create_transaction {
      DataProvider *_p;
    public:
      create_transaction(DataProvider * p, unsigned int flags = 0) : _p(p) {
        assert(p != NULL);
        p->begin_trans(flags);
      }

      virtual ~create_transaction() {
        if(_p) _p->rollback();
      }

      void commit() {
        if(_p) {
          DataProvider *p = _p;
          _p = NULL;
           p->commit();
        }
      }

      void rollback() {
        if(_p) {
          DataProvider *p = _p;
          _p = NULL;
          p->rollback();
        }
      }
    };
  }
}

#endif
