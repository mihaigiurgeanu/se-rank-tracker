/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// ranktracker.cc
// main application file of RankTracker application

#include "RankTrackerUI.hh"
#include "data_provider.hh"
#include "engines.hh"
#include <iostream>
#include <curl/curl.h>
#include <locale>
#include "logging.hh"
#include "app_support_folder.hh"
#include <cstdlib>

static void init_log();

/**
 main entry point
*/
int main(int argc, char **argv) {
  std::locale::global(std::locale(""));
  init_log();
  BOOST_LOG_TRIVIAL(info) << "Application start";

  auto curl_result = curl_global_init(CURL_GLOBAL_ALL);
  if(curl_result != 0) {
    BOOST_LOG_TRIVIAL(error) << "curl library failed to initialize with the code: " << curl_result << std::endl;
    BOOST_LOG_TRIVIAL(error) << "you won't be able to check the online (current) ranking\n";
  }
  ranktracker::engine::init_search_engines();

  RankTrackerUI *ui = new RankTrackerUI();

  ui->show(argc, argv);
  Fl::run();
  delete ui;

  BOOST_LOG_TRIVIAL(info) << "Application end";
}

// log initialization
static void init_log() {
  const char *log_folder = app_support_folder();
  std::string log_file = std::string(log_folder) + "/google_rank_tracker-%N.log";
  std::free((void *)log_folder);

  logging::add_file_log
    (keywords::file_name = std::move(log_file),
     keywords::rotation_size = 10 * 1024 * 1024,
     keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
     keywords::format = "[%TimeStamp%][%ThreadID%][%Severity%]: %Message%"
     );
  logging::add_console_log();

  logging::core::get()->set_filter
    (logging::trivial::severity >= logging::trivial::trace);

  logging::add_common_attributes();
}
