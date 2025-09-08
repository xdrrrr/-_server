#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "server.h"
#include "cjson/cJSON.h"
#include "myjson.h"
#include "sqlite3_user.h"
#include "user_file_save.h"

#define TEST
#ifdef  TEST   //判断宏是否定义   
    #define test(fmt,...)   printf("%s-%d-%s\n",__func__, __LINE__,fmt);         //宏函数 
#else
    #define test(fmt,...)  
#endif 

#define SIZE 1024

void *client_handle(void *client_fd_p)
{
    int client_fd = *(int *)client_fd_p;
    cJSON *root = NULL;
        
    //保存登陆成功的用户名
    char *user_name = NULL;

    //初始化文件版本管理链表头节点
    init_version_list();

    while (1)
    {
        //接收json字节数和json字符串
        char *recvbuf = NULL;
        if (NULL == (recvbuf = receive_json(client_fd)))
        {
            printf("recv fail\n");
            close(client_fd);
            return NULL;
        }

        //josn字符串变成json结构体
        root = cJSON_Parse(recvbuf);
        if (!root) {
            fprintf(stderr, "JSON parse error: %s\n", cJSON_GetErrorPtr());
            close(client_fd);
            return NULL;
        }

        // 解析JSON请求
        cJSON *req = cJSON_GetObjectItemCaseSensitive(root, "req");
        if (!cJSON_IsString(req) || !req->valuestring) {
            fprintf(stderr, "Invalid operation\n");
            goto error;
        }

        //实现不同功能
        
        //1.上传
        if (strcmp(req->valuestring, "up") == 0) 
        {
            cJSON *file = cJSON_GetObjectItemCaseSensitive(root, "file");
            cJSON *content = cJSON_GetObjectItemCaseSensitive(root, "content");
            if (!cJSON_IsString(file) || !cJSON_IsString(content) || !file->valuestring || !content->valuestring) 
            {
                fprintf(stderr, "Invalid upload format\n");
                //回复ON
                char *stat = "NO";
                char * json_str = generate_status_json(stat);
                send_json(client_fd,json_str);
                continue;
            }
            
            if (save_file(user_name, file->valuestring, content->valuestring) == 0) {
                printf("保存成功\n");
            } else {
                printf("保存失败\n");
                //回复ON
                char *stat = "NO";
                char * json_str = generate_status_json(stat);
                send_json(client_fd,json_str);
                continue;
            }
            printf("Upload completed: %s\n", file->valuestring);
            

            //回复ok
            char *stat = "OK";
            char * json_str = generate_status_json(stat);
            send_json(client_fd,json_str);
        }

        // 2. 处理下载操作
        else if (strcmp(req->valuestring, "download") == 0) {
            cJSON *file = cJSON_GetObjectItemCaseSensitive(root, "file");
            cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
            if (!cJSON_IsString(file) || !cJSON_IsNumber(version) || !file->valuestring || !version->valueint) {
                fprintf(stderr, "Invalid download format\n");
                goto error;
            }
            
            // 读取文件内容
            char *content = read_file(user_name,file->valuestring, version->valueint);
            if (NULL == content) {
                //文件不存在回复NO
                char *stat = "NO";
                char * json_str = generate_status_json(stat);
                send_json(client_fd,json_str);

                free(content); // 必须释放内存
                continue;
            }
            // 构建响应JSON(文件内容)
            cJSON *response = cJSON_CreateObject();
            cJSON_AddStringToObject(response, "content", content);
            free(content);
            printf("Download completed: %s\n", file->valuestring);
            
            //将JSON对象转换为字符串
            char *json_str = cJSON_PrintUnformatted(response);
            if (json_str == NULL) 
            {
                fprintf(stderr, "错误：生成JSON字符串失败\n");
                cJSON_Delete(response);
                goto error;
            }


            // 发送响应给客户端
            send_json(client_fd, json_str);
            free(json_str);          // 释放cJSON_PrintUnformatted生成的字符串
            cJSON_Delete(response);//删除response json对象

        }

        //3.处理查看操作
        else if (strcmp(req->valuestring, "look") == 0) 
        {
            //把用户所有文件及版本发送
            cJSON *file_versions = get_user_files_json(user_name);
            if (NULL == file_versions)
            {
                //文件不存在回复NO
                char *stat = "NO";
                char * json_str = generate_status_json(stat);
                send_json(client_fd,json_str);
                continue;
            }

            char *json_str = cJSON_Print(file_versions);
            send_json(client_fd, json_str);
            free(json_str);  // 释放临时字符串
            cJSON_Delete(file_versions);  // 释放 JSON 数组对象

            //接收json字节数和json字符串
            char *recvbuf = NULL;
            if (NULL == (recvbuf = receive_json(client_fd)))
            {
                printf("recv fail\n");
                goto error;
            }

            //josn字符串变成json结构体
            root = cJSON_Parse(recvbuf);
            if (!root) {
                fprintf(stderr, "JSON parse error: %s\n", cJSON_GetErrorPtr());
                goto error;
            }

        //发送用户要预览的版本
            cJSON *file = cJSON_GetObjectItemCaseSensitive(root, "file");
            cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "version");
            if (!cJSON_IsString(file) || !cJSON_IsNumber(version) || !file->valuestring || !version->valueint) {
                fprintf(stderr, "Invalid look format\n");
                goto error;
            }

            // 读取文件内容
            char *content = read_file(user_name,file->valuestring, version->valueint);
            if (NULL == content) {
                //文件不存在回复NO
                char *stat = "NO";
                test("read_fail")
                char * json_str = generate_status_json(stat);
                send_json(client_fd,json_str);

                free(content); // 必须释放内存
                continue;
            }
            // 构建响应JSON(文件内容)
            cJSON *response = cJSON_CreateObject();
            cJSON_AddStringToObject(response, "content", content);
            free(content);
            printf("look completed: %s\n", file->valuestring);

            //将JSON对象转换为字符串
            json_str = cJSON_PrintUnformatted(response);
            if (json_str == NULL) 
            {
                fprintf(stderr, "错误：生成JSON字符串失败\n");
                cJSON_Delete(response);
                goto error;
            }

            // 发送响应给客户端
            send_json(client_fd, json_str);
            free(json_str);          // 释放cJSON_PrintUnformatted生成的字符串
            cJSON_Delete(response);//删除response json对象
            
        }

        //4.登陆账号
        else if (strcmp(req->valuestring, "login") == 0) 
        {
            cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
            cJSON *passwd = cJSON_GetObjectItemCaseSensitive(root, "passwd");
            if (!cJSON_IsString(name) || !cJSON_IsString(passwd) || !name->valuestring || !passwd->valuestring) 
            {
                fprintf(stderr, "Invalid login format\n");
                goto error;
            }

            sqlite3 *db;
            if (NULL == (db = sqlite3_init()))
            {
                printf("sqlite3_init fail\n");
                goto error;
            }
            test("sqlite3 table open success")

                if (-1 == user_login(db, name->valuestring, passwd->valuestring))
                {
                    //登陆失败回复NO
                    char *stat = "NO";
                    char * json_str = generate_status_json(stat);
                    send_json(client_fd,json_str);
                }else
                {
                    //登陆成功回复OK
                    char *stat = "OK";
                    char * json_str = generate_status_json(stat);
                    send_json(client_fd,json_str);
                    
                    //通过作用域更大的变量保存用户名
                    int name_len = sizeof(name->valuestring);
                    user_name = (char *)malloc(name_len);
                    strcpy(user_name,name->valuestring);
                }
            sqlite3_close(db);

        }

        //5.注册账号
        else if (strcmp(req->valuestring, "register") == 0) 
        {
            cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
            cJSON *passwd = cJSON_GetObjectItemCaseSensitive(root, "passwd");
            if (!cJSON_IsString(name) || !cJSON_IsString(passwd) || !name->valuestring || !passwd->valuestring) 
            {
                fprintf(stderr, "Invalid register format\n");
                goto error;
            }

            sqlite3 *db;
            if (NULL == (db = sqlite3_init()))
            {
                printf("sqlite3_init fail\n");
                goto error;
            }
            test("sqlite3 table open success")

                if (-1 == user_register(db, name->valuestring, passwd->valuestring))
                {
                    //注册失败回复NO
                    char *stat = "NO";
                    char * json_str = generate_status_json(stat);
                    send_json(client_fd,json_str);
                }else
                {
                    //注册成功回复OK
                    char *stat = "OK";
                    char * json_str = generate_status_json(stat);
                    send_json(client_fd,json_str);
                }
            sqlite3_close(db);
        }



    }
error:
    free_version_list();// 释放文件版本管理链表内存
    free(user_name);//释放user_name动态内存
    cJSON_Delete(root);//删除root json对象
    close(client_fd);//关闭client_fd描述符号

    return NULL;
}

int main(int argc, char *argv[])
{ 
    int server_fd; 
    int client_fd;
    pthread_t tid;

    if (-1 == (server_fd = server_init("0.0.0.0",6666,10)))
    {
        printf("server_init fail\n");
        return -1;
    }
    test("server_init sucess")

    struct sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    while (1)
    {
        sleep(1);
        if (-1 == (client_fd = server_accept(server_fd)))
            continue;

        int *client_fd_p = &client_fd;
        if(0 != (pthread_create(&tid,NULL,client_handle,client_fd_p)))
        {
            perror("create fail");
            close(client_fd);
            return -1;
        }

         pthread_detach(tid);
    }
    
    close(server_fd);
    return 0;
} 
