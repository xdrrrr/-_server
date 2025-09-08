/*===============================================
*   文件名称：myjosh.h
*   创 建 者：熊袋然    
*   创建日期：2025年07月26日
*   描    述：
================================================*/
#ifndef MYJOSH_H
#define MYJOSN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

char* generate_status_json(const char* status);
cJSON *get_user_files_json(const char *username);
#endif
