/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// data_provider.cc
// Database operations

#define _GNU_SOURCE

#include "data_provider.hh"
#include "app_support_folder.hh"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/stacktrace.hpp>

#include <sstream>
#include <cstring>
#include <thread>

namespace ranktracker {
  namespace persistence {
    using namespace ranktracker::data;

    int const maxdbs = 7;
    int const db_mapsize = RT_DB_MAPSIZE;

    char const * const dbname_categories = "categories";
    char const * const dbname_domains = "domains";
    char const * const dbname_keywords = "keywords";
    char const * const dbname_ranking = "rankingv2";
    char const * const dbname_categorydomains = "categorydomains";
    char const * const dbname_domainkeywords = "domainkeywords";
    char const * const dbname_keywordranking = "keywordrankingv2";

    template <class DataObject, class Key = AbstractEntity::id_type>
    class Cursor {
      MDB_cursor *_cursor;
      Cursor() = delete;

      void get(MDB_val *key, MDB_val *val, MDB_cursor_op op);
    public:
      Cursor(MDB_txn * txn, MDB_dbi dbi) {
        using namespace std;
        int result = mdb_cursor_open(txn, dbi, &_cursor);
        switch(result) {
        case 0:
          // everything is ok
          goto cursor_open_ok;
        case EINVAL:
          BOOST_LOG_TRIVIAL(error) << "Error opneing the databae cursor: an invalid parameter was passed to mdb_cursor_open\n";
          break;
        default:
          BOOST_LOG_TRIVIAL(error) << "Unexpected error opening a new cursor: " << result << endl;
        }
        throw DataProviderException(DataProviderException::OPEN_CURSOR, result);
      cursor_open_ok:
        return;
      }

      virtual ~Cursor() {
        mdb_cursor_close(_cursor);
      }

      void get(Key& key, DataObject& data, MDB_cursor_op op);
      void del();
      size_t count();
    };

    DataProvider::DataProvider() {
      create_env();
      set_mapsize();
      set_maxdbs();
      env_open();

      begin_trans();
      try {
        open_db(dbname_categories, MDB_CREATE, &dbi_categories);
        open_db(dbname_domains, MDB_CREATE, &dbi_domains);
        open_db(dbname_keywords, MDB_CREATE, &dbi_keywords);
        open_db(dbname_ranking, MDB_CREATE, &dbi_ranking);
        open_db(dbname_categorydomains, MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, &dbi_categorydomains);
        open_db(dbname_domainkeywords, MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, &dbi_domainkeywords);
        open_db(dbname_keywordranking, MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, &dbi_keywordranking);
        commit();
      } catch (...) {
        using namespace std;
        BOOST_LOG_TRIVIAL(error) << "Error initializing the database\n";
        rollback();
        throw;
      }
    }

    DataProvider::~DataProvider() {
      if (db_env) {
        mdb_env_close(db_env);
      }
    }

    std::vector<Category>
    DataProvider::categories() const {
      boost::uuids::nil_generator uuidgen;
      std::vector<Category> cats {Category(uuidgen(), "All Domains")};
      try {
        // expects a transaction to be opened (see create_transaction class)
        Cursor<Category> crs(txn_ptr->get(), dbi_categories);
        Category cat;
        AbstractEntity::id_type id;

        crs.get(id, cat, MDB_FIRST);
        cats.push_back(std::move(cat));
      categories_read_next_cat:
        crs.get(id, cat, MDB_NEXT);
        cats.push_back(std::move(cat));
        goto categories_read_next_cat;

      } catch (NotFoundException) {
        // success; nothing to do
      }
      return cats;
    }

    void DataProvider::storeCategory(const Category& category) {
      using namespace std;
      try {
        // expects a write transaction open
        put(dbi_categories, category.id(), category);
      } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "Failed to store category '" << category.name() << "'\n";
        throw;
      }

    }

    void DataProvider::deleteCategory(const Category& category) {
      auto ds = domains(category);
      for(auto& d: ds) {
        deleteDomain(d);
      }
      del(dbi_categories, category.id());
    }

    std::vector<Domain>
    DataProvider::domains() const {
      std::vector<Domain> ds;
      try {
        // expects a transation already opened
        Cursor<Domain> crs(txn_ptr->get(), dbi_domains);
        Domain d;
        AbstractEntity::id_type id;

        crs.get(id, d, MDB_FIRST);
        ds.push_back(std::move(d));
      domains_read_next_d:
        crs.get(id, d, MDB_NEXT);
        ds.push_back(std::move(d));
        goto domains_read_next_d;

      } catch (NotFoundException) {
        // success
      }
      return ds;
    }

