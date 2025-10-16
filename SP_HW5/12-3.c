#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>    

#define MAX_PATH_LEN 256
#define BUF_SIZE 512

// 檢查字串是否全為數字 
int is_number_dir(const char *s) {
    return strspn(s, "0123456789") == strlen(s) && strlen(s) > 0;
}

// 讀取程序命令名稱
void get_command_name(const char *pid_str, char *command_name, size_t size) {
    char comm_path[MAX_PATH_LEN];
    int fd_comm;
    ssize_t read_len;

    if (snprintf(comm_path, sizeof(comm_path), "/proc/%s/comm", pid_str) >= sizeof(comm_path)) {
        strncpy(command_name, "[Path too long]", size);
        return;
    }

    // 打開comm 檔案
    fd_comm = open(comm_path, O_RDONLY); 
    if (fd_comm != -1) {
        // 讀取命令名稱
        read_len = read(fd_comm, command_name, size - 1);
        close(fd_comm);

        if (read_len > 0) {
            // 確保字串終止
            command_name[read_len] = '\0';
            // 移除尾隨的換行符和空字符
            command_name[strcspn(command_name, "\n\r")] = '\0';
        } else {
            strncpy(command_name, "[unknown cmd]", size);
        }
    } else {
        strncpy(command_name, "[access denied]", size);
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2 || strcmp(argv[1], "--help") == 0) {
        fprintf(stderr, "Usage: %s <file_pathname>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *target_path = argv[1];
    printf("Scanning for processes using: %s\n", target_path);
    printf("------------------------------------------------------------------\n");

    // 宣告變數
    DIR *dirp = opendir("/proc");
    DIR *fd_direc;
    struct dirent *dp, *fd_dp;
    ssize_t link_len; 
    char pid_path[MAX_PATH_LEN];
    char full_link_path[MAX_PATH_LEN];
    char target_read_path[MAX_PATH_LEN];
    char command_name[BUF_SIZE];
    int found_count = 0;

    if (dirp == NULL) {
        perror("Failed to open /proc");
        return EXIT_FAILURE;
    }

    while ((dp = readdir(dirp)) != NULL) {
        if (is_number_dir(dp->d_name)) { // 使用輔助函數檢查 PID
            
            // 構建 /proc/PID/fd 路徑
            if (snprintf(pid_path, sizeof(pid_path), "/proc/%s/fd", dp->d_name) >= sizeof(pid_path)) {
                continue; 
            }
            
            fd_direc = opendir(pid_path); 
            if (fd_direc == NULL){
                if (errno != ENOENT) { // 忽略 ENOENT (No such file)
                    fprintf(stderr, "Warning: need to use sudo "); // 可以選擇輸出警告
                    perror(pid_path);
                }
                continue;
            }
            
            // 修正：使用 PathInFd 追蹤是否找到，避免重複輸出
            int path_in_fd_found = 0; 
            
            while ((fd_dp = readdir(fd_direc)) != NULL) {
                
                // 忽略特殊條目 "." 和 ".."
                if (strcmp(fd_dp->d_name, ".") == 0 || strcmp(fd_dp->d_name, "..") == 0) {
                    continue;
                }
                
                // 構建符號連結的完整路徑：/proc/[PID]/fd/[n]
                if (snprintf(full_link_path, sizeof(full_link_path), "%s/%s", pid_path, fd_dp->d_name) >= sizeof(full_link_path)) {
                    continue;
                }

                // 讀取符號連結的目標
                link_len = readlink(full_link_path, target_read_path, sizeof(target_read_path) - 1);

                if (link_len > 0) {
                    target_read_path[link_len] = '\0'; // 手動添加空終止符
                    // 執行最終比對
                    if (strcmp(target_read_path, target_path) == 0) {
                        // 確保只有找到匹配才執行一次命令獲取和輸出
                        if (!path_in_fd_found) {
                            get_command_name(dp->d_name, command_name, sizeof(command_name));
                            
                            if (found_count > 0) {
                                printf("------------------------------------------------------------------\n");
                            }
                            printf("PID: %s\n", dp->d_name);
                            printf("CMD: %s\n", command_name);
                            found_count++;
                            path_in_fd_found = 1;
                        }
                        
                        // 輸出檔案描述符 (一個程序可能透過多個FD開啟同一個檔案)
                        printf("FD:  %s\n", fd_dp->d_name);
                        // 注意：如果找到，不 break，因為該程序可能透過多個 FD 開啟同一個檔案。
                    }
                } 
            } 
            
            // 內層迴圈結束後，必須關閉目錄句柄
            closedir(fd_direc);
        }
    }

    closedir(dirp);
    printf("------------------------------------------------------------------\n");
    printf("Total %d process(es) found using file: %s\n", found_count, target_path);

    return EXIT_SUCCESS;
}
