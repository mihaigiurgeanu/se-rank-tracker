#import <Foundation/Foundation.h>
#import <lmdb.h>
#import <stdlib.h>

NSURL *app_data_folder() {
  NSFileManager *fileManager = [NSFileManager defaultManager];
  NSURL *homeDir =
    [fileManager homeDirectoryForCurrentUser];
  NSURL *baseFolderURL =
    [fileManager URLForDirectory:NSApplicationSupportDirectory
                        inDomain:NSUserDomainMask
               appropriateForURL:homeDir
                          create:YES
                           error:nil];

  NSURL *appFolderURL = [NSURL URLWithString:@"BrandThatName/GoogleRankTracker" relativeToURL:baseFolderURL];
  [fileManager createDirectoryAtURL:appFolderURL
        withIntermediateDirectories:YES
                         attributes:nil
                              error:nil];
  return appFolderURL;
}

int open_app_db_environment(MDB_env *env) {
  NSURL * appFolderURL = app_data_folder();
  return mdb_env_open(env,
                      [appFolderURL fileSystemRepresentation],
                      0,
                      0640);
}

char *app_support_folder() {
  NSURL * appFolderURL = app_data_folder();
  const char * appFolder = [appFolderURL fileSystemRepresentation];
  size_t len = strlen(appFolder);
  char *buf = (char *)malloc(len + 1);
  strcpy(buf, appFolder);
  buf[len] = '\0';
  return buf;
}
