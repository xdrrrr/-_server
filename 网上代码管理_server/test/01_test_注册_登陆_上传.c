#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>     // 新增：使用标准整数类型
#include "cjson/cJSON.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6666
#define BUFFER_SIZE 4096

// 发送JSON数据（与服务器格式完全匹配）
int send_json(int sockfd, cJSON *root) {
    if (!root) {
        fprintf(stderr, "错误：空JSON对象\n");
        return -1;
    }

    // 生成JSON字符串（使用PrintUnformatted以减少数据量）
    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        fprintf(stderr, "错误：生成JSON字符串失败\n");
        return -1;
    }

    // 计算JSON长度并转换为网络字节序
    uint32_t json_length = strlen(json_str);
    uint32_t json_length_net = htonl(json_length);
    printf("准备发送JSON长度：%u 字节\n", json_length);

    // 发送长度信息
    ssize_t bytes_sent = send(sockfd, &json_length_net, sizeof(json_length_net), 0);
    if (bytes_sent != sizeof(json_length_net)) {
        perror("发送JSON长度失败");
        free(json_str);
        return -1;
    }

    // 发送JSON数据（处理分段发送）
    size_t total_sent = 0;
    while (total_sent < json_length) {
        bytes_sent = send(sockfd, json_str + total_sent, json_length - total_sent, 0);
        if (bytes_sent <= 0) {
            perror("发送JSON数据失败");
            free(json_str);
            return -1;
        }
        total_sent += bytes_sent;
    }

    printf("JSON发送完成：%s\n", json_str);
    free(json_str);  // 释放JSON字符串
    return 0;
}

// 接收JSON数据（与服务器格式完全匹配）
char *recv_json(int sockfd) {
    // 接收4字节的JSON长度（网络字节序）
    uint32_t json_length_net;
    ssize_t bytes_received = recv(sockfd, &json_length_net, sizeof(json_length_net), 0);
    
    if (bytes_received != sizeof(json_length_net)) {
        if (bytes_received == 0) {
            fprintf(stderr, "接收失败：服务器关闭连接\n");
        } else {
            perror("接收JSON长度失败");
        }
        return NULL;
    }
    
    // 转换为本地字节序
    uint32_t json_length = ntohl(json_length_net);
    printf("接收JSON长度：%u 字节\n", json_length);
    
    // 动态分配内存（+1 用于字符串结束符）
    char *json_data = (char *)malloc(json_length + 1);
    if (!json_data) {
        perror("内存分配失败");
        return NULL;
    }
    
    // 初始化内存并接收完整JSON数据
    memset(json_data, 0, json_length + 1);
    size_t total_received = 0;
    
    while (total_received < json_length) {
        bytes_received = recv(sockfd, json_data + total_received, 
                             json_length - total_received, 0);
        
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                fprintf(stderr, "接收失败：服务器关闭连接（数据不完整）\n");
            } else {
                perror("接收JSON数据失败");
            }
            free(json_data);
            return NULL;
        }
        
        total_received += bytes_received;
    }
    
    // 确保字符串以'\0'结尾
    json_data[json_length] = '\0';
    printf("JSON接收完成：%s\n", json_data);
    return json_data;
}

// 注册功能测试
void test_register(int sockfd, const char *name, const char *passwd) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "req", "register");
    cJSON_AddStringToObject(root, "name", name);
    cJSON_AddStringToObject(root, "passwd", passwd);

    printf("=== 测试注册 ===\n");
    if (send_json(sockfd, root) != 0) {
        printf("注册请求发送失败\n");
        cJSON_Delete(root);
        return;
    }

    char *response = recv_json(sockfd);
    if (!response) {
        printf("注册响应接收失败\n");
        cJSON_Delete(root);
        return;
    }

    printf("服务器响应: %s\n", response);
    cJSON *res_root = cJSON_Parse(response);
    if (res_root) {
        cJSON *stat = cJSON_GetObjectItemCaseSensitive(res_root, "stat");
        if (cJSON_IsString(stat) && strcmp(stat->valuestring, "OK") == 0) {
            printf("注册成功\n");
        } else {
            printf("注册失败\n");
        }
        cJSON_Delete(res_root);
    }

    free(response);
    cJSON_Delete(root);
}

// 登录功能测试
void test_login(int sockfd, const char *name, const char *passwd) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "req", "login");
    cJSON_AddStringToObject(root, "name", name);
    cJSON_AddStringToObject(root, "passwd", passwd);

    printf("\n=== 测试登录 ===\n");
    if (send_json(sockfd, root) != 0) {
        printf("登录请求发送失败\n");
        cJSON_Delete(root);
        return;
    }

    char *response = recv_json(sockfd);
    if (!response) {
        printf("登录响应接收失败\n");
        cJSON_Delete(root);
        return;
    }

    printf("服务器响应: %s\n", response);
    cJSON *res_root = cJSON_Parse(response);
    if (res_root) {
        cJSON *stat = cJSON_GetObjectItemCaseSensitive(res_root, "stat");
        if (cJSON_IsString(stat) && strcmp(stat->valuestring, "OK") == 0) {
            printf("登录成功\n");
        } else {
            printf("登录失败\n");
        }
        cJSON_Delete(res_root);
    }

    free(response);
    cJSON_Delete(root);
}

// 文件上传功能测试（移除 version 相关逻辑）
void test_upload(int sockfd, const char *filename, const char *content) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "req", "up");
    cJSON_AddStringToObject(root, "file", filename);
    cJSON_AddStringToObject(root, "content", content);

    printf("\n=== 测试文件上传 ===\n");
    if (send_json(sockfd, root) != 0) {
        printf("上传请求发送失败\n");
        cJSON_Delete(root);
        return;
    }

    char *response = recv_json(sockfd);
    if (!response) {
        printf("上传响应接收失败\n");
        cJSON_Delete(root);
        return;
    }

    printf("服务器响应: %s\n", response);
    cJSON *res_root = cJSON_Parse(response);
    if (res_root) {
        cJSON *stat = cJSON_GetObjectItemCaseSensitive(res_root, "stat");
        if (cJSON_IsString(stat) && strcmp(stat->valuestring, "ok") == 0) {
            printf("文件上传成功\n");
        } else {
            printf("文件上传失败\n");
        }
        cJSON_Delete(res_root);
    }

    free(response);
    cJSON_Delete(root);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // 创建socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket创建失败");
        return -1;
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("IP地址转换失败");
        close(sockfd);
        return -1;
    }

    // 连接服务器
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("连接服务器失败");
        close(sockfd);
        return -1;
    }
    printf("已连接到服务器 %s:%d\n", SERVER_IP, SERVER_PORT);

    // 测试流程：注册->登录->上传文件
    test_register(sockfd, "test_user", "test_pass123");
    test_login(sockfd, "test_user", "test_pass123");
    // 移除 version 参数
    test_upload(sockfd, "test_file.txt", "这是测试文件的内容，用于客户端上传测试"); 
    test_upload(sockfd, "test_file.txt", "这是测试文件的内容，用于客户端上传测试"); 

    // 关闭连接
    close(sockfd);
    return 0;
}
