// generated by Fast Light User Interface Designer (fluid) version 1.0305

#ifndef RankTrackerUI_hh
#define RankTrackerUI_hh
#include <FL/Fl.H>
void show_about_dlg(Fl_Widget *not_used, void *dlg_wnd);
void refresh_summary_table_cb(void *summary_table);
void load_chart_data_cb(void *app);
void chart_click_cb(Fl_Widget *w, void *data);
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <algorithm>
#include <FL/fl_ask.H>
#include "ranking.hh"
#include <thread>
#include <FL/Fl_Box.H>
#include <FL/Fl_Tile.H>
#include "widgets.hh"
#include <FL/Fl_Wizard.H>
#include "ranks_chart.hh"
#include <sstream> /* to format display strings */
#include "domain_summary_table.hh"
extern void summary_table_click_cb(ranktracker::ui::DomainSummaryTable*, void*);
#include "rank_url_table.hh"
#include <FL/Fl_Pack.H>
#include <FL/Fl_Output.H>
extern void chart_click_cb(ranktracker::ui::RanksChart<ranktracker::data::Ranking>*, void*);
#include <FL/Fl_Input.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Menu_Button.H>
#include <memory>
#include <cstdlib>
#include <boost/algorithm/string/trim.hpp>
#include <FL/Fl_Progress.H>

/**
 Application's UI logic
*/
class RankTrackerUI : ranktracker::controller::AbstractController {
  /**
   current_selection - the type of currently selected UI component
  */
  enum selection_type {CRT_NONE, CRT_CATEGORY, CRT_ALL, CRT_DOMAIN, CRT_KEYWORD } current_selection; 
  /**
   user_action - The action for the settings windows.
   The settings windows are for add or update,
   and this sets the type of button pressed by
   the user: either add or update.
  */
  enum {ACTION_NONE, ACTION_ADD, ACTION_EDIT} user_action; 
public:
  RankTrackerUI();
private:
  Fl_Double_Window *app;
  inline void cb__i(Fl_Button*, void*);
  static void cb_(Fl_Button*, void*);
  inline void cb_1_i(Fl_Button*, void*);
  static void cb_1(Fl_Button*, void*);
  Fl_Button *button_refresh;
  inline void cb_button_refresh_i(Fl_Button*, void*);
  static void cb_button_refresh(Fl_Button*, void*);
  Fl_Button *button_settings;
  inline void cb_button_settings_i(Fl_Button*, void*);
  static void cb_button_settings(Fl_Button*, void*);
public:
  Fl_Tile *main_area;
  ranktracker::ui::Categories *categories;
private:
  inline void cb_categories_i(ranktracker::ui::Categories*, void*);
  static void cb_categories(ranktracker::ui::Categories*, void*);
public:
  ranktracker::ui::Domains *tree;
private:
  inline void cb_tree_i(ranktracker::ui::Domains*, void*);
  static void cb_tree(ranktracker::ui::Domains*, void*);
public:
  Fl_Wizard *details_view;
  Fl_Group *no_view;
  Fl_Tile *summary_table_view;
  ranktracker::ui::DomainSummaryTable *summary_table;
  ranktracker::ui::RankURLTable *summary_details_table;
private:
  inline void cb_summary_details_table_i(ranktracker::ui::RankURLTable*, void*);
  static void cb_summary_details_table(ranktracker::ui::RankURLTable*, void*);
public:
  Fl_Pack *chart_view;
  Fl_Output *out_crtkwd;
  ranktracker::ui::RanksChart<ranktracker::data::Ranking> *chart;
  ranktracker::ui::RankURLTable *chart_details_table;
private:
  inline void cb_chart_details_table_i(ranktracker::ui::RankURLTable*, void*);
  static void cb_chart_details_table(ranktracker::ui::RankURLTable*, void*);
  Fl_Double_Window *about_dlg;
  Fl_Double_Window *category_settings_dlg;
  Fl_Input *category_name;
  inline void cb_Ok_i(Fl_Return_Button*, void*);
  static void cb_Ok(Fl_Return_Button*, void*);
  inline void cb_Cancel_i(Fl_Button*, void*);
  static void cb_Cancel(Fl_Button*, void*);
  Fl_Double_Window *domain_settings_dlg;
  Fl_Input *domain_name;
  Fl_Browser *domain_engines;
public:
  Fl_Menu_Button *domain_add_engine_btn;
private:
  inline void cb_Remove_i(Fl_Button*, void*);
  static void cb_Remove(Fl_Button*, void*);
  inline void cb_Ok1_i(Fl_Return_Button*, void*);
  static void cb_Ok1(Fl_Return_Button*, void*);
  inline void cb_Cancel1_i(Fl_Button*, void*);
  static void cb_Cancel1(Fl_Button*, void*);
  Fl_Double_Window *keyword_settings_dlg;
  inline void cb_Ok2_i(Fl_Return_Button*, void*);
  static void cb_Ok2(Fl_Return_Button*, void*);
  inline void cb_Cancel2_i(Fl_Button*, void*);
  static void cb_Cancel2(Fl_Button*, void*);
  Fl_Double_Window *progress_wnd;
public:
  Fl_Progress *progress_bar;
  ~RankTrackerUI();
  void show(int argc, char **argv);
private:
  void onWidgetFocus(Fl_Widget *w);
  void update_domains_tree();
  void update_current_selection_for_tree();
  /**
   the domain currently selected in the ui tree
  */
  Fl_Tree_Item *selected_domain; 
  /**
   currently selected keyword in the tree
  */
  Fl_Tree_Item *selected_kwd; 
public:
  void load_chart_data();
private:
  /**
   The text buffer for keywords editor in domain settings dlg.
  */
  Fl_Text_Buffer *domain_keywords; 
  void refresh_keyword();
public:
  void refresh_domain();
  void refresh_crt_domain_list();
private:
  /**
   the text buffer for keywords settings dlg
  */
  Fl_Text_Buffer *keyword_keywords; 
public:
  void setup_chart_details_table(Fl_Table *t);
private:
  friend void summary_table_click_cb(Fl_Widget *w, void *data); 
  friend void chart_click_cb(Fl_Widget *w, void *data); 
};

/**
 A function object converting an int to a
 date-time chart label
*/
class make_datetime_label {
  boost::posix_time::ptime _start_date; 
public:
  make_datetime_label(boost::posix_time::ptime start_date) ;
  std::string operator() (int x);
};
std::string make_rank_label(int y);
double closest_day_duration(double secs);
double closest_int_rank(double y);
void summary_table_click_cb(Fl_Widget *w, void *data);
#endif
