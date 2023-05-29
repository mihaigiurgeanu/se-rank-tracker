#ifndef RANKTRACKER_PREFERENCES_H
#define RANKTRACKER_PREFERENCES_H

#ifdef __cplusplus
extern "C" {
#endif

  int get_int_pref(const char *name);
  void set_int_pref(const char *name, int value);

#ifdef __cplusplus
}
#endif

#endif
