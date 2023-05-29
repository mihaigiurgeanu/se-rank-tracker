/* preferences.m - manage user preferences */
#include "preferences.h"
#import <Foundation/Foundation.h>

int get_int_pref(const char *name) {
  return [[NSUserDefaults standardUserDefaults]
           integerForKey:[NSString stringWithUTF8String:name]];
}

void set_int_pref(const char *name, int value) {
  [[NSUserDefaults standardUserDefaults]
    setInteger:value forKey:[NSString stringWithUTF8String:name]];
}
