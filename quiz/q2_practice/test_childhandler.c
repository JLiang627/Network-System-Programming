#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

/* * childhandler - SIGCHLD 信號處理函式
 * 負責回收所有已終止的子程序（殭屍程序），避免資源洩漏。
 */
void childhandler (int signo) {
    int status;
    int saveerrno;
    pid_t pid;
    extern int errno; 

    // 儲存原始的 errno 值
    saveerrno = errno;

    // 進入迴圈，回收所有待處理的已終止子程序
    for (;;) {
        // waitpid(-1, &status, WNOHANG)
        // -1: 等待任何子程序
        // WNOHANG: 非阻塞模式，確保信號處理器不會停滯
        pid = waitpid(-1, &status, WNOHANG);

        if (pid == 0) {
            /* 情況 1: 沒有更多的已終止子程序可供回收。退出。 */
            break; 
        } else if (pid == -1 && errno == ECHILD) {
            /* 情況 2: 沒有任何子程序 (活著或已終止)。退出。 */
            break;
        } else if (pid == -1) {
            /* 情況 3: 非預期的 waitpid 系統呼叫錯誤。*/
            perror("waitpid");
            // 由於這是處理器內部發生的嚴重錯誤，選擇終止程序
            abort(); 
        }
    }
    errno = saveerrno;
    return;
}

int main() {
    struct sigaction sa;

    printf("父程序 PID: %d\n", getpid());

    // 設定 SIGCHLD 處理器
    sa.sa_handler = childhandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; 

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction(SIGCHLD)");
        return 1;
    }

    // 創建兩個child
    for (int i = 0; i < 2; i++) {
        pid_t child_pid = fork();
        if (child_pid == 0) {
            // child process
            printf("子程序 %d 啟動，PID: %d\n", i+1, getpid());
            sleep(i + 1); // child process run 1 秒和 2 秒
            printf("子程序 %d 終止\n", i+1);
            exit(0);
        } else if (child_pid < 0) {
            perror("fork");
        }
    }

    printf("parent進入無限迴圈等待child信號...\n");
    // parent進入迴圈等待 SIGCHLD 信號
    while (1) {
        sleep(1); 
    }

    return 0;
}

