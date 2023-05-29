/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */

// controller.hh
// Abstract class implemented by the RankTrackerUI, defining
// the controller interface for the operations called from
// the UI widgets.
#ifndef RANKTRACKER_CONTROLLER_HH
#define RANKTRACKER_CONTROLLER_HH

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>

#include "data_model.hh"
#include "data_provider.hh"
#include "ranking.hh"
#include "progress.hh"
#include <cassert>


namespace ranktracker {
  namespace controller {

    using namespace ranktracker::data;

    inline void hide_window_callback(void *w) {
      ((Fl_Widget *)w)->hide();
    }

    template<class ProgressBar>
    class progress_bar_updater : public ranktracker::progress::progress_updater {
      ProgressBar *_w;
    public:
      progress_bar_updater(ProgressBar *w) : _w(w) {}
      void operator() (int new_val) {
        BOOST_LOG_TRIVIAL(trace) << "progress_bar_updater::operator() called\n";
        BOOST_LOG_TRIVIAL(trace) << "progress_bar_updater::operator() progress = "
                                 << new_val << std::endl;
        Fl::lock();
        _w->value(new_val);
        Fl::unlock();
        Fl::awake();
        BOOST_LOG_TRIVIAL(trace) << "progress_bar_updater::operator() exit\n";
      }
    };

    struct TreeItemData {
      enum {DOMAIN_ITEM, KEYWORD_ITEM} item_type;
      union {
        ranktracker::data::Domain * domain;
        ranktracker::data::Keyword * keyword;
      };
    };

    class AbstractController {
    protected:
      ranktracker::persistence::DataProvider db;
      std::vector<Category> categories_list;
      ranktracker::ranking::RankingService ranking_service;

      // current index of the selected category
      int selected_category_idx;

      std::vector<Domain> domains_list;
      std::vector<Keyword> keywords_list;
    public:
      // UI logic for certain witdgets that get the focus.
      virtual void onWidgetFocus(Fl_Widget *w) = 0;

      AbstractController() : ranking_service(db)
      {
        ranktracker::persistence::create_transaction transaction(&db, MDB_RDONLY);
        categories_list = db.categories();
        selected_category_idx = 0;
        domains_list = db.domains(categories_list[selected_category_idx]);
        transaction.commit();
      }

      virtual ~AbstractController() {
        BOOST_LOG_TRIVIAL(trace) << "~AbstractController() enter";
        BOOST_LOG_TRIVIAL(trace) << "~AbstractController() exit";
      }
    };

    namespace domain {
      namespace settings {
        // helper function to search for an item in the list box
        template<class ListBox>
        int find_item(ListBox *l, void *data) {
          for(int i = 1; i <= l->size(); i++) {
            if(l->data(i) == data)
              return i;
          }
          return 0;
        }

        // UI Callback called when an engine should be added to
        // the domain in the domain settings dlg
        template<class EnginesList>
        void add_engine_cb(Fl_Widget *w, void *data) {
          assert(w != NULL);
          assert(data != NULL);
          EnginesList * _engines_list = (EnginesList *)w->user_data();
          assert(_engines_list != NULL);

          // add item only if it not exist
          if(find_item<EnginesList>(_engines_list, data) <= 0) {
            _engines_list->add(((ranktracker::engine::SearchEngine *)data)->name().c_str(), (void *)data);
          }
        }



        // Add engines to the menu
        template<class Menu>
        class EnginesMenuAddr {
          Menu *_enginesMenu;
          Fl_Callback *_cb;
          EnginesMenuAddr() = delete;
        public:
          EnginesMenuAddr(Menu *menu, Fl_Callback *cb) :
            _enginesMenu(menu),
            _cb(cb)
          {
            assert(menu != NULL);
          }

          void operator() (const ranktracker::engine::engines_map::value_type& engine) {
            _enginesMenu->add(engine.second.name().c_str(), 0, _cb, (void *)engine.second.engine(), 0);
          }
        };
      }
    }
  }
}

#endif
