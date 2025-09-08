#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_PATH_LENGTH 256
#define MAX_USERNAME_LENGTH 50
#define MAX_FILENAME_LENGTH 50
#define MAX_CONTENT_LENGTH 1024

int create_directory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0700) == -1) {
            perror("无法创建目录");
            return -1;
        }
    }
    return 0;
}

int save_file(const char *username, const char *filename, const char *content, int version) {
    char user_dir[MAX_PATH_LENGTH];
    char version_dir[MAX_PATH_LENGTH];
    char file_path[MAX_PATH_LENGTH];
    int n;
    // 构建用户目录路径
    n = snprintf(user_dir, MAX_PATH_LENGTH, "user_files/%s", username);
    if (n < 0 || n >= MAX_PATH_LENGTH) 
    {
        fprintf(stderr, "Path too long: %s/version_%d\n", user_dir, version);
        return -1;   // 或其他错误处理
    }

    if (create_directory("user_files") != 0) return -1;
    if (create_directory(user_dir) != 0) return -1;

    // 构建版本目录路径
    n = snprintf(version_dir, MAX_PATH_LENGTH, "%s/version_%d", user_dir, version);
    if (n < 0 || n >= MAX_PATH_LENGTH) 
    {
        fprintf(stderr, "Path too long: %s/version_%d\n", user_dir, version);
        return -1;   // 或其他错误处理
    }
    if (create_directory(version_dir) != 0) return -1;

    // 构建文件路径
    n = snprintf(file_path, MAX_PATH_LENGTH, "%s/%s", version_dir, filename);
    if (n < 0 || n >= MAX_PATH_LENGTH) 
    {
        fprintf(stderr, "Path too long: %s/version_%d\n", user_dir, version);
        return -1;   // 或其他错误处理
    }

    // 写入文件内容
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("无法打开文件");
        return -1;
    }
    fputs(content, file);
    fclose(file);

    printf("文件已成功保存至: %s\n", file_path);
    return 0;
}

// 遍历指定用户的所有文件版本
int list_user_files(const char *username) {
    char user_dir[MAX_PATH_LENGTH];
    int n;
    n = snprintf(user_dir, MAX_PATH_LENGTH, "user_files/%s", username);
    if (n < 0 || n >= MAX_PATH_LENGTH) 
    {
        fprintf(stderr, "Path too long:");
        return -1;   // 或其他错误处理
    }

    // 检查用户目录是否存在
    struct stat st;
    if (stat(user_dir, &st) == -1) {
        printf("用户目录不存在: %s\n", user_dir);
        return -1;
    }

    if (!S_ISDIR(st.st_mode)) {
        printf("路径不是目录: %s\n", user_dir);
        return -1;
    }

    // 打开用户目录
    DIR *user_dir_ptr = opendir(user_dir);
    if (user_dir_ptr == NULL) {
        perror("无法打开用户目录");
        return -1;
    }

    struct dirent *entry;
    printf("用户 %s 的文件列表:\n", username);

    // 遍历所有版本目录
    while ((entry = readdir(user_dir_ptr)) != NULL) {
        // 跳过.和..目录
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 检查是否为版本目录 (格式: version_数字)
        int version;
        if (sscanf(entry->d_name, "version_%d", &version) == 1) {
            char version_dir[MAX_PATH_LENGTH];
            n = snprintf(version_dir, MAX_PATH_LENGTH, "%s/%s", user_dir, entry->d_name);
            if (n < 0 || n >= MAX_PATH_LENGTH) 
            {
                fprintf(stderr, "Path too long: ");
                return -1;   // 或其他错误处理
            }

            // 打开版本目录
            DIR *version_dir_ptr = opendir(version_dir);
            if (version_dir_ptr == NULL) {
                perror("无法打开版本目录");
                continue;
            }

            struct dirent *file_entry;
            // 遍历版本目录中的所有文件
            while ((file_entry = readdir(version_dir_ptr)) != NULL) {
                // 跳过.和..目录
                if (strcmp(file_entry->d_name, ".") == 0 || strcmp(file_entry->d_name, "..") == 0) {
                    continue;
                }

                // 打印文件信息
                printf("版本 %d: %s\n", version, file_entry->d_name);
            }

            closedir(version_dir_ptr);
        }
    }

    closedir(user_dir_ptr);
    return 0;
}

int main() {
    const char *username = "user1";
    const char *filename = "example.txt";
    const char *content = "这是文件内容";
    int version = 1;

    // 保存示例文件
    if (save_file(username, filename, content, version) == 0) {
        printf("保存成功\n");
    } else {
        printf("保存失败\n");
        return 1;
    }

    // 保存第二个版本
    if (save_file(username, filename, "新版本内容", version + 1) == 0) {
        printf("保存成功\n");
    } else {
        printf("保存失败\n");
        return 1;
    }

    // 遍历用户文件
    if (list_user_files(username) != 0) {
        printf("列出文件失败\n");
        return 1;
    }

    return 0;
}
