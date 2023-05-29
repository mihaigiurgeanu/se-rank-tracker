// RankTrackerWidgets.cc
// extended FL widgets

#include "widgets.hh"

namespace ranktracker {
  namespace ui {

    int Categories::handle(int event) {
      int res;

      switch(event) {
      case FL_PUSH:
      case FL_FOCUS:
        assert(_controller != nullptr);
        _controller->onWidgetFocus(this);
        res = Fl_Choice::handle(event);
        break;
      default:
        res = Fl_Choice::handle(event);
        break;
      }

      return res;
    }

    int Domains::handle(int event) {
      switch(event) {
      case FL_FOCUS:
        assert(_controller != nullptr);
        _controller->onWidgetFocus(this);
        break;
      }

      return Fl_Tree::handle(event);
    }
  }
}
