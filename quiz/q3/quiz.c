#define _GNU_SOURCE // 為了使用 gettid()
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/syscall.h> // 為了獲取執行緒ID (TID)

// 獲取 Linux 核心執行緒 ID (TID) 的輔助函式
pid_t gettid() {
    return syscall(SYS_gettid);
}

void sig_handler(int sig) {
    printf("程序/群組 ID (PID/TGID) : %-15d\n", getpid());    // TGID (Thread Group ID) 等同於程序 ID (PID)
    printf("處理執行緒 ID (TID) : %-15ld\n", (long)gettid());    // TID (Thread ID) 是核心識別單一執行緒的 ID
}

void* worker_thread(void* arg) {
    char* name = (char*)arg;
    pid_t tid = gettid();
    pthread_t ptid = pthread_self();

    printf("[%s] Thread啟動。 TID: %ld, pthread_t: %lu\n", name, (long)tid, (unsigned long)ptid);

    // 設定 SIGUSR1 的訊號處理器
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction error");
        return NULL;
    }

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    // 移除 SIGUSR1 訊號，讓thread可以接收它
    if (pthread_sigmask(SIG_UNBLOCK, &mask, NULL) != 0) {
        perror("pthread_sigmask UNBLOCK 失敗");
    }

    printf("[%s] 執行緒正在等待訊號...\n", name);

    sigset_t empty_mask;
    sigemptyset(&empty_mask);
    while (1) {
        sigsuspend(&empty_mask); 
    }
    
    return NULL;
}

int main() {
    pthread_t t1, t2;  // 建立兩個thread做比較
    sigset_t set;

    // A. 在主執行緒中阻塞 SIGUSR1，防止主執行緒意外處理訊號。
    // 這確保了只有我們的工作執行緒可以處理它（因為它們在 worker_thread 內顯式解除了阻塞）。
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("pthread_sigmask BLOCK in main 失敗");
        return 1;
    }

    printf("主程序啟動。 PID (TGID): %d\n", getpid());
    printf("==========================================\n");

    // B. 創建兩個工作執行緒
    pthread_create(&t1, NULL, worker_thread, "Thread A (t1)");
    pthread_create(&t2, NULL, worker_thread, "Thread B (t2)");

    // 等一下thread去處理各自的signal handler
    sleep(1);

    // 測試 1: pthread_kill()，指定其中一個thread
    printf("\n指定用Thread A做測試...\n");
    if (pthread_kill(t1, SIGUSR1) == 0) {
        printf("發送成功\n");
    } else {
        perror("pthread_kill 失敗");
    }
    sleep(1);

    // 測試 2: kill()，從group裡面任選一個thread
    printf("\n從group裡面挑一個thread做測試...\n");
    if (kill(getpid(), SIGUSR1) == 0) {
        printf("發送成功\n");
    } else {
        perror("kill 失敗");
    }
    sleep(3);
    
    // 清理：取消並等待執行緒結束 (因為它們在無限迴圈中)
    pthread_cancel(t1);
    pthread_cancel(t2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("\n==========================================\n");
    printf("程序演示結束。\n");
    return 0;
}
