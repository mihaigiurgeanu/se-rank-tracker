/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// chart.hh
// definition of the chart widget
// extends Fl_Chart to add specific drawing requirements

#ifndef RANKTRACKER_CHART_H
#define RANKTRACKER_CHART_H

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Tooltip.H>

#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <cmath>

#include "logging.hh"


namespace ranktracker {
  namespace ui {

    //////////////////////////////////////////////////
    // Chart_Entry
    //////////////////////////////////////////////////
    template<class Entry_Data>
    struct Chart_Entry {
      double x, y;
      std::string point_label;
      Entry_Data data;
    public:
      Chart_Entry(double x, double y,
                  std::string point_label,
                  Entry_Data data) :
        x(x), y(y),
        point_label(std::move(point_label)),
        data(data)
      {}

      Chart_Entry(Chart_Entry&& e) :
        x(e.x), y(e.y),
        point_label(std::move(e.point_label)),
        data(e.data)
      {}

      Chart_Entry& operator= (Chart_Entry&& e) {
        x = e.x;
        y = e.y;
        point_label = std::move(e.point_label);
        data = e.data;

        return *this;
      }
    };
    ///////////////////////////////////////////////////

    inline std::string make_default_label(double val) {
      std::ostringstream s;
      s << val;
      return s.str();
    }

    inline double default_closest_point_for_label(double x) {
      return x;
    }

    /////////////////////////////////////////////////////
    // Chart class
    /////////////////////////////////////////////////////
    template<class Entry_Data>
    class Chart : public Fl_Widget {
      std::vector<Chart_Entry<Entry_Data>> _entries;

    protected:
      int _xlblh, _xlblw,
        _ylblh, _ylblw;
      double _minx, _maxx, _miny, _maxy;

      const Fl_Font font = FL_HELVETICA;
      const int font_size = 10;
      const int padding = 3;

    private:
      std::function<std::string(double)> _make_x_label_f, _make_y_label_f;
      std::function<double(double)> _closest_x_for_label, _closest_y_for_label;

      bool _horizontal_grid, _vertical_grid;

      Chart_Entry<Entry_Data> *_point_under_mouse, *_release_point;
    public:
      Chart(int X, int Y, int W, int H, const char *L = 0) :
        Fl_Widget(X, Y, W, H, L),
        _xlblh(0), _xlblw(0),
        _ylblh(0), _ylblw(0),
        _minx(0), _maxx(0),
        _miny(0), _maxy(0),
        _make_x_label_f(make_default_label),
        _make_y_label_f(make_default_label),
        _point_under_mouse(NULL),
        _closest_y_for_label(default_closest_point_for_label),
        _closest_x_for_label(default_closest_point_for_label),
        _horizontal_grid(false), _vertical_grid(false)
      {}

      void make_x_label_f(std::function<std::string(int)> F) {
        _make_x_label_f = F;
      }

      void make_y_label_f(std::function<std::string(int)> F) {
        _make_y_label_f = F;
      }


      void closest_x_for_label(std::function<double(double)> f) {
        _closest_x_for_label = f;
      }

      void closest_y_for_label(std::function<double(double)> f) {
        _closest_y_for_label = f;
      }

      void horizontal_grid(bool v) { _horizontal_grid = v; }
      void vertical_grid(bool v) { _vertical_grid = v; }
      bool horizontal_grid() { return _horizontal_grid; }
      bool vertical_grid() { return _vertical_grid; }

      /**
       * callback can access the point that was pushed through this.
       */
      Chart_Entry<Entry_Data> *release_point() const { return _release_point; }
    protected:
      void draw() override {
        draw_box();
        Fl_Boxtype b = box();
        int xx = x()+Fl::box_dx(b);
        int yy = y()+Fl::box_dy(b);
        int ww = w()-Fl::box_dw(b);
        int hh = h()-Fl::box_dh(b);
        fl_push_clip(xx, yy, ww, hh);

        ww--; hh--; // adjust for line thickness
        fl_color(FL_WHITE);
        fl_rectf(xx, yy, ww, hh);
        draw_axes(xx, yy, ww, hh);
        draw_labels(xx, yy, ww, hh);
        draw_lines(xx, yy, ww, hh);
        draw_points(xx, yy, ww, hh);

        draw_label();
        fl_pop_clip();
      }
    public:
      void minx(double x) { _minx = x; }
      void miny(double y) { _miny = y; }
      void maxx(double x) { _maxx = x; }
      void maxy(double y) { _maxy = y; }

      double minx() { return _minx; }
      double miny() { return _miny; }
      double maxx() { return _maxx; }
      double maxy() { return _maxy; }

      void add(Chart_Entry<Entry_Data> e) {
        _entries.emplace_back(std::move(e));
        if(_entries.size() >= 1) {
          if(e.x > _maxx) _maxx = e.x;
          if(e.x < _minx) _minx = e.x;
          if(e.y > _maxy) _maxy = e.y;
          if(e.y < _miny) _miny = e.y;
        } else {
          _minx = _maxx = e.x;
          _miny = _maxy = e.y;
        }
      }

