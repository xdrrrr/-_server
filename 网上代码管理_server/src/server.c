#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "server.h"

int server_init(char *IP,unsigned short PORT,int backlog)
{
    int server_fd;
    struct sockaddr_in server_addr = {0};
    if (-1 == (server_fd = socket(AF_INET,SOCK_STREAM,0)))
    {
        perror("socket fail");
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(IP);
    server_addr.sin_port = htons(PORT);
    if (-1 == bind(server_fd,(struct sockaddr *)&server_addr,sizeof(server_addr)))
    {
        perror("bind fail");
        close(server_fd);
        return -1;
    }

    if (-1 == listen(server_fd,backlog))
    {
        perror("listen fail");
        close(server_fd);
        return -1;
    }
    return server_fd;
}

int server_accept(int server_fd)
{
    int client_fd;
    struct sockaddr_in client_addr  = {0};
    socklen_t client_len = sizeof(client_addr);
    if(-1 == (client_fd = accept(server_fd,  (struct sockaddr *) &client_addr,&client_len)))
    {
        perror("accept fail");
        printf("%d\n",client_fd);
        return -1;
    }
    
    printf("accept client_fd=%d success\n",client_fd - 3);
    return client_fd;
}

char *receive_json(int client_socket) {
    // 1. 接收4字节的JSON长度（网络字节序）
    uint32_t json_length_net;
    ssize_t bytes_received = recv(client_socket, &json_length_net, sizeof(json_length_net), 0);
    
    if (bytes_received != sizeof(json_length_net)) {
        if (bytes_received == 0) {
            fprintf(stderr, "接收失败：客户端关闭连接\n");
        } else {
            perror("接收JSON长度失败");
        }
        return NULL;
    }
    
    // 2. 转换为本地字节序
    uint32_t json_length = ntohl(json_length_net);
    printf("接收JSON长度：%u 字节\n", json_length);
    
    // 3. 动态分配内存（+1 用于字符串结束符）
    char* json_data = (char*)malloc(json_length + 1);
    if (json_data == NULL) {
        perror("内存分配失败");
        return NULL;
    }
    
    // 4. 初始化内存
    memset(json_data, 0, json_length + 1);
    
    // 5. 循环接收JSON数据（处理粘包/分包情况）
    size_t total_bytes_received = 0;
    while (total_bytes_received < json_length) {
        bytes_received = recv(client_socket, 
                             json_data + total_bytes_received, 
                             json_length - total_bytes_received, 
                             0);
        
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                fprintf(stderr, "接收失败：客户端关闭连接（数据不完整）\n");
            } else {
                perror("接收JSON数据失败");
            }
            free(json_data);
            return NULL;
        }
        
        total_bytes_received += bytes_received;
    }
    
    // 6. 添加字符串结束符（确保安全性）
    json_data[json_length] = '\0';
    
    printf("JSON接收完成：%s\n", json_data);
    return json_data;
}

int send_json(int client_socket, const char* json_str) {
    // 检查输入参数
    if (client_socket < 0 || json_str == NULL) {
        fprintf(stderr, "错误：无效的套接字或空JSON字符串\n");
        return -1;
    }

    // 1. 计算JSON字符串长度
    uint32_t json_length = strlen(json_str);
    printf("准备发送JSON长度：%u 字节\n", json_length);

    // 2. 转换为网络字节序（大端序）
    uint32_t json_length_net = htonl(json_length);

    // 3. 发送4字节的长度信息
    ssize_t bytes_sent = send(client_socket, &json_length_net, sizeof(json_length_net), 0);
    if (bytes_sent != sizeof(json_length_net)) {
        perror("发送JSON长度失败");
        return -1;
    }

    // 4. 发送JSON数据（处理分段发送情况）
    size_t total_bytes_sent = 0;
    while (total_bytes_sent < json_length) {
        bytes_sent = send(client_socket,
                         json_str + total_bytes_sent,
                         json_length - total_bytes_sent,
                         0);

        if (bytes_sent <= 0) {
            perror("发送JSON数据失败");
            return -1;
        }

        total_bytes_sent += bytes_sent;
    }

    printf("JSON发送完成：%s\n", json_str);
    return 0;
}
