/*===============================================
*   文件名称：myjson.c
*   创 建 者：熊袋然    
*   创建日期：2025年07月26日
*   描    述：
================================================*/
#include "myjson.h"
#include <dirent.h>
#include <sys/stat.h>
#include "cjson/cJSON.h"

#define MAX_PATH_LENGTH 256
/**
 * 生成状态JSON字符串（固定格式：{"stat":"ok"} 或 {"stat":"error"}）
 * @param status 状态字符串（如 "ok", "error", "pending" 等）
 * @return 生成的JSON字符串，使用后需调用 free() 释放内存
 */
char* generate_status_json(const char* status) {
    if (!status) return NULL;
    
    // 计算所需缓冲区大小：
    // 固定部分：2个大括号 + 4个引号 + 1个冒号 = 7字节
    // 动态部分："stat" 长度(4) + 状态字符串长度 + 1（\0）
    size_t len = 7 + 4 + strlen(status) + 1;
    char* json = (char*)malloc(len);
    if (!json) return NULL;
    
    // 格式化JSON字符串
    snprintf(json, len, "{\"stat\":\"%s\"}", status);
    return json;
}

/**
 * 生成指定用户所有文件及完整版本历史的 JSON 数组对象
 * @param username 用户名（对应 user_files/用户名 目录）
 * @return 成功返回 cJSON 数组对象（需调用者用 cJSON_Delete 释放），失败返回 NULL
 */
cJSON *get_user_files_json(const char *username) {
    if (username == NULL) {
        fprintf(stderr, "错误：用户名为空\n");
        return NULL;
    }

    // 构建用户目录路径
    char user_dir[MAX_PATH_LENGTH];
    int ret = snprintf(user_dir, MAX_PATH_LENGTH, "user_files/%s", username);
    if (ret < 0 || ret >= MAX_PATH_LENGTH) {
        fprintf(stderr, "用户目录路径过长\n");
        return NULL;
    }

    // 打开用户目录
    DIR *user_dir_ptr = opendir(user_dir);
    if (user_dir_ptr == NULL) {
        perror("无法打开用户目录");
        return NULL;
    }

    // 创建根 JSON 数组（直接作为返回值）
    cJSON *root_array = cJSON_CreateArray();
    if (root_array == NULL) {
        fprintf(stderr, "创建 JSON 数组失败\n");
        closedir(user_dir_ptr);
        return NULL;
    }

    // 遍历用户目录下的所有版本目录
    struct dirent *entry;
    while ((entry = readdir(user_dir_ptr)) != NULL) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 检查是否为版本目录
        int version;
        if (strncmp(entry->d_name, "version_", 8) != 0 ||
            sscanf(entry->d_name + 8, "%d", &version) != 1) {
            continue;  // 不是版本目录，跳过
        }

        // 构建版本目录路径
        char version_dir[MAX_PATH_LENGTH];
        ret = snprintf(version_dir, MAX_PATH_LENGTH, "%s/%s", user_dir, entry->d_name);
        if (ret < 0 || ret >= MAX_PATH_LENGTH) {
            fprintf(stderr, "版本目录路径过长\n");
            continue;
        }

        // 打开版本目录
        DIR *version_dir_ptr = opendir(version_dir);
        if (version_dir_ptr == NULL) {
            perror("无法打开版本目录");
            continue;
        }

        // 遍历版本目录下的所有文件
        struct dirent *file_entry;
        while ((file_entry = readdir(version_dir_ptr)) != NULL) {
            // 跳过 . 和 ..
            if (strcmp(file_entry->d_name, ".") == 0 || strcmp(file_entry->d_name, "..") == 0) {
                continue;
            }

            // 检查该文件是否已在 JSON 数组中
            cJSON *existing_file = NULL;
            for (int i = 0; i < cJSON_GetArraySize(root_array); i++) {
                cJSON *item = cJSON_GetArrayItem(root_array, i);
                cJSON *filename_json = cJSON_GetObjectItemCaseSensitive(item, "filename");
                if (filename_json && cJSON_IsString(filename_json) &&
                    strcmp(filename_json->valuestring, file_entry->d_name) == 0) {
                    existing_file = item;
                    break;
                }
            }

            // 处理文件记录
            if (existing_file) {
                // 若文件已存在，追加版本号到 versions 数组
                cJSON *versions_array = cJSON_GetObjectItemCaseSensitive(existing_file, "versions");
                if (!versions_array || !cJSON_IsArray(versions_array)) {
                    // 创建 versions 数组（如果不存在或不是数组）
                    versions_array = cJSON_CreateArray();
                    cJSON_AddItemToObject(existing_file, "versions", versions_array);
                }
                cJSON_AddItemToArray(versions_array, cJSON_CreateNumber(version));
            } else {
                // 若文件不存在，创建新记录
                cJSON *file_item = cJSON_CreateObject();
                cJSON_AddStringToObject(file_item, "filename", file_entry->d_name);

                // 创建 versions 数组并添加当前版本
                cJSON *versions_array = cJSON_CreateArray();
                cJSON_AddItemToArray(versions_array, cJSON_CreateNumber(version));
                cJSON_AddItemToObject(file_item, "versions", versions_array);

                cJSON_AddItemToArray(root_array, file_item);
            }
        }

        closedir(version_dir_ptr);
    }

    closedir(user_dir_ptr);

    return root_array;  // 返回 JSON 数组对象（由调用者释放）
}
