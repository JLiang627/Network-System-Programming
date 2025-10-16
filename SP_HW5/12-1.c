#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <errno.h>

#define STATUS_BUF_SIZE 4096    
#define MAX_PATH_LEN 256        
#define MAX_CMD_NAME 256        

void errExit(const char *msg)
{
    int savedErrno = errno;
    fprintf(stderr, "%s: %s\n", msg, strerror(savedErrno));
    exit(EXIT_FAILURE);
}

uid_t userIdFromName(const char *name)
{
    struct passwd *pwd;
    uid_t u;
    char *endptr;

    if (name == NULL || *name == '\0') { 
        return getuid();
    }

    u = strtoul(name, &endptr, 10); 
    if (*endptr == '\0') return u; 
    
    errno = 0; 
    pwd = getpwnam(name);

    if (pwd == NULL) {
        if (errno == 0)
            fprintf(stderr, "User '%s' not found\n", name);
        else 
            errExit("getpwnam");
        exit(EXIT_FAILURE);
    }

    return pwd->pw_uid;
}


int main(int argc, char *argv[]) {
    // 檢查參數數量 (確保有 argv[1])
    if (argc != 2 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr, "Usage: %s <username>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // 變數宣告
    struct dirent *dp;
    int fd;
    char statusPath[MAX_PATH_LEN]; // 用於路徑
    char buf[STATUS_BUF_SIZE];     // 用於讀取狀態檔內容
    ssize_t num_read;
    uid_t proc_ruid; 
    char command_name[MAX_CMD_NAME]; // 用於命令名稱
    
    uid_t target_userID = userIdFromName(argv[1]);
    
    DIR *dirp = opendir("/proc");
    if (dirp == NULL) errExit("opendir /proc");
    
    printf("%-8s %s\n", "PID", "COMMAND");
    printf("---------------------------\n");
    
    while ((dp = readdir(dirp)) != NULL) {
        
        // 篩選 PID 目錄
        if (strspn(dp->d_name, "0123456789") == strlen(dp->d_name) && strlen(dp->d_name) > 0) {
        
            // 構建路徑 
            if (snprintf(statusPath, sizeof(statusPath), "/proc/%s/status", dp->d_name) >= sizeof(statusPath)) {
                continue; 
            }
            
            fd = open(statusPath, O_RDONLY);
            if (fd == -1) {
                if (errno == ENOENT) {
                    continue; // 程序消失，跳過
                } else {
                    // 報告非 ENOENT 的系統錯誤
                    fprintf(stderr, "Warning: Failed to open %s: %s\n", statusPath, strerror(errno));
                    continue;
                }
            }
            
            num_read = read(fd, buf, STATUS_BUF_SIZE - 1);
            close(fd); // 立即關閉
            
            if (num_read <= 0) continue; 
            buf[num_read] = '\0'; // 確保字串終止

            // 解析 Uid: 欄位
            char *uid_line = strstr(buf, "Uid:");
            
            // 檢查是否找到 Uid: 行並成功解析第一個數字（真實 UID）
            if (uid_line != NULL && sscanf(uid_line, "Uid:\t%u", &proc_ruid) == 1) {
                
                // UID 比對
                if (proc_ruid == target_userID) {
                    
                    // 如果匹配，解析 Name: 欄位
                    char *name_line = strstr(buf, "Name:");
                    if (name_line != NULL && sscanf(name_line, "Name:\t%255s", command_name) == 1) {
                        printf("%-8s %s\n", dp->d_name, command_name);
                    }
                }
            }
        }
    }
    
    closedir(dirp);
    return EXIT_SUCCESS;
}
