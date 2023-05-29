#!/bin/sh
cp src/ranktracker GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib @executable_path/libpng16.16.dylib \
		  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change /usr/local/opt/jpeg/lib/libjpeg.9.dylib @executable_path/libjpeg.9.dylib \
		  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change /usr/local/opt/lmdb/lib/liblmdb.dylib @executable_path/liblmdb.dylib \
                  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change /usr/local/lib/libboost_serialization.dylib @executable_path/libboost_serialization.dylib \
                  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change /usr/local/lib/libboost_date_time.dylib @executable_path/libboost_date_time.dylib \
                  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change @rpath/libboost_serialization.dylib @executable_path/libboost_serialization.dylib \
                  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change @rpath/libboost_date_time.dylib @executable_path/libboost_date_time.dylib \
                  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change /usr/local/opt/curl/lib/libcurl.4.dylib @executable_path/libcurl.4.dylib \
                  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change @rpath/libboost_log.dylib @executable_path/libboost_log.dylib \
                  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change @rpath/libboost_log_setup.dylib @executable_path/libboost_log_setup.dylib \
                  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change @rpath/libboost_filesystem.dylib @executable_path/libboost_filesystem.dylib \
                  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change @rpath/libboost_system.dylib @executable_path/libboost_system.dylib \
                  GoogleRankTracker.app/Contents/MacOS/ranktracker
install_name_tool -change @rpath/libboost_thread.dylib @executable_path/libboost_thread.dylib \
                  GoogleRankTracker.app/Contents/MacOS/ranktracker

install_name_tool -change @rpath/libboost_atomic.dylib @executable_path/libboost_atomic.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log.dylib
install_name_tool -change @rpath/libboost_chrono.dylib @executable_path/libboost_chrono.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log.dylib
install_name_tool -change @rpath/libboost_thread.dylib @executable_path/libboost_thread.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log.dylib
install_name_tool -change @rpath/libboost_date_time.dylib @executable_path/libboost_date_time.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log.dylib
install_name_tool -change @rpath/libboost_filesystem.dylib @executable_path/libboost_filesystem.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log.dylib
install_name_tool -change @rpath/libboost_regex.dylib @executable_path/libboost_regex.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log.dylib

install_name_tool -change @rpath/libboost_log.dylib @executable_path/libboost_log.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log_setup.dylib
install_name_tool -change @rpath/libboost_atomic.dylib @executable_path/libboost_atomic.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log_setup.dylib
install_name_tool -change @rpath/libboost_chrono.dylib @executable_path/libboost_chrono.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log_setup.dylib
install_name_tool -change @rpath/libboost_thread.dylib @executable_path/libboost_thread.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log_setup.dylib
install_name_tool -change @rpath/libboost_date_time.dylib @executable_path/libboost_date_time.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log_setup.dylib
install_name_tool -change @rpath/libboost_filesystem.dylib @executable_path/libboost_filesystem.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log_setup.dylib
install_name_tool -change @rpath/libboost_regex.dylib @executable_path/libboost_regex.dylib \
                  GoogleRankTracker.app/Contents/MacOS/libboost_log_setup.dylib
