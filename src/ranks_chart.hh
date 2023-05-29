/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// ranks_chart.hh
// Specialization of the Chart class for ranking.
// It displays a green horizontal line at rank 5.

#include "chart.hh"
#include "colors.hh"

#ifndef RANKTRACKER_RANKS_CHART_H
#define RANKTRACKER_RANKS_CHART_H

namespace ranktracker {
  namespace ui {

    template<class Entry_Data>
    class RanksChart: public Chart<Entry_Data> {
    public:
      RanksChart(int X, int Y, int W, int H, const char *L = 0) : Chart<Entry_Data>(X, Y, W, H, L) {}

    protected:
      void draw() override {
        Chart<Entry_Data>::draw();

        Fl_Boxtype b = Fl_Widget::box();
        int xx = Fl_Widget::x()+Fl::box_dx(b);
        int yy = Fl_Widget::y()+Fl::box_dy(b);
        int ww = Fl_Widget::w()-Fl::box_dw(b);
        int hh = Fl_Widget::h()-Fl::box_dh(b);
        fl_push_clip(xx, yy, ww, hh);

        ww--; hh--; // adjust for line thickness
        fl_color(green);
        fl_line_style(FL_SOLID);
        double domain_y = 90; // 90 is the rank 10
        int screen_y = hh + yy
          - (domain_y - Chart<Entry_Data>::_miny)
          * (hh - Chart<Entry_Data>::_xlblh)
          / (Chart<Entry_Data>::_maxy - Chart<Entry_Data>::_miny)
          - Chart<Entry_Data>::_xlblh;

        fl_draw("10",
                xx + Chart<Entry_Data>::padding,
                screen_y - (Chart<Entry_Data>::_ylblh/2) + Chart<Entry_Data>::padding,
                Chart<Entry_Data>::_ylblw - 2*Chart<Entry_Data>::padding,
                Chart<Entry_Data>::_ylblh,
                FL_ALIGN_RIGHT);
        fl_line(xx + Chart<Entry_Data>::_ylblw, screen_y, xx + ww, screen_y);
        fl_pop_clip();
      }
    };

  }
}

#endif
