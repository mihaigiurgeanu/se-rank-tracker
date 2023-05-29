/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// domain_summry_table.cc

#include "domain_summary_table.hh"
#include "preferences.h"
#include "colors.hh"

#include <FL/fl_draw.H>
#include <sstream>
#include <sstream>
#include <algorithm>
#include <boost/uuid/uuid_io.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace ranktracker {
  namespace ui {

    bool keyword_name_comp(const Keyword& k1, const Keyword& k2) {
      return k1.value() < k2.value();
    }

    class keyword_ranking_comp_base {
    protected:
      const DataProvider &_db;
      const SearchEngineRef &_e;
    public:
      keyword_ranking_comp_base(const DataProvider &db, const SearchEngineRef &e) :
        _db(db), _e(e)
      {}
    };

    class keyword_rank_comp : public keyword_ranking_comp_base {
    public:
      keyword_rank_comp(const DataProvider &db, const SearchEngineRef &e) :
        keyword_ranking_comp_base(db, e)
      {}

      bool operator() (const Keyword& k1, const Keyword& k2) {
        Ranking r1, r2;
        bool r1_found, r2_found;
        try {
          r1 = _db.last_ranking(k1, *_e);
          r1_found = true;
        } catch (NotFoundException) {
          r1_found = false;
        }
        try {
          r2 = _db.last_ranking(k2, *_e);
          r2_found = true;
        } catch (NotFoundException) {
          r2_found = false;
        }
        if(r1_found && r2_found) {
          if (r1._rank >= 0 && r2._rank >= 0) {
            return r1._rank < r2._rank;
          }
          return (r1._rank >= 0);
        }
        return r1_found;
      }
    };

    class keyword_rankdiff_comp : public keyword_ranking_comp_base {
    public:
      keyword_rankdiff_comp(const DataProvider &db, const SearchEngineRef &e) :
        keyword_ranking_comp_base(db, e)
      {}

      bool operator() (const Keyword& k1, const Keyword& k2) {
        ranktracker::engine::rank_result_type d1;
        ranktracker::engine::rank_result_type d2;

        try {
          d1 = _db.diff_ranking(k1, *_e);
        } catch (NotFoundException) {
          d1 = -1;
        }

        try {
          d2 = _db.diff_ranking(k2, *_e);
        } catch (NotFoundException) {
          d2 = -1;
        }

        return d1 < d2;
      }
    };

    void DomainSummaryTable::domain(const ranktracker::data::Domain *domain) {
      _domain = domain;
      refresh_view();
    }

    void DomainSummaryTable::refresh_view() {
      _engines.clear();
      if(_domain) {
        cols(2 * _domain->engines().size() + 1);
        set_preferred_col_width(0);
        int crt_col = 1;
        for(auto i = _domain->engines().cbegin();
            i != _domain->engines().cend();
            i++) {
          _engines.push_back(*i);
          set_preferred_col_width(crt_col++);
          set_preferred_col_width(crt_col++);
        }
        if(_db) {
          try {
            create_transaction transaction(_db, MDB_RDONLY);
            _keywords = _db->keywords(*_domain);
            transaction.commit();
          } catch (...) {
            _keywords.clear();
            fl_alert("Failed to load the keywords for domain: %s",
                     _domain->name().c_str());
          }
        } else {
          _keywords.clear();
        }
        rows(_keywords.size());
      }

      redraw();
    }

    void DomainSummaryTable::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) {
      std::ostringstream s;

      switch ( context ) {
      case CONTEXT_STARTPAGE:             // Fl_Table telling us it's starting to draw page
        fl_font(FL_HELVETICA, 16);
        BOOST_LOG_TRIVIAL(trace) << "today is " << boost::gregorian::day_clock::local_day();
        return;

      case CONTEXT_ROW_HEADER:            // Fl_Table telling us to draw row/col headers
        s << "@#-9circle";
        fl_push_clip(X, Y, W, H);
        {
          fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, color());
          if(_engines.size() > 0) {
            using boost::gregorian::day_clock;
            using boost::gregorian::date;
            try {
              create_transaction t(_db, MDB_RDONLY);
              date last_date = _db->last_ranking(_keywords[R],
                                                 *_engines[0])._ranking_date.date();
              date today = day_clock::local_day();
              BOOST_LOG_TRIVIAL(trace) << "Last ranking date for <"
                                       << _keywords[R].value()
                                       << ">: " << last_date;
              if(last_date < today) {
                BOOST_LOG_TRIVIAL(trace) << "last date is less than today";
                fl_color(red);
              } else {
                BOOST_LOG_TRIVIAL(trace) << "last date is today";
                fl_color(green);
              }
              t.commit();
            } catch (...) {
              fl_color(red);
            }
          }

          fl_draw(s.str().c_str(), X, Y, W, H, FL_ALIGN_CENTER);
        }
        fl_pop_clip();
        return;
      case CONTEXT_COL_HEADER:
        if(C == 0) {
          s << "Keywords";
        } else if(_domain && (C-1) < 2*(int)_domain->engines().size()) {
          if((C-1) % 2 == 0) {
            s << _engines[(C-1)/2].name();
          } else {
            s << "Diff";
          }
        }
        fl_push_clip(X, Y, W, H);
        {
          fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, color());
          fl_color(FL_BLACK);
          fl_draw(s.str().c_str(), X+2, Y, W-2, H, C==0?FL_ALIGN_LEFT:FL_ALIGN_CENTER);
        }
        fl_pop_clip();
        return;

      case CONTEXT_CELL:                  // Fl_Table telling us to draw cells
        fl_push_clip(X, Y, W, H);
        {
          // BG COLOR
          fl_color( row_selected(R) ? selection_color() : FL_WHITE);
          fl_rectf(X, Y, W, H);

          // TEXT
          fl_color(FL_BLACK);

          if(_domain && C == 0 && R < (int)_keywords.size()) {
            s << _keywords[R].value();
          } else if(_domain && (C-1) % 2 == 0 && (C-1)/2 < (int)_engines.size() && R < (int)_keywords.size()) {
            try {
              create_transaction trans(_db, MDB_RDONLY);
              Ranking r = _db->last_ranking(_keywords[R], *(_engines[(C-1)/2]));
              trans.commit();

              if(r._rank < 0) {
                s << ">100";
              } else {
                s << r._rank;
              }
              if(r._rank < 0) {
                fl_color(FL_WHITE);
              } else if(r._rank <= 10) {
                fl_color(green);
              } else if(r._rank <= 30) {
                fl_color(yellow);
              } else {
                fl_color(orange);
              }
              fl_rectf(X, Y, W, H);
              fl_color(FL_BLACK);
            } catch (...) {
              s << "N/A";
            }
          } else if(_domain && (C-1) % 2 == 1 && (C-1)/2 < (int)_domain->engines().size()) {
            try {
              create_transaction trans(_db, MDB_RDONLY);
              auto diff = _db->diff_ranking(_keywords[R], *(_engines[(C-1)/2]));
              trans.commit();
              s << diff;
              if(diff > 0) {
                fl_color(FL_GREEN);
              } else if (diff < 0) {
                fl_color(FL_RED);
              }
            } catch (...) {
              s << "-";
            }
          }

          fl_draw(s.str().c_str(), X+2, Y, W-2, H, C==0?FL_ALIGN_LEFT:FL_ALIGN_CENTER);

          // BORDER
          fl_color(FL_LIGHT2);
          fl_rect(X, Y, W, H);
        }
        fl_pop_clip();
        return;

      default:
        return;
      }
      //NOTREACHED
    }

    std::string make_col_width_pref_key(AbstractEntity::id_type engine_id) {
      std::ostringstream key;
      key << "DomainTable-DataColumnWidth-" << engine_id;
      return key.str();
    }
    void DomainSummaryTable::set_preferred_col_width(int col) {
      BOOST_LOG_TRIVIAL(trace) << "loading column width for col " << col << std::endl;
      if(col == 0) {
        int width = get_int_pref("DomainTable-RowHeaderWidth");
        if (width > 0) {
          BOOST_LOG_TRIVIAL(trace) << "loading col width for col " << col << ", width " << width << std::endl;
          col_width(col, width);
        } else {
          BOOST_LOG_TRIVIAL(trace) << "no column width saved for col " << col << std::endl;
        }
      } else if((col-1) % 2 == 0) {
        std::string key = make_col_width_pref_key(_engines[(col-1)/2].id());
        int width = get_int_pref(key.c_str());
        if (width > 0) {
          BOOST_LOG_TRIVIAL(trace) << "loading col width for col " << col << ", width " << width << std::endl;
          col_width(col, width);
        }  else {
          BOOST_LOG_TRIVIAL(trace) << "no column width saved for col " << col << std::endl;
        }
      }
    }

    void DomainSummaryTable::save_preferred_col_width(int col) {
       BOOST_LOG_TRIVIAL(trace) << "saving column width for col " << col
                               << "; " << col_width(col) << std::endl;
      if(col == 0) {
        set_int_pref("DomainTable-RowHeaderWidth", col_width(col));
        BOOST_LOG_TRIVIAL(trace) << "saved col width for col " << col << ", width " << col_width(col) << std::endl;
      } else if((col-1) % 2 == 0) {
        std::string key = make_col_width_pref_key(_engines[(col-1)/2].id());
        set_int_pref(key.c_str(), col_width(col));
        BOOST_LOG_TRIVIAL(trace) << "saved col width for col " << col << ", width " << col_width(col) << std::endl;
      }
    }

    int DomainSummaryTable::handle(int event) {
      int ret = Fl_Table_Row::handle(event);
      int R, C;
      ResizeFlag resizeflag;
      TableContext ctx;

      switch(event) {
      case FL_DRAG:
        if (event == FL_DRAG && is_interactive_resize()) {
          cursor2rowcol(R, C, resizeflag);
          switch(resizeflag) {
          case RESIZE_COL_LEFT:
            save_preferred_col_width(C-1);
            break;
          case RESIZE_COL_RIGHT:
            save_preferred_col_width(C);
            break;
          default:
            break;
          }
        }
        break;
      case FL_RELEASE:
        // is click on header?
        ctx = cursor2rowcol(R, C, resizeflag);
        if(!resizeflag && ctx == CONTEXT_COL_HEADER) {
          BOOST_LOG_TRIVIAL(trace) << "TRACE: Sorting or col " << C << std::endl;
          if(C==0) {
            if(std::is_sorted(_keywords.begin(), _keywords.end(), keyword_name_comp)) {
              std::reverse(_keywords.begin(), _keywords.end());
            } else {
              std::sort(_keywords.begin(), _keywords.end(), keyword_name_comp);
            }
          } else if((C-1) % 2 == 0) {
            try {
              create_transaction t(_db, MDB_RDONLY);
              keyword_rank_comp rank_comp(*_db, _engines[(C-1)/2]);
              if(std::is_sorted(_keywords.begin(), _keywords.end(), rank_comp)) {
                std::reverse(_keywords.begin(), _keywords.end());
              } else {
                std::sort(_keywords.begin(), _keywords.end(), rank_comp);
              }
              t.commit();
            } catch (...) {
              BOOST_LOG_TRIVIAL(error) << "Sort failed\n";
            }
          } else {
            try {
              create_transaction t(_db, MDB_RDONLY);
              keyword_rankdiff_comp diff_comp(*_db, _engines[(C-1)/2]);
              if(std::is_sorted(_keywords.begin(), _keywords.end(), diff_comp)) {
                std::reverse(_keywords.begin(), _keywords.end());
              } else {
                std::sort(_keywords.begin(), _keywords.end(), diff_comp);
              }
              t.commit();
            } catch (...) {
              BOOST_LOG_TRIVIAL(error) << "Sort failed\n";
            }
          }

          redraw();
        }
      }
      return ret;
    }

    const Keyword& DomainSummaryTable::keyword(int idx) const {
      if (idx >= 0 || idx < _keywords.size()) {
        return _keywords[idx];
      }

      throw KeywordIdxNotValid();
    }

    const engine::SearchEngineRef& DomainSummaryTable::engine(int idx) const {
      if(idx >= 0 && idx < _engines.size()) {
        return _engines[idx];
      }

      throw EngineIdxNotValid();
    }
  }
}
