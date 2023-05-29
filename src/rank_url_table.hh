/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// rank_url_table.hh
// implements table describing the ranking data of a point in the graph

#ifndef RANKTRACKER_UI_RANK_URL_TABLE_HH
#define RANKTRACKER_UI_RANK_URL_TABLE_HH

#include <FL/Fl.H>
#include <FL/Fl_Table_Row.H>
#include "data_model.hh"

namespace ranktracker {
  namespace ui {
    class RankURLTable : public Fl_Table_Row {
      const ranktracker::data::Ranking *_rank_info, *_prev_rank_info, *_best_rank_info;
    protected:
      virtual void draw_cell(TableContext context,
                             int R = 0,
                             int C = 0,
                             int X = 0,
                             int Y = 0,
                             int W = 0,
                             int H = 0);
    public:
      RankURLTable(int x, int y, int w, int h, const char *l = 0) :
        Fl_Table_Row(x, y, w, h, l), _rank_info(nullptr),
        _prev_rank_info(nullptr),
        _best_rank_info(nullptr)
      {}

      void rank_info(const ranktracker::data::Ranking *ri) {
        _rank_info = ri;
      }

      const ranktracker::data::Ranking * rank_info() const {
        return _rank_info;
      }

      void prev_rank_info(const ranktracker::data::Ranking *ri) {
        _prev_rank_info = ri;
      }

      const ranktracker::data::Ranking * prev_rank_info() const {
        return _prev_rank_info;
      }

      void best_rank_info(const ranktracker::data::Ranking *ri) {
        _best_rank_info = ri;
      }

      const ranktracker::data::Ranking * best_rank_info() const {
        return _best_rank_info;
      }
    private:
      // internal function to display the cell text
      void draw_cell_text(int R, int C, int X, int Y, int W, int H);
    };
  }
}

#endif // RANKTRACKER_UI_RANK_URL_TABLE_HH
