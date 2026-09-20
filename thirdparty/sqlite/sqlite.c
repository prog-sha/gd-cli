/**************************************************************************/
/*  sqlite.c                                                              */
/**************************************************************************/
/*                          gd-cli / GDScript CLI                         */
/**************************************************************************/

// SQLite amalgamationを標準の容量設定とgdの権限設定で1回だけcompileする。

#define SQLITE_THREADSAFE 1
#define SQLITE_DQS 0
#define SQLITE_OMIT_DEPRECATED 1
#define SQLITE_OMIT_LOAD_EXTENSION 1
#define SQLITE_OMIT_SHARED_CACHE 1

#include "sqlite3.c"
