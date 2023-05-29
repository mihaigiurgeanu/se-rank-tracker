CXX      = $(shell fltk-config --cxx)
CC       = $(shell fltk-config --cc)
DEBUG    = -g
CXXFLAGS = $(shell fltk-config --use-images --cxxflags ) -I. -std=c++11 -mmacosx-version-min=10.10 ${shell /usr/local/opt/curl/bin/curl-config --cflags} -DBOOST_LOG_DYN_LINK
CCFLAGS  = -I. -mmacosx-version-min=10.10
LDFLAGS  = $(shell fltk-config --use-images --ldflags ) -llmdb -lboost_log -lboost_log_setup -lboost_serialization -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -mmacosx-version-min=10.10 $(shell /usr/local/opt/curl/bin/curl-config --libs) -llexbor_static
LINK     = $(CXX)
TARGET = ranktracker
OBJS = ranktracker.o RankTrackerUI.o widgets.o data_provider.o data_model.o engines.o app_support_folder.o domain_summary_table.o ranking.o preferences.o colors.o chart.o rank_url_table.o

.SUFFIXES: .o .cc
.PHONY: all clean
%.o: %.cc
	$(CXX) $(CXXFLAGS) $(DEBUG) -c $<
all: $(TARGET)
$(TARGET): $(OBJS)
	$(LINK) -o $(TARGET) $(OBJS) $(LDFLAGS)
app_support_folder.o: app_support_folder.m
	$(CC) $(CCFLAGS) $(DEBUG) -c app_support_folder.m
preferences.o: preferences.m preferences.h
	$(CC) $(CCFLAGS) $(DEBUG) -c preferences.m
ranktracker.o: ranktracker.cc RankTrackerUI.hh controller.hh widgets.hh engines.hh data_model.hh data_provider.hh entity.hh logging.hh
RankTrackerUI.o: RankTrackerUI.cc RankTrackerUI.hh controller.hh widgets.hh engines.hh data_model.hh data_provider.hh entity.hh domain_summary_table.hh ranking.hh logging.hh chart.hh ranks_chart.hh rank_url_table.hh
widgets.o: widgets.cc widgets.hh logging.hh
data_provider.o: data_provider.cc data_provider.hh data_model.hh engines.hh entity.hh logging.hh
data_model.o: data_model.cc data_model.hh engines.hh entity.hh logging.hh
engines.o: engines.cc engines.hh entity.hh logging.hh
domain_summary_table.o: domain_summary_table.cc domain_summary_table.hh data_model.hh entity.hh engines.hh logging.hh colors.hh data_provider.hh
ranking.o: ranking.cc data_provider.hh data_model.hh engines.hh entity.hh logging.hh
colors.o: colors.cc colors.hh
chart.o:: chart.cc chart.hh
rank_url_table.o: rank_url_table.cc rank_url_table.cc

RankTrackerUI.cc RankTrackerUI.hh: RankTrackerUI.fld
	fluid -o .cc -h .hh -c RankTrackerUI.fld
clean:
	rm -f $(OBJS) 2> /dev/null
	rm -f $(TARGET) 2> /dev/null
	rm -f RankTrackerUI.cc RankTrackerUI.hh 2> /dev/null
