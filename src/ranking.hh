/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// ranking.hh
// high level functions to refresh the rankings on domains and
// keywords

#ifndef RANCKTRACKER_RANKING_H
#define RANCKTRACKER_RANKING_H

#include "data_provider.hh"
#include "progress.hh"

namespace ranktracker {
  namespace ranking {
    using namespace ranktracker::data;
    using namespace ranktracker::persistence;
    using namespace ranktracker::progress;

    class RankingService {
      DataProvider& _db;

    public:

      RankingService(DataProvider& db) : _db(db) {}

      /**
       * The basic functionality defined in updating the
       * ranking of a keyword in a given domain. It requests
       * the rank for the keyword for every search engine
       * configured for the domain and updates the ranking
       * into the database.
       *
       * The function does not check if the given `Keyword`
       * object is configured for the given `Domain` object.
       * You must pass the correct `Domain` for the object or
       * the ranking will not be correctly performed, i.e. the
       * ranking will take into consideration the position of
       * the given `Domain`, but will store into the database for
       * the `Domain` the `Keyword` was actually configured for.
       */
      void refresh_ranking(const Keyword& k, const Domain& d, progress_updater p);

      /**
       * Check online ranking for all the keywords of a domain,
       * by calling `refresh_ranking(k, d)` for each keyword k
       * contained in a domain d.
       */
      void refresh_ranking(const Domain& d,  progress_updater p);

      /**
       * Check  the online rankings of a list of domains by
       * calling `refresh_ranking(d)` for each domain d in the
       * list of domains, ds.
       * `DomainsList` is the container of the `Domain` objects
       * supporting iterator operations.
       */
      template <class DomainsList>
      void refresh_ranking(const DomainsList& ds, progress_updater p) {
        BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(domains list) called\n";
        int crt_progress = 0;
        progress_updater _p(tracking_progress_updater(crt_progress, p));
        for(auto i = ds.cbegin(); i != ds.cend(); i++) {
          BOOST_LOG_TRIVIAL(trace)  << "RankingService::refresh_ranking(domains list) crt_progress = "
                                    << crt_progress << std::endl;
          progress_updater __p(offset_progress_updater(crt_progress, _p));
          BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(domains list) refresh domain ranking\n";
          refresh_ranking(*i, __p);
        }
        BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(domains list) exit\n";
      }
    };
  }
}

#endif
