/*===============================================
*   文件名称：sqlite3_USER.c
*   创 建 者：熊袋然    
*   创建日期：2025年07月26日
*   描    述：
================================================*/
#include <stdio.h>
#include <string.h>
#include "sqlite3_user.h"

#define SQL_LEN 512        // SQL语句缓冲区大小

sqlite3 *sqlite3_init(void)
{
    sqlite3 *db;
    int ret = sqlite3_open("my.db", &db);
    if (ret != SQLITE_OK) {
        fprintf(stderr, "数据库打开失败：%s\n", sqlite3_errmsg(db));
        return NULL;
    }

    // 创建用户表（name为主键，确保用户名唯一）
    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS name_passwd ("
        "name TEXT PRIMARY KEY, "  // 用户名唯一（避免重复注册）
        "passwd TEXT NOT NULL"     // 密码非空
        ");";
    char *errmsg = NULL;
    ret = sqlite3_exec(db, create_sql, NULL, NULL, &errmsg);
    if (ret != SQLITE_OK) {
        fprintf(stderr, "表创建失败：%s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return NULL;
    }
    return db;
}

int user_register(sqlite3 *db, const char *name, const char *passwd) {
    // 构造插入SQL语句（注意字符串用单引号包裹）
    char sql[SQL_LEN];
    int ret = snprintf(sql, SQL_LEN,
        "INSERT INTO name_passwd (name, passwd) VALUES ('%s', '%s');",
        name, passwd);

    // 检查SQL语句是否被截断
    if (ret >= SQL_LEN) {
        fprintf(stderr, "注册失败：用户名或密码过长\n");
        return -1;
    }

    char *errmsg = NULL;
    // 执行插入（主键冲突时会失败，避免重复注册）
    ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if (ret != SQLITE_OK) {
        fprintf(stderr, "注册失败：%s（可能用户名已存在）\n", errmsg);
        sqlite3_free(errmsg);  // 释放错误信息
        return -1;
    }

    printf("注册成功！\n");
    return 0;
}

// 登录功能：查询数据库中是否存在匹配的用户名和密码
int user_login(sqlite3 *db, const char *name, const char *passwd) {
    // 构造查询SQL语句
    char sql[SQL_LEN];
    int ret = snprintf(sql, SQL_LEN,
        "SELECT * FROM name_passwd WHERE name = '%s' AND passwd = '%s';",
        name, passwd);

    if (ret >= SQL_LEN) {
        fprintf(stderr, "登录失败：用户名或密码过长\n");
        return -1;
    }

    char **result;  // 存储查询结果
    int row, col;   // 行数（不含表头）和列数
    char *errmsg = NULL;

    // 执行查询（sqlite3_get_table返回结果集）
    ret = sqlite3_get_table(db, sql, &result, &row, &col, &errmsg);
    if (ret != SQLITE_OK) {
        fprintf(stderr, "登录失败：%s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }

    // 判断是否存在匹配记录：row为查询到的数据行数（不含表头）
    if (row > 0) {
        printf("登录成功！\n");
        sqlite3_free_table(result);
        return 0;
    } else {
        printf("登录失败：用户名或密码不存在\n");
        sqlite3_free_table(result);
        return -1;
    }

    // 释放查询结果集（必须调用，否则内存泄漏）
    sqlite3_free_table(result);
    return 0;
}



