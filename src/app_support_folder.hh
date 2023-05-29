#ifndef RANKTRACKER_APP_SUPPORT_FOLDER_H
#define RANKTRACKER_APP_SUPPORT_FOLDER_H

#include <lmdb.h>

extern "C"
{

  /**
   * Open the mdb environemnt on default application folder path, which, for
   * MacOS, is in the library application support folder.
   */
  int open_app_db_environment(MDB_env *);

  /**
   * returns the app support folder as a heap allocated string; you must
   * free the memory using std::free
   */
  char *app_support_folder();
}

#endif