      void clear() {
        _entries.clear();
        _minx = _maxx = _miny = _maxy = 0;
        _point_under_mouse = NULL;
      }

      void widths(int xlblw, int xlblh, int ylblw, int ylblh) {
        int padding_2 = 2*padding;

        _xlblw = xlblw + padding_2;
        _xlblh = xlblh + padding_2;
        _ylblw = ylblw + padding_2;
        _ylblh = ylblh + padding_2;
      }

    private:
      void draw_axes(int xx, int yy, int ww, int hh) {
        fl_line_style(FL_SOLID);
        fl_color(FL_BLACK);
        fl_line(xx,
                yy+hh-_xlblh,
                xx+ww,
                yy+hh-_xlblh);
        fl_line(xx+_ylblw,
                yy+hh,
                xx+_ylblw,
                yy);
      }

      void draw_labels(int xx, int yy, int ww, int hh) {
        if(_ylblh == 0 || _xlblw == 0)
          return;

        fl_font(font, font_size);
        fl_color(FL_BLACK);
        fl_line_style(FL_SOLID);
        draw_y_labels(xx, yy, ww, hh);
        draw_x_labels(xx, yy, ww, hh);
      }

      void draw_y_labels(int xx, int yy, int ww, int hh) {
        for(int y = yy + hh - _xlblh - _ylblh / 2; y > yy; y-= _ylblh) {
          double ny = _closest_y_for_label(((yy + hh - _xlblh - y) * (_maxy - _miny)) / (hh - _xlblh) + _miny);
          int computed_y = hh + yy - (ny - _miny) * (hh - _xlblh) /( _maxy - _miny) - _xlblh ;
          if(computed_y - y < _ylblh) y = computed_y;

          fl_draw(_make_y_label_f(ny).c_str(),
                  xx + padding,
                  computed_y - (_ylblh / 2) + padding,
                  _ylblw - 2*padding,
                  _ylblh,
                  FL_ALIGN_RIGHT);
          if(_horizontal_grid) {
            fl_color(FL_LIGHT1);
            fl_line(xx + _ylblw, computed_y, xx + ww, computed_y);
            fl_color(FL_BLACK);
          }
        }
      }

      void draw_x_labels(int xx, int yy, int ww, int hh) {
        for(int x = xx + _ylblw + _xlblw/2; x < xx + ww; x += _xlblw) {
          double nx = _closest_x_for_label((x - xx - _ylblw) * (_maxx - _minx) / (ww - _ylblw) + _minx);
          int computed_x = (nx - _minx) * (ww - _ylblw) / (_maxx - _minx) + xx + _ylblw;
          if(x - computed_x < _xlblw) x = computed_x;

          fl_draw(_make_x_label_f(nx).c_str(),
                  computed_x - (_xlblw/2) + padding,
                  yy + hh - _xlblh + padding,
                  _xlblw - 2 * padding,
                  _xlblh - 2 * padding,
                  FL_ALIGN_CENTER);
          if(_vertical_grid) {
            fl_color(FL_LIGHT1);
            fl_line(computed_x, yy + hh - _xlblh, computed_x, yy);
            fl_color(FL_BLACK);
          }
        }
      }

      void draw_lines(int xx, int yy, int ww, int hh) {
        double dx =  (ww - _ylblw) / (_maxx - _minx);
        double dy =  (hh - _xlblh) / (_maxy - _miny);
        fl_line_style(FL_SOLID);
        fl_color(FL_DARK_BLUE);
        for(int i = 0; i < (int)_entries.size() - 1; i++) {
          int x1, y1, x2, y2;
          x1 = xx + _ylblw + dx * (_entries[i].x - _minx);
          y1 = yy + hh - _xlblh - dy * (_entries[i].y - _miny);
          x2 = xx + _ylblw + dx * (_entries[i+1].x - _minx);
          y2 = yy + hh - _xlblh - dy * (_entries[i+1].y - _miny);
          fl_line(x1, y1, x2, y2);
        }
      }

