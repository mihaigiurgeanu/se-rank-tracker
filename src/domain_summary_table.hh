/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// domain_summary_table.hh
// Table widget to display the domain summary

#ifndef RANKTRACKER_DOMAIN_SUMMARY_TABLE_H
#define RANKTRACKER_DOMAIN_SUMMARY_TABLE_H

#include <FL/Fl.H>
#include <FL/Fl_Table_Row.H>
#include <FL/fl_ask.H>
#include "data_model.hh"
#include "data_provider.hh"

namespace ranktracker {
  namespace ui {
    using namespace ranktracker::data;
    using namespace ranktracker::persistence;

    class DomainSummaryTable : public Fl_Table_Row {
      DataProvider *_db;
      const Domain *_domain;
      std::vector<Keyword> _keywords;
      std::vector<SearchEngineRef> _engines;

      /** sets the column width according to the user preferences */
      void set_preferred_col_width(int col);

      /** save the preferred column width in user preferences database */
      void save_preferred_col_width(int col);

    protected:
      void  draw_cell(TableContext context, int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0);
      int handle(int event);
    public:
      DomainSummaryTable(int x, int y, int w, int h, const char *l = 0) :
        Fl_Table_Row(x, y, w, h, l),
        _db(NULL),
        _domain(NULL)
      {
        col_header(1);
        row_header(1);
        //row_header_width(200);
        col_resize(1);
      }

      void database(DataProvider *db) { _db = db; }
      void domain(const Domain *domain);
      void refresh_view();

    public:
      const Keyword& keyword(int idx) const;
      const engine::SearchEngineRef& engine(int idx) const;
    };

    class KeywordIdxNotValid {};
    class EngineIdxNotValid {};
  }
}

#endif
