/*===============================================
*   文件名称：user_file_save.h
*   创 建 者：熊袋然    
*   创建日期：2025年07月27日
*   描    述：
================================================*/
#ifndef USER_FILE_SAVE_H
#define USER_FILE_SAVE_H

#include <stdio.h>

/**
 * @file file_manager.h
 * @brief 文件版本管理系统的公共接口
 * 
 * 提供文件保存和版本控制功能，支持为同一文件名自动创建不同版本，
 * 并按用户目录组织文件结构。
 */

/** 最大路径长度（包括终止符） */
#define MAX_PATH_LENGTH 256
/** 最大文件名长度（包括终止符） */
#define MAX_FILENAME_LENGTH 50
/** 最大文件内容长度（包括终止符） */
#define MAX_CONTENT_LENGTH 1024

/**
 * @brief 初始化文件版本管理系统
 * 
 * 必须在调用其他文件管理函数前调用此函数，用于创建内部数据结构。
 * 程序结束时需调用 free_version_list() 释放资源。
 */
void init_version_list(void);

/**
 * @brief 保存文件并自动管理版本
 * 
 * 按用户名和文件名保存文件内容，相同文件名自动创建新版本。
 * 文件保存路径格式为：user_files/用户名/version_版本号/文件名
 * 
 * @param username 用户名（用于组织文件目录，需确保不包含路径分隔符）
 * @param filename 文件名（不含路径，需确保不包含路径分隔符）
 * @param content 文件内容（字符串形式）
 * @return 成功返回0，失败返回-1（如内存分配失败、目录创建失败等）
 * 
 * @note 首次保存文件时版本号为1，后续相同文件名的保存操作版本号自动递增。
 * @warning 用户名和文件名长度不得超过 MAX_FILENAME_LENGTH，
 *          完整路径长度不得超过 MAX_PATH_LENGTH。
 */
int save_file(const char *username, const char *filename, const char *content);

/**
 * 读取指定用户、文件名和版本的文件内容
 * @param username 用户名（对应user_files下的子目录）
 * @param filename 文件名（不含路径）
 * @param version  文件版本号
 * @return 成功返回文件内容的字符串指针（需调用者free），失败返回NULL
 * 
 * @note 文件路径格式为：user_files/用户名/version_版本号/文件名
 * @warning 返回的内存需由调用者负责释放，避免内存泄漏
 */
char *read_file(const char *username, const char *filename, int version);

/**
 * @brief 释放文件版本管理系统占用的内存
 * 
 * 必须在程序结束前调用，用于释放内部链表结构占用的内存，
 * 避免内存泄漏。
 */
void free_version_list(void);

#endif
