/*===============================================
*   文件名称：user_file_save.c
*   创 建 者：熊袋然    
*   创建日期：2025年07月27日
*   描    述：
================================================*/
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "user_file_save.h"

// 私有结构体：链表节点
typedef struct FileVersionNode {
    char filename[MAX_FILENAME_LENGTH];
    int version;
    struct FileVersionNode *next;
} FileVersionNode;

static FileVersionNode *version_head = NULL;

// 私有函数：创建目录
static int create_directory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0700) == -1) {
            perror("无法创建目录");
            return -1;
        }
    }
    return 0;
}

// 私有函数：获取文件版本号
static int get_file_version(const char *filename) {
    FileVersionNode *current = version_head->next;
    FileVersionNode *prev = version_head;

    while (current != NULL) {
        if (strcmp(current->filename, filename) == 0) {
            current->version++;
            return current->version;
        }
        prev = current;
        current = current->next;
    }

    FileVersionNode *new_node = (FileVersionNode *)malloc(sizeof(FileVersionNode));
    if (new_node == NULL) {
        perror("内存分配失败");
        return -1;
    }
    // 对文件名复制添加snprintf判断
    int ret = snprintf(new_node->filename, MAX_FILENAME_LENGTH, "%s", filename);
    if (ret < 0 || ret >= MAX_FILENAME_LENGTH) {
        fprintf(stderr, "文件名过长或复制失败: %s\n", filename);
        free(new_node);
        return -1;
    }
    new_node->version = 1;
    new_node->next = NULL;

    prev->next = new_node;
    return new_node->version;
}

// 公共函数：初始化链表
void init_version_list(void) {
    version_head = (FileVersionNode *)malloc(sizeof(FileVersionNode));
    if (version_head == NULL) {
        perror("头节点内存分配失败");
        exit(EXIT_FAILURE);
    }
    version_head->filename[0] = '\0';
    version_head->version = 0;
    version_head->next = NULL;
}

// 公共函数：保存文件（核心修改处：为每个snprintf添加错误判断）
int save_file(const char *username, const char *filename, const char *content) {
    int version = get_file_version(filename);
    if (version == -1) {
        fprintf(stderr, "获取版本号失败\n");
        return -1;
    }

    char user_dir[MAX_PATH_LENGTH];
    char version_dir[MAX_PATH_LENGTH];
    char file_path[MAX_PATH_LENGTH];
    int ret;

    // 1. 构建用户目录路径
    ret = snprintf(user_dir, MAX_PATH_LENGTH, "user_files/%s", username);
    if (ret < 0 || ret >= MAX_PATH_LENGTH) {
        fprintf(stderr, "用户目录路径过长或格式错误: %s\n", username);
        return -1;
    }
    if (create_directory("user_files") != 0 || create_directory(user_dir) != 0) {
        return -1;
    }

    // 2. 构建版本目录路径
    ret = snprintf(version_dir, MAX_PATH_LENGTH, "%s/version_%d", user_dir, version);
    if (ret < 0 || ret >= MAX_PATH_LENGTH) {
        fprintf(stderr, "版本目录路径过长或格式错误: %s, 版本%d\n", user_dir, version);
        return -1;
    }
    if (create_directory(version_dir) != 0) {
        return -1;
    }

    // 3. 构建文件路径
    ret = snprintf(file_path, MAX_PATH_LENGTH, "%s/%s", version_dir, filename);
    if (ret < 0 || ret >= MAX_PATH_LENGTH) {
        fprintf(stderr, "文件路径过长或格式错误: %s, %s\n", version_dir, filename);
        return -1;
    }

    // 写入文件内容
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("无法打开文件");
        return -1;
    }
    fputs(content, file);
    fclose(file);

    printf("文件已成功保存至: %s（版本 %d）\n", file_path, version);
    return 0;
}

//根据用户名、文件名、版本号读取文件
char *read_file(const char *username, const char *filename, int version) {
    char user_dir[MAX_PATH_LENGTH];
    char version_dir[MAX_PATH_LENGTH];
    char file_path[MAX_PATH_LENGTH];
    int ret;

    // 1. 拼接用户目录路径
    ret = snprintf(user_dir, MAX_PATH_LENGTH, "user_files/%s", username);
    if (ret < 0 || ret >= MAX_PATH_LENGTH) {
        fprintf(stderr, "用户目录路径过长或格式错误: %s\n", username);
        return NULL;
    }

    // 2. 拼接版本目录路径
    ret = snprintf(version_dir, MAX_PATH_LENGTH, "%s/version_%d", user_dir, version);
    if (ret < 0 || ret >= MAX_PATH_LENGTH) {
        fprintf(stderr, "版本目录路径过长或格式错误: %s, 版本%d\n", user_dir, version);
        return NULL;
    }

    // 3. 拼接文件完整路径
    ret = snprintf(file_path, MAX_PATH_LENGTH, "%s/%s", version_dir, filename);
    if (ret < 0 || ret >= MAX_PATH_LENGTH) {
        fprintf(stderr, "文件路径过长或格式错误: %s, %s\n", version_dir, filename);
        return NULL;
    }

    // 4. 打开文件并获取文件大小
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("无法打开文件（可能文件不存在或权限不足）");
        return NULL;
    }

    // 获取文件大小
    if (fseek(file, 0, SEEK_END) != 0) {
        perror("无法定位文件末尾");
        fclose(file);
        return NULL;
    }
    long file_size = ftell(file);
    if (file_size == -1) {
        perror("无法获取文件大小");
        fclose(file);
        return NULL;
    }
    rewind(file); // 回到文件开头

    // 5. 动态分配精确大小的内存（+1 用于字符串结束符）
    char *content = (char *)malloc(file_size + 1);
    if (content == NULL) {
        perror("内存分配失败，无法读取文件内容");
        fclose(file);
        return NULL;
    }

    // 6. 读取完整文件内容
    size_t bytes_read = fread(content, 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        if (feof(file)) {
            fprintf(stderr, "警告：文件读取不完整（可能文件被修改）\n");
        } else {
            perror("文件读取失败");
            free(content);
            fclose(file);
            return NULL;
        }
    }

    content[file_size] = '\0'; // 确保字符串以'\0'结尾
    fclose(file);
    return content;
}

// 公共函数：释放链表
void free_version_list(void) {
    FileVersionNode *current = version_head->next;
    while (current != NULL) {
        FileVersionNode *temp = current;
        current = current->next;
        free(temp);
    }
    free(version_head);
    version_head = NULL;
}
