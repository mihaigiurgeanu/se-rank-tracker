/* -*- mode: C++; flycheck-clang-language-standard: "c++11" -*- */
// progress.hh
// progress updater support

#ifndef RANKTRACKER_PROGRESS_H
#define RANKTRACKER_PROGRESS_H

#include <functional>

namespace ranktracker {
  namespace progress {
    typedef std::function<void(int)> progress_updater;

    class tracking_progress_updater {
      int& _crt_progress;
      progress_updater _target_updater;
    public:
      tracking_progress_updater(int& progress_var, progress_updater target_updater)
        : _crt_progress(progress_var), _target_updater(target_updater)
      {}

      void operator() (int progress) {
        BOOST_LOG_TRIVIAL(trace) << "tracking_progress_updater::operator() called\n";
        BOOST_LOG_TRIVIAL(trace) << "tracking_progress_updater::operator() progress = "
                                 << progress << std::endl;
        _crt_progress = progress;
        BOOST_LOG_TRIVIAL(trace) << "tracking_progress_updater::operator() calling target updater\n";
        _target_updater(progress);
        BOOST_LOG_TRIVIAL(trace) << "tracking_progress_updater::operator() exit\n";
      }

      int crt_progress() {
        return _crt_progress;
      }
    };

    class offset_progress_updater {
      const int _offset;
      progress_updater _target_updater;
    public:
      offset_progress_updater(int offset, progress_updater target_updater)
        : _offset(offset), _target_updater(target_updater)
      {}
      void operator() (int progress) {
        BOOST_LOG_TRIVIAL(trace) << "offset_progress_updater::operator() called\n";
        BOOST_LOG_TRIVIAL(trace) << "offset_progress_updater::operator() progress = "
                                 << progress << std::endl;
        BOOST_LOG_TRIVIAL(trace) << "offset_progress_updater::operator() actual progress = "
                                 << _offset + progress << std::endl;
        BOOST_LOG_TRIVIAL(trace) << "offset_progress_updater::operator() calling target_updater\n";
        _target_updater(_offset + progress);
        BOOST_LOG_TRIVIAL(trace) << "offset_progress_updater::operator() exit\n";
      }
    };
  }
}

#endif