    std::vector<Domain>
    DataProvider::domains(const Category& domainCategory) const {
      using namespace std;
      if((domainCategory.id().is_nil())) {
        return domains();
      } else {
        std::vector<Domain> ds;
        try {
          // expoects  transaction to be opened aready
          Cursor<AbstractEntity::id_type> crs(txn_ptr->get(), dbi_categorydomains);
          AbstractEntity::id_type cat_id = domainCategory.id();
          AbstractEntity::id_type dom_id;
          crs.get(cat_id, dom_id, MDB_SET);
        category_domains_next:
          try {
            ds.push_back(get<Domain>(dbi_domains, dom_id));
          } catch (NotFoundException) {
            BOOST_LOG_TRIVIAL(warning) << "Warning: domain id not foun ("
                 << dom_id << "), but apears to be parth of category "
                 << domainCategory.name() << endl;
          }
          crs.get(cat_id, dom_id, MDB_NEXT_DUP);
          goto category_domains_next;
        } catch (NotFoundException) {
          //finished reading the database
        }
        return ds;
      }
    }

    void DataProvider::storeDomain(const Domain& domain, const Category& category) {
      using namespace std;
      try {
        // expects a transaction to be opened
        put(dbi_domains, domain.id(), domain);
        if(!category.id().is_nil()) {
          put(dbi_categorydomains, category.id(), domain.id());
        }
      } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "Failed to store domain '" << domain.name() << "'\n";
        throw;
      }

    }

    void DataProvider::deleteDomain(const Domain& domain) {
      BOOST_LOG_TRIVIAL(trace) << "Delete keywords for domain: " << domain.name() << "\n";
      auto ks = keywords(domain);
      for(auto &k:ks) {
        deleteKeyword(k, domain);
      }

      BOOST_LOG_TRIVIAL(trace) << "Delete domain: " << domain.name() << "\n";
      del(dbi_domains, domain.id());
    }

    void DataProvider::storeDomain(const Domain& domain) {
      using namespace std;
      try {
        // expects a transaction to be opened
        put(dbi_domains, domain.id(), domain);
      } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "Failed to store domain '" << domain.name() << "'\n";
        throw;
      }

    }

    /**
     * Loads the keywords for a domain from db. It expects a transaction
     * to be started.
     */
    std::vector<Keyword> DataProvider::keywords(const Domain& domain) const {
      std::vector<Keyword> ks;
      // a transaction should already be started
      Cursor<AbstractEntity::id_type>crs(txn_ptr->get(), dbi_domainkeywords);
      AbstractEntity::id_type domain_id = domain.id();
      AbstractEntity::id_type kwd_id;

      try {
        crs.get(domain_id, kwd_id, MDB_SET);
      domain_keywords_next:
        BOOST_LOG_TRIVIAL(trace) << "get next keyword for domain " << domain.name() << "\n";
        ks.push_back(get<Keyword>(dbi_keywords, kwd_id));
        crs.get(domain_id, kwd_id, MDB_NEXT_DUP);
        goto domain_keywords_next;
      } catch (NotFoundException) {
        // do nothing; successfully read all the available
        // keywords
      }

      return ks;
    }

    /**
     * Count keywords for a domain.
     */
    size_t DataProvider::countKeywords(const Domain& domain) const {
      // a transaction should already be started
      Cursor<AbstractEntity::id_type>crs(txn_ptr->get(), dbi_domainkeywords);
      AbstractEntity::id_type domain_id = domain.id();
      AbstractEntity::id_type kwd_id;
      try {
        crs.get(domain_id, kwd_id, MDB_SET);
      } catch (NotFoundException) {
        // the key was not found
        return 0;
      }
      return crs.count();
    }

    /**
     * stores a keyword in the database
     */
    void DataProvider::storeKeyword(const Keyword& keyword, const Domain& domain) {
      try {
        put<Keyword>(dbi_keywords, keyword.id(), keyword);
        put<AbstractEntity::id_type>(dbi_domainkeywords, domain.id(), keyword.id());
      } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "Failed to store keywords in database: '"
                  << keyword.value()
                  << "'\n";
        throw;
      }
    }

    void DataProvider::storeKeyword(const Keyword& keyword) {
      try {
        put<Keyword>(dbi_keywords, keyword.id(), keyword);
      } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "Failed to store keywords in database: '"
                  << keyword.value()
                  << "'\n";
        throw;
      }
    }

    void DataProvider::store_bulk_keywords(const Domain& domain, const std::string& kwds) {
      std::istringstream _kwds(kwds);
      std::string kwd;
      try {
        while(std::getline(_kwds, kwd)) {
          boost::algorithm::trim(kwd);
          if(kwd.size() > 0) {
            storeKeyword(kwd, domain);
          }
        }
      } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "Failed to store bulk keywords to database\n";
        throw;
      }
    }

    void DataProvider::deleteKeyword(const Keyword& keyword, const Domain& domain) {
      try {
        BOOST_LOG_TRIVIAL(trace) << "delete keyword: " << keyword.value() << "\n";
        del(dbi_keywords, keyword.id());
        del<AbstractEntity::id_type>(dbi_domainkeywords, domain.id(), keyword.id());
        for(auto e: domain.engines()) {
          BOOST_LOG_TRIVIAL(trace) << "delete rankings for keyword: " << keyword.value() << "\n";
          delete_rankings(keyword, *e);
        }
      } catch (NotFoundException) {
        BOOST_LOG_TRIVIAL(warning) << "Keyword not found when trying to delete: " << keyword.value() << std::endl;
      } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "Failed to delete keyword from database: '"
                  << keyword.value()
                  << "'\n";
        throw;
      }
    }

    void DataProvider::storeRanking(const Keyword& k,
                                    ranktracker::engine::SearchEngine const &e,
                                    const Ranking& rank_info) {
      try {
        RankingKey key = {k.id(), e.id(), rank_info._ranking_date};
        BOOST_LOG_TRIVIAL(trace) << "storing keyword rank " << rank_info._rank;
        put<Ranking, RankingKey>(dbi_ranking, key, rank_info);
        BOOST_LOG_TRIVIAL(trace) << "storing ranking date";
        put<ptime, KeywordEngine>(dbi_keywordranking, {k.id(), e.id()}, key._ranking_date);
      } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "Failed to store new rankoing in the database for keyword: "
                  << k.value()
                  << std::endl;
        throw;
      }
    }

    std::vector<Ranking> DataProvider::rankings(const Keyword& k,
                                                ranktracker::engine::SearchEngine const &e) const {
      std::vector<Ranking> rs;
      Cursor<ptime, KeywordEngine> crs(txn_ptr->get(), dbi_keywordranking);
      AbstractEntity::id_type kwd_id = k.id();
      AbstractEntity::id_type eng_id = e.id();
      KeywordEngine key = {kwd_id, eng_id};
      ptime ranking_date;

      try {
        BOOST_LOG_TRIVIAL(trace) << "reading first ranking for the keyword\n";
        crs.get(key, ranking_date, MDB_SET);
      rankings_next:
        rs.push_back(
              get<Ranking, RankingKey>(dbi_ranking,
                {kwd_id, eng_id, ranking_date}));
        BOOST_LOG_TRIVIAL(trace) << "reading next ranking for the keyword\n";
        crs.get(key, ranking_date, MDB_NEXT_DUP);
        goto rankings_next;
      } catch (NotFoundException) {
        BOOST_LOG_TRIVIAL(trace) << "no more ranking for the keyword";
        // successfully read all available raking records
      }
      return rs;
    }

    void DataProvider::delete_rankings(const Keyword& k,
                                       ranktracker::engine::SearchEngine const &e) {
      Cursor<ptime, KeywordEngine> crs(txn_ptr->get(), dbi_keywordranking);
      AbstractEntity::id_type kwd_id = k.id();
      AbstractEntity::id_type eng_id = e.id();
      KeywordEngine key = {kwd_id, eng_id};
      ptime ranking_date;

      crs.get(key, ranking_date, MDB_SET);
      rankings_next:
      BOOST_LOG_TRIVIAL(trace) << "delete ranking keys\n";
      try {
        del<RankingKey>(dbi_ranking, {kwd_id, eng_id, ranking_date});
        crs.get(key, ranking_date, MDB_NEXT_DUP);
        goto rankings_next;
      } catch (NotFoundException) {
        // successfully read all available raking records
        BOOST_LOG_TRIVIAL(trace) << "delete ranking\n";
        try {
          del<KeywordEngine>(dbi_keywordranking, key);
        } catch (NotFoundException) {
          BOOST_LOG_TRIVIAL(warning) << "Delete ranking: ranking not found\n";
        }
      }
    }

    Ranking DataProvider::last_ranking(const Keyword& k,
                                       ranktracker::engine::SearchEngine const &e) const {
      Cursor<ptime, KeywordEngine> crs(txn_ptr->get(), dbi_keywordranking);
      AbstractEntity::id_type kwd_id = k.id();
      AbstractEntity::id_type eng_id = e.id();
      KeywordEngine key = {kwd_id, eng_id};
      ptime ranking_date;

      try {
        BOOST_LOG_TRIVIAL(trace) << "reading the last ranking for the keyword\n";
        crs.get(key, ranking_date, MDB_SET);
        crs.get(key, ranking_date, MDB_LAST_DUP);
        return
          get<Ranking,
              RankingKey>(dbi_ranking, {kwd_id, eng_id, ranking_date});
      } catch (NotFoundException) {
        // OK - no rankings available
        BOOST_LOG_TRIVIAL(trace) << "no rankings available\n";
        throw;
      } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "could not get last ranking for keyword: " << k.value() << std::endl;
        throw;
      }
    }

    ranktracker::engine::rank_result_type
    DataProvider::diff_ranking(const Keyword& k,
                               ranktracker::engine::SearchEngine const &e,
                               Ranking *last,
                               Ranking *prev) const {
      Cursor<ptime, KeywordEngine> crs(txn_ptr->get(), dbi_keywordranking);
      AbstractEntity::id_type kwd_id = k.id();
      AbstractEntity::id_type eng_id = e.id();
      KeywordEngine key = {kwd_id, eng_id};
      ptime ranking_date_last, ranking_date_prev;

      try {
        crs.get(key, ranking_date_last, MDB_SET);
        crs.get(key, ranking_date_last, MDB_LAST_DUP);
        crs.get(key, ranking_date_prev, MDB_PREV_DUP);

        auto last_result = get<Ranking, RankingKey>
          (dbi_ranking, {kwd_id, eng_id, ranking_date_last});
        if(last_result._rank < 0) throw NotFoundException();

        auto prev_result = get<Ranking, RankingKey>
          (dbi_ranking, {kwd_id, eng_id, ranking_date_prev});
        if(prev_result._rank < 0) throw  NotFoundException();

        if(last) {
          *last = last_result;
        }

        if(prev) {
          *prev = prev_result;
        }

        return prev_result._rank - last_result._rank;
      } catch (NotFoundException) {
        // OK - no rankings available
        throw;
      } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "could not get last ranking for keyword: " << k.value() << std::endl;
        throw;
      }
    }

    Ranking DataProvider::best_ranking(const Keyword& k,
                                       ranktracker::engine::SearchEngine const &e) const {
      Ranking b;
      Cursor<ptime, KeywordEngine> crs(txn_ptr->get(), dbi_keywordranking);
      AbstractEntity::id_type kwd_id = k.id();
      AbstractEntity::id_type eng_id = e.id();
      KeywordEngine key = {kwd_id, eng_id};
      ptime ranking_date;

      try {
        BOOST_LOG_TRIVIAL(trace) << "reading first ranking for the keyword\n";
        crs.get(key, ranking_date, MDB_SET);
        b = get<Ranking, RankingKey>(dbi_ranking,
                                     {kwd_id, eng_id, ranking_date});
      } catch (NotFoundException) {
        BOOST_LOG_TRIVIAL(trace) << "no ranking found when trhing to get nest ranking";
        throw;
      }
      try {
      rankings_next:
        BOOST_LOG_TRIVIAL(trace) << "reading next ranking for the keyword\n";
        crs.get(key, ranking_date, MDB_NEXT_DUP);
        Ranking _b = get<Ranking, RankingKey>(dbi_ranking,
                                              {kwd_id, eng_id, ranking_date});
        if(b._rank < 0 || (_b._rank > -1 && _b._rank < b._rank)) {
          b = _b;
        }
        goto rankings_next;
      } catch (NotFoundException) {
        BOOST_LOG_TRIVIAL(trace) << "no more ranking for the keyword";
        // successfully read all available raking records
      }
      return b;
    }

    Ranking DataProvider::prev_ranking(const Keyword& k,
                                       ranktracker::engine::SearchEngine const &e,
                                       const Ranking& r) {
      Cursor<Ranking, RankingKey> crs(txn_ptr->get(), dbi_ranking);
      RankingKey crtk = {k.id(), e.id(), r._ranking_date}, prvk;
      Ranking prv;
      try {
        BOOST_LOG_TRIVIAL(trace) << "position to current ranking";
        crs.get(crtk, prv, MDB_SET);
        BOOST_LOG_TRIVIAL(trace) << "reading previous ranking";
        crs.get(prvk, prv, MDB_PREV);
      } catch(...) {
        BOOST_LOG_TRIVIAL(warning) << "failed to get previous raning";
        throw;
      }
      return prv;
    }

    void DataProvider::create_env() {
      using namespace std;

      int result;
      result = mdb_env_create(&db_env);

      if(result != 0) {
        BOOST_LOG_TRIVIAL(error) << "Error initializing database enviroment: " << result << endl;
        db_env = NULL;
        throw DataProviderException(DataProviderException::CREATE_ENV, result);
      }
    }

    void DataProvider::set_mapsize(unsigned int mapsize) {
      using namespace std;

      int result;
      result = mdb_env_set_mapsize(db_env, mapsize);

      if(result == EINVAL) {
        BOOST_LOG_TRIVIAL(error) << "Invalid value for parameter when setting the database map size: " << db_mapsize << endl;
        throw DataProviderException(DataProviderException::SET_MAP_SIZE, result);
      } else if(result != 0) {
        BOOST_LOG_TRIVIAL(error) << "Error trying to set the map size of database evironment: " << result << endl;
        throw DataProviderException(DataProviderException::SET_MAP_SIZE, result);
      }
    }

    /**
     * Increase the map size of the database environment. This will increase
     * the space to store data.
     */
    void DataProvider::increase_mapsize() {
      using namespace std;

      MDB_envinfo info;
      int result;
      result = mdb_env_info(db_env, &info);
      if(result != 0) {
        BOOST_LOG_TRIVIAL(error) << "Error getting environment information: " << result << endl;
        throw DataProviderException(DataProviderException::GET_ENV_INFO, result);
      }

      set_mapsize(info.me_mapsize + db_mapsize);
    }

    void DataProvider::set_maxdbs() {
      using namespace std;

      int result;
      result = mdb_env_set_maxdbs(db_env, maxdbs);
      if(result == EINVAL) {
        BOOST_LOG_TRIVIAL(error) << "Invalid parameter value when setting the maximum number of databases: " << 7 << endl;
        throw DataProviderException(DataProviderException::SET_MAXDBS, result);
      } else if(result != 0) {
        BOOST_LOG_TRIVIAL(error) << "Error setting the maximum number of databases: " << result;
        throw DataProviderException(DataProviderException::SET_MAXDBS, result);
      }
    }

    void DataProvider::env_open() {
      using namespace std;

      int result;
      result = open_app_db_environment(db_env);
      switch(result) {
      case 0:
        goto OPENENV_SUCCESS;
      case MDB_VERSION_MISMATCH:
        BOOST_LOG_TRIVIAL(error) << "The version of existing rank tracker database is not compatible with this version of rank tracker.\n";
        break;
      case MDB_INVALID:
        BOOST_LOG_TRIVIAL(error) << "The existing rank tracker database is corrupted and cannot be opened.\n";
        break;
      case ENOENT:
        BOOST_LOG_TRIVIAL(error) << "The application data folder does not exist.\n";
        break;
      case EACCES:
        BOOST_LOG_TRIVIAL(error) << "No permissions to access the rank tracker's database.\n";
        break;
      case EAGAIN:
        BOOST_LOG_TRIVIAL(error) << "The rank tracker's database is in use. Please try again.\n";
        break;
      default:
        BOOST_LOG_TRIVIAL(error) << "Unknown error when trying to open the rank tracker's database: " << result << endl;
      }
      throw DataProviderException(DataProviderException::OPEN_ENV, result);

    OPENENV_SUCCESS:
      return;
    }

    void DataProvider::open_db(const char *dbname,
                               unsigned int flags,
                               MDB_dbi *dbi) {
      using namespace std;

      int result = mdb_dbi_open(txn_ptr->get(), dbname, flags, dbi);
      switch(result) {
      case 0:
        // success
        goto rt_open_db_SUCCESS;
      case MDB_NOTFOUND:
        BOOST_LOG_TRIVIAL(error) << "Index not found when trying to open index " << dbname << endl;
        break;
      case MDB_DBS_FULL:
        BOOST_LOG_TRIVIAL(error) << "Too many indexes opened, trying to open index " << dbname << endl;
        break;
      default:
        BOOST_LOG_TRIVIAL(error) << "Unexpected error, trying to open index " << dbname << endl;
      }
      throw DataProviderException(DataProviderException::OPEN_DB, result);

    rt_open_db_SUCCESS:
      return;
    }

    db_txn::db_txn(DataProvider &db, unsigned int flags) {
      using namespace std;
      BOOST_LOG_TRIVIAL(trace) << "begin transaction     " << std::this_thread::get_id();
      int resize_count = 0;
    begin_trans_retry:
      int result = mdb_txn_begin(db.db_env, NULL, flags, &_txn);
      switch(result) {
      case 0:
        // everything is ok
        goto begin_trans_ok;
      case MDB_PANIC:
        BOOST_LOG_TRIVIAL(error) << "Fatal error accessing the database. The execution of the app cannot continue.\n";
        break;
      case MDB_MAP_RESIZED:
        if(resize_count++ > 0) {
          BOOST_LOG_TRIVIAL(error) << "Map resized too many times when begining new transaction.\n";
        } else {
          db.set_mapsize(0); // read the current size from disk
          goto begin_trans_retry;
        }
        break;
      case MDB_READERS_FULL:
        BOOST_LOG_TRIVIAL(error) << "Maximum number of readers exceeded when starting new transaction.\n";
        break;
      case ENOMEM:
        BOOST_LOG_TRIVIAL(error) << "Out of memory when starting new transaction\n";
        break;
      default:
        BOOST_LOG_TRIVIAL(error) << "Unexpectee error when startong new transaction: " << result << endl;
      }
      throw DataProviderException(DataProviderException::BEGIN_TRANS, result);
    begin_trans_ok:
      return;
    }

    db_txn::~db_txn() {
      if(_txn) {
        rollback();
      }
    }

    void db_txn::commit() {
      using namespace std;
      BOOST_LOG_TRIVIAL(trace) << "commit transaction    " << std::this_thread::get_id();
      if(!_txn) {
        BOOST_LOG_TRIVIAL(warning) << "commit called on not started (maybe closed) txn"
                                   << boost::stacktrace::stacktrace();
        return;
      }
      int result = mdb_txn_commit(_txn);
      _txn = NULL;
      switch(result) {
      case 0:
        // everything is ok
        goto commit_ok;
      case EINVAL:
        BOOST_LOG_TRIVIAL(error) << "Invalid parameter when calling commit on transaction\n";
        break;
      case ENOSPC:
        BOOST_LOG_TRIVIAL(error) << "Not enough disk space when executing commit on transaction\n";
        break;
      case EIO:
        BOOST_LOG_TRIVIAL(error) << "Low level IO error while writing data when executing commit on transaction\n";
      case ENOMEM:
        BOOST_LOG_TRIVIAL(error) << "Out of memory when executing commit on transaction\n";
      default:
        BOOST_LOG_TRIVIAL(error) << "Unexpected error when executing commit on transaction: " << result << endl;
      }
      throw DataProviderException(DataProviderException::COMMIT, result);
    commit_ok:
      return;
    }

    void db_txn::rollback() {
      BOOST_LOG_TRIVIAL(trace) << "rollback transaction  "
                               << std::this_thread::get_id()
                               << boost::stacktrace::stacktrace();
      if(!_txn) {
        BOOST_LOG_TRIVIAL(warning) << "rollback called on not started (maybe closed) txn"
                                   << boost::stacktrace::stacktrace();
        return;
      }
      mdb_txn_abort(_txn);
      _txn = NULL;
    }

    void DataProvider::begin_trans(unsigned int flags) {
      if(txn_ptr.get()) {
        BOOST_LOG_TRIVIAL(warning) << "begin_trans called but another transaction is already running; the old transaction will be rolled back"
                                   << boost::stacktrace::stacktrace();
      }
      txn_ptr.reset(new db_txn(*this, flags));
    }

    void DataProvider::commit() {
      auto _txn = txn_ptr.release();
      if(_txn == NULL) {
        BOOST_LOG_TRIVIAL(warning) << "transaction not initialized on current TLS in commit call; commit will be ignored."
                                   << boost::stacktrace::stacktrace();
      }
      _txn->commit();
    }

    void DataProvider::rollback() {
      auto _txn = txn_ptr.release();
      if(_txn == NULL) {
        BOOST_LOG_TRIVIAL(warning) << "transaction not initialized on current TLS in rollback call; commit will be ignored."
                                   << boost::stacktrace::stacktrace();
      }
      _txn->rollback();
    }

    void DataProvider::put(MDB_dbi dbi, MDB_val *k, MDB_val *v) {
      using namespace std;
      int result = mdb_put(txn_ptr->get(), dbi, k, v, 0);
      switch(result) {
      case 0:
        //everything is OK
        goto put_ok;
      case MDB_MAP_FULL:
        BOOST_LOG_TRIVIAL(error) << "The maximum database map size has been reached.\n";
        // we can't auto resize the database map here because setting new size requires
        // no transaction to be active; the calling code should catch this exception, call
        // database resize and retry the transaction
        throw DatabaseMapFullException(DataProviderException::PUT);
        break;
      case MDB_TXN_FULL:
        BOOST_LOG_TRIVIAL(error) << "The transaction has too many dirty pages and it wil be aborted, while executing put.\n";
        break;
      case EACCES:
        BOOST_LOG_TRIVIAL(error) << "Writing to database is not permited, while executing put.\n";
        break;
      case EINVAL:
        BOOST_LOG_TRIVIAL(error) << "Invalid parameter passed to mdb_put.\n";
        break;
      default:
        BOOST_LOG_TRIVIAL(error) << "Unexpected database error while executing put: " << result << endl;
      }
      throw DataProviderException(DataProviderException::PUT, result);
    put_ok:
      return;
    }

    void DataProvider::del(MDB_dbi dbi, MDB_val *k, MDB_val *v) {
      using namespace std;
      int result = mdb_del(txn_ptr->get(), dbi, k, v);
      switch(result) {
      case 0:
        //everything is OK
        goto put_ok;
      case MDB_NOTFOUND:
        throw NotFoundException();
      case EACCES:
        BOOST_LOG_TRIVIAL(error) << "Writing to database is not permited, while executing del.\n";
        break;
      case EINVAL:
        BOOST_LOG_TRIVIAL(error) << "Invalid parameter passed to mdb_del.\n";
        break;
      default:
        BOOST_LOG_TRIVIAL(error) << "Unexpected database error while executing del: " << result << endl;
      }
      throw DataProviderException(DataProviderException::DEL, result);
    put_ok:
      return;
    }

    MDB_val DataProvider::get(MDB_dbi dbi, const MDB_val &k) const {
      using namespace std;
      MDB_val data;
      int result = mdb_get(txn_ptr->get(), dbi, (MDB_val *)&k, &data);
      switch(result) {
      case 0:
        // everything is ok
        goto get_ok;
      case MDB_NOTFOUND:
        throw NotFoundException();
      case EINVAL:
        BOOST_LOG_TRIVIAL(error) << "Invalid parameter specified in call of databse's mdb_get function.\n";
        break;
      default:
        BOOST_LOG_TRIVIAL(error) << "Unexpected database error when executing mdb_get: " << result << endl;
      }
      throw DataProviderException(DataProviderException::GET, result);
    get_ok:
      return data;
    }

    struct MDB_heap_val : public MDB_val {
      virtual ~MDB_heap_val() {
        if(mv_data) {
          delete[] (char *)mv_data;
        }
      }

      MDB_heap_val() {
        mv_size = 0;
        mv_data = NULL;
      }

      MDB_heap_val(const MDB_heap_val &v) = delete;
      MDB_heap_val & operator= (const MDB_heap_val &v) = delete;

      MDB_heap_val(MDB_heap_val &&v) : MDB_val(v) {
        v.mv_size = 0;
        v.mv_data = NULL;
      }


      MDB_heap_val & operator= (MDB_heap_val &&v) {
        mv_size = v.mv_size;
        mv_data = v.mv_data;
        v.mv_size = 0;
        v.mv_data = NULL;
        return *this;
      }
    };

    template<class Data>
    MDB_heap_val write_binary(const Data& data) {
      std::stringbuf buf(std::ios_base::out|std::ios_base::binary);
      boost::archive::binary_oarchive ar(buf);
      ar << data;

      MDB_heap_val val;
      auto str = buf.str();

      val.mv_size = str.size();
      val.mv_data = new char[val.mv_size];
      std::memcpy(val.mv_data, str.data(), val.mv_size);

      return val;
    }

    template<class Data>
    void read_binary(Data& data, MDB_val& val) {
      std::stringbuf valbuf(std::string((char *)val.mv_data, val.mv_size), std::ios_base::in | std::ios_base::binary);
      boost::archive::binary_iarchive valar(valbuf);
      valar >> data;
    }


    template<class E, class K>
    void DataProvider::put(MDB_dbi dbi, const K &k, const E &e) {
      MDB_heap_val key = write_binary<K>(k);
      MDB_heap_val val = write_binary<E>(e);

      put(dbi, (MDB_val *)&key, (MDB_val *)&val);
    }

    template<class E, class K>
    E DataProvider::get(MDB_dbi dbi, const K &k) const {
      MDB_heap_val key = write_binary<K>(k);
      MDB_val val = get(dbi, key);

      E e;
      read_binary<E>(e, (MDB_val &)val);
      return e;
    }

    template<class E, class K>
    void DataProvider::del(MDB_dbi dbi, const K &k, const E &e) {
      MDB_heap_val key = write_binary<K>(k);
      MDB_heap_val val = write_binary<E>(e);

      del(dbi, (MDB_val *)&key, (MDB_val *)&val);
    }

    template<class K>
    void DataProvider::del(MDB_dbi dbi, const K &k) {
      MDB_heap_val key = write_binary<K>(k);

      del(dbi, (MDB_val *)&key, (MDB_val *)NULL);
    }

    template<class DataObject, class Key>
    void Cursor<DataObject, Key>::get(MDB_val *key, MDB_val *val, MDB_cursor_op op) {
      using namespace std;
      int result = mdb_cursor_get(_cursor, key, val, op);
      switch(result) {
      case 0:
        // everithing is ok;
        goto cursor_get_ok;
      case MDB_NOTFOUND:
        throw NotFoundException();
      default:
        BOOST_LOG_TRIVIAL(error) << "Unexepected error while reading from database cursor: " << result << endl;
      }
      throw DataProviderException(DataProviderException::CURSOR_GET, result);
    cursor_get_ok:
      return;
    }

    template<class DataObject, class Key>
    void Cursor<DataObject, Key>::del() {
      using namespace std;
      int result = mdb_cursor_del(_cursor, 0);
      switch(result) {
      case 0:
        // everything is ok
        goto cursor_del_ok;
      case EACCES:
        BOOST_LOG_TRIVIAL(error) << "an attempt was made to write in a read-only transaction in the call of mdb_cursor_del()";
        break;
      case EINVAL:
        BOOST_LOG_TRIVIAL(error) << "an invalid parameter was specified in the call of mdb_cursor_del()";
        break;
      default:
        BOOST_LOG_TRIVIAL(error) << "Unexpected error in the call of mdb_cursor_del()";
      }
      throw DataProviderException(DataProviderException::CURSOR_DEL, result);
    cursor_del_ok:
      return;
    }

    template<class DataObject, class Key>
    void Cursor<DataObject, Key>::get(Key &key, DataObject& data, MDB_cursor_op op) {
      using namespace std;
      MDB_heap_val bink = write_binary<Key>(key);
      MDB_heap_val binv = write_binary<DataObject>(data);

      // get will be able to change the mv_data pointer in the
      // key and value, so i make a copy  such bink and binv will
      // correctly manage the allocated memory
      MDB_val resk = bink, resv = binv;
      get(&resk, &resv, op);

      read_binary<Key>(key, resk);
      read_binary<DataObject>(data, resv);
    }

    template<class DataObject, class Key>
    size_t Cursor<DataObject, Key>::count() {
      using namespace std;
      size_t c;
      int res = mdb_cursor_count(_cursor, &c);
      switch(res) {
      case 0:
        // everithing is ok;
        goto cursor_count_ok;
      case EINVAL:
        BOOST_LOG_TRIVIAL(error) << "Cursor is not initialized or other invalid parameter on mdb_cursor_count\n";
      default:
        BOOST_LOG_TRIVIAL(error) << "Unexepected error on mdb_cursor_count: " << res << endl;
      }
      throw DataProviderException(DataProviderException::CURSOR_GET, res);
    cursor_count_ok:
      return c;
    }
  }
}
