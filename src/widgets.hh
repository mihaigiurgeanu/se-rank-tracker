/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// RankTrackerWidgets.hh
// Specific widgets for the RankTracker application

#include <FL/Fl.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Tree.H>
#include "controller.hh"
#include <cassert>

namespace ranktracker {
  namespace ui {

    using ranktracker::controller::AbstractController;

    // class with controller
    class HasController {
    protected:
      AbstractController *_controller;
    public:
      HasController() : _controller(nullptr) {}

      void controller(AbstractController *pcontroller) {
        assert(pcontroller != nullptr);
        _controller = pcontroller;
      }
    };

    class Categories : public Fl_Choice, public HasController {
    public:
      Categories(int x, int y, int w, int h, const char *l = 0) :
        Fl_Choice(x, y, w, h, l)
      {}

      int handle(int event);
    };

    class Domains : public Fl_Tree, public HasController {
    public:
      Domains(int x, int y, int w, int h, const char *l = 0) :
        Fl_Tree(x, y, w, h, l)
      {}

      int handle(int event);
    };
  }
}