      void draw_points(int xx, int yy, int ww, int hh) {
        BOOST_LOG_TRIVIAL(trace) << "Chart::draw_points() called\n";
        if(_maxx != _minx && _maxy != _miny) {
          double dx = (ww - _ylblw) / (_maxx - _minx);
          double dy = (hh - _xlblh) / (_maxy - _miny);
          fl_color(FL_BLACK);
          BOOST_LOG_TRIVIAL(trace) << "Chart::draw_points() number of entries: " << _entries.size() << std::endl;
          for(int i = 0; i < _entries.size(); i++) {
            int x1, y1;
            x1 = xx + _ylblw + dx * (_entries[i].x - _minx);
            y1 = yy + hh - _xlblh - dy * (_entries[i].y - _miny);
            fl_pie(x1-3, y1-3, 6, 6, 0, 360);
          }

          if(_point_under_mouse != NULL) {
            BOOST_LOG_TRIVIAL(trace) << "Chart::draw_points() _point_under_mouse is not null\n";

            int x1, y1;
            x1 = xx + _ylblw + dx * (_point_under_mouse->x - _minx);
            y1 = yy + hh - _xlblh - dy * (_point_under_mouse->y - _miny);
            fl_pie(x1-6, y1-6, 12, 12, 0, 360);
          } else {
            BOOST_LOG_TRIVIAL(trace) << "Chart::draw_points() _point_udner_mouse is null\n";
          }
        } else {
          BOOST_LOG_TRIVIAL(trace) << "Chart::draw_points() _maxx equals _minx or _maxy equals _miny\n";
        }
        BOOST_LOG_TRIVIAL(trace) << "Chart::draw_points() end\n";
      }


      Chart_Entry<Entry_Data>* chart_point(int X, int Y) {
        BOOST_LOG_TRIVIAL(trace) << "Chart::chart_point() called\n";
        if(_maxx != _minx && _maxy != _miny) {
          Fl_Boxtype b = box();
          int xx = x()+Fl::box_dx(b);
          int yy = y()+Fl::box_dy(b);
          int ww = w()-Fl::box_dw(b);
          int hh = h()-Fl::box_dh(b);

          double dx = (ww - _ylblw) / (_maxx - _minx);
          double dy = (hh - _xlblh) / (_maxy - _miny);
          for(auto &e : _entries) {
            int x1, y1;
            x1 = xx + _ylblw + dx * (e.x - _minx);
            y1 = yy + hh - _xlblh - dy * (e.y - _miny);
            if(abs(X-x1) < 6 && abs(Y-y1) < 6) {
              BOOST_LOG_TRIVIAL(trace) << "Chart::chart_point() found entry; returning entry\n";
              return &e;
            }
          }
        } else {
          BOOST_LOG_TRIVIAL(trace) << "Chart::chart_point() _maxx equals _minx or _maxy equals _miny\n";
        }
        BOOST_LOG_TRIVIAL(trace) << "Chart::chart_point() no entry found; returning NULL\n";
        return NULL;
      }

    public:
      int handle(int event) override {
        switch(event) {
        case FL_ENTER:
          BOOST_LOG_TRIVIAL(trace) << "chart: FL_ENTER";
          return 1;
        case FL_LEAVE:
          BOOST_LOG_TRIVIAL(trace) << "chart: FL_LEAVE";
          {
            bool was_point_under_mouse = _point_under_mouse != NULL;
            _point_under_mouse = NULL;
            if(was_point_under_mouse) redraw();
          }
          tooltip(NULL);
          return 1;
        case FL_MOVE:
          BOOST_LOG_TRIVIAL(trace) << "chart: FL_MOVE";
          {
            auto point = chart_point(Fl::event_x(), Fl::event_y());
            if(point != _point_under_mouse) {
              if((_point_under_mouse = point)) {
                BOOST_LOG_TRIVIAL(trace) << "point under mouse";
                copy_tooltip(_point_under_mouse->point_label.c_str());

                Fl_Boxtype b = box();
                int xx = x()+Fl::box_dx(b);
                int yy = y()+Fl::box_dy(b);
                int ww = w()-Fl::box_dw(b);
                int hh = h()-Fl::box_dh(b);
                int x1, y1;

                double dx = (ww - _ylblw) / (_maxx - _minx);
                double dy = (hh - _xlblh) / (_maxy - _miny);

                x1 = xx + _ylblw + dx * (_point_under_mouse->x - _minx);
                y1 = yy + hh - _xlblh - dy * (_point_under_mouse->y - _miny);

                Fl_Tooltip::enter_area((Fl_Widget *)this, x1-3, y1-3, 6, 6, tooltip());
                fl_pie(x1-6, y1-6, 12, 12, 0, 360);
              } else {
                tooltip(NULL);
                BOOST_LOG_TRIVIAL(trace) << "not under mouse";
              }
              redraw();
            }
          }
          return 1;
        case FL_PUSH:
          BOOST_LOG_TRIVIAL(trace) << "chart: FL_PUSH" << "; " << when() << "; " << FL_WHEN_RELEASE;
          return (when()&FL_WHEN_RELEASE && chart_point(Fl::event_x(), Fl::event_y()))?1:0;
        case FL_RELEASE:
          BOOST_LOG_TRIVIAL(trace) << "chart: FL_RELEASE";
          if(when()&FL_WHEN_RELEASE) {
            auto point = chart_point(Fl::event_x(), Fl::event_y());
            if(point) {
              _release_point = point;
              BOOST_LOG_TRIVIAL(trace) << "Calling callback on FL_RELEASE";
              do_callback();
            }
          }
        default:
          return 0;
        }
      }
    };
  }
}

#endif
