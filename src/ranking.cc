/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// ranknig.cc
// operations to retrieve the ranking of different domains (web pages)
// related to keywords and write this information into the persistent storage (database)

#include "ranking.hh"

namespace ranktracker {
  namespace ranking {

    void RankingService::refresh_ranking(const Keyword& k, const Domain& d, progress_updater p) {
      BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(keyword, domain) called\n";
      BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(): "
                               << "domain '" << d.name() << "', keywords '" << k.value() << "'\n";
      auto const engines = d.engines();
      int crt_progress = 0;
      progress_updater _p(tracking_progress_updater(crt_progress, p)); // _p updates ctr_progress
      auto ranking_date = second_clock::local_time();

      for(auto i = engines.begin(); i != engines.end(); i++) {
        std::string page_url = "";
        auto const &e = *i;
        BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(): "
                                 << "perfomm ranking query for engine: "
                                 << e.name() << std::endl;
        BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(keyword, domain) current progress = "
                                 << crt_progress;
        // __p receives values between 1 and 100
        progress_updater __p(offset_progress_updater(crt_progress, _p));
        auto rank = e.perform_rank_query(d.name(), k.value(), __p, &page_url);
        BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(keyword, domain) save storing to db\n";
        _db.storeRanking(k, *e, {ranking_date, rank, page_url});
        BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(keyword, domain) storing saved to db\n";
        BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(keyword, domain) exit\n";
      }
    }

    void RankingService::refresh_ranking(const Domain& d, progress_updater p) {
      BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(domain) called\n";
      BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(): "
                               << "refreshing rankings for all keywords of the domain '"
                               << d.name() << "'\n";
      auto const ks = _db.keywords(d);
      int crt_progress = 0;
      progress_updater _p (tracking_progress_updater(crt_progress, p));
      for(auto i = ks.begin(); i != ks.end(); i++) {
        BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(domain) crt_progress = "
                                 << crt_progress << std::endl;
        progress_updater __p(offset_progress_updater(crt_progress, _p));
        refresh_ranking(*i, d, __p);
      }
      BOOST_LOG_TRIVIAL(trace) << "RankingService::refresh_ranking(domain) exit\n";
    }
  }
}
