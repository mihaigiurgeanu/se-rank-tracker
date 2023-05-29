/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// rank_url_table.cc
// The table displaying information about the ranking data in the graph view.

#include "rank_url_table.hh"
#include <sstream>
#include "logging.hh"
#include <FL/fl_draw.H>

namespace ranktracker {
  namespace ui {

    void
    RankURLTable::draw_cell(TableContext context, int R, int C,
                            int X, int Y, int W, int H) {
      static const char * const headers[] = { "Current Rank",
                                              "Previous Rank",
                                              "Change",
                                              "Best",
                                              "Update Time",
                                              "Ranking URL"      };
      using namespace std;
      BOOST_LOG_TRIVIAL(trace) << "displaying ranking info: "
                               << R << ", " << C << ", " << X << ", "
                               << Y << ", " << W << ", " << H << endl;

      switch(context) {
      case CONTEXT_STARTPAGE:
        fl_font(FL_HELVETICA, 12);
        break;

      case CONTEXT_COL_HEADER:
        fl_push_clip(X, Y, W, H);
        fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, color());
        fl_color(FL_BLACK);
        if(C < sizeof headers / sizeof(char *)) {
          fl_draw(headers[C], X, Y, W, H, FL_ALIGN_CENTER);
        }
        fl_pop_clip();
        break;

      case CONTEXT_CELL:
        fl_push_clip(X, Y, W, H);
        // BG COLOR
        fl_color(row_selected(R) ? selection_color() : FL_WHITE);
        fl_rectf(X, Y, W, H);

        // TEXT
        fl_color(FL_BLACK);
        draw_cell_text(R, C, X, Y, W, H);

        // BORDER
        fl_color(FL_LIGHT2);
        fl_rect(X, Y, W, H);

        fl_pop_clip();
        break;

      default:
        break;
      }
    }

    void RankURLTable::draw_cell_text(int R, int C, int X, int Y, int W, int H) {
      if(R > 0 || !_rank_info) {
        fl_draw("N/A", X, Y, W, H, FL_ALIGN_CENTER);
      } else {
        std::ostringstream txt;
        switch(C) {
        case 0:
          txt << _rank_info->_rank;
          fl_draw(txt.str().c_str(), X, Y, W, H, FL_ALIGN_CENTER);
          break;
        case 1:
          if(_prev_rank_info) {
            txt << _prev_rank_info->_rank;
          } else {
            txt << "N/A";
          }
          fl_draw(txt.str().c_str(), X, Y, W, H, FL_ALIGN_CENTER);
          break;
        case 2:
          if(_prev_rank_info) {
            txt << _prev_rank_info->_rank - _rank_info->_rank;
          } else {
            txt << "N/A";
          }
          fl_draw(txt.str().c_str(), X, Y, W, H, FL_ALIGN_CENTER);
          break;
        case 3:
          if(_best_rank_info) {
            txt << _best_rank_info->_rank;
          } else {
            txt << "N/A";
          }
          fl_draw(txt.str().c_str(), X, Y, W, H, FL_ALIGN_CENTER);
          break;
        case 4:
          txt << _rank_info->_ranking_date;
          fl_draw(txt.str().c_str(), X, Y, W, H, FL_ALIGN_CENTER);
          break;
        case 5:
          txt << _rank_info->_page_url;
          fl_draw(txt.str().c_str(), X+5, Y, W, H, FL_ALIGN_LEFT);
          break;
        default:
          txt << "N/A";
          fl_draw(txt.str().c_str(), X, Y, W, H, FL_ALIGN_CENTER);
          break;
        }
      }
    }
  }
}
