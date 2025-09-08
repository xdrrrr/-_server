/*===============================================
*   文件名称：sqlite3_user.h
*   创 建 者：熊袋然    
*   创建日期：2025年07月26日
*   描    述：
================================================*/
#ifndef SQLITE3_USER_H
#define SQLITE3_USER_H

#include <sqlite3.h>

sqlite3 *sqlite3_init(void);
int user_register(sqlite3 *db, const char *name, const char *passwd);
int user_login(sqlite3 *db, const char *name, const char *passwd);

#endif
