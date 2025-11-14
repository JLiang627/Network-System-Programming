#define _GNU_SOURCE          // 啟用 strsignal 函式
#define _XOPEN_SOURCE 700    // 確保 POSIX 函式可用

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

// --- 輔助函式：替換 tlpi_hdr.h/signal_functions.h 中的功能 ---

static void 
errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

// 函式：將訊號集轉換為可讀字串（簡化版）
static int
printSigSet(FILE *of, const char *msg, const sigset_t *ss)
{
    int sig, cnt = 0;
    
    // 輸出開頭信息
    fprintf(of, "%s", msg);

    for (sig = 1; sig < NSIG; sig++) {
        // 檢查訊號是否在集合中
        if (sigismember(ss, sig)) {
            cnt++;
            fprintf(of, "%d (%s) ", sig, strsignal(sig));
        }
    }
    
    if (cnt == 0)
        fprintf(of, "<empty signal set>");

    fprintf(of, "\n");
    return cnt;
}

// 輔助函式：顯示當前行程的訊號遮罩 (原 printSigMask)
static void
printSigMask(FILE *of, const char *msg)
{
    sigset_t currMask;
    if (sigprocmask(SIG_SETMASK, NULL, &currMask) == -1) {
        errExit("sigprocmask - printMask");
    }
    printSigSet(of, msg, &currMask);
}

// 輔助函式：顯示當前行程的待處理訊號 (原 printPendingSigs)
static void
printPendingSigs(FILE *of, const char *msg)
{
    sigset_t pendingSigs;
    if (sigpending(&pendingSigs) == -1) {
        errExit("sigpending");
    }
    printSigSet(of, msg, &pendingSigs);
}


// --- 核心邏輯：變數與處理器 ---

static volatile sig_atomic_t gotSigquit = 0; // 退出標誌

static void
handler(int sig)
{
    // [UNSAFE]: 在 Handler 中使用 printf/strsignal，為演示目的犧牲安全性
    printf("\n[Handler] Caught signal %d (%s)\n", sig, strsignal(sig));

    if (sig == SIGQUIT)
        gotSigquit = 1; // 設定標誌，讓主迴圈退出
}


// --- 主程式 ---

int
main(int argc, char *argv[])
{
    int loopNum;
    time_t startTime;
    sigset_t origMask, blockMask;
    struct sigaction sa;

    // 顯示初始訊號遮罩
    printSigMask(stdout, "Initial signal mask is:\n");

    // 設定要阻擋的訊號集合 (SIGINT 和 SIGQUIT)
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGINT);
    sigaddset(&blockMask, SIGQUIT);

    // [Step 1] 阻擋 SIGINT 和 SIGQUIT，並儲存原始遮罩
    if (sigprocmask(SIG_BLOCK, &blockMask, &origMask) == -1)
        errExit("sigprocmask - SIG_BLOCK");

    // 設定訊號處理器
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;          // 不使用 SA_RESTART, SA_SIGINFO 等
    sa.sa_handler = handler;

    // [Step 2] 註冊 SIGINT 和 SIGQUIT 的處理器
    if (sigaction(SIGINT, &sa, NULL) == -1)
        errExit("sigaction - SIGINT");
    if (sigaction(SIGQUIT, &sa, NULL) == -1)
        errExit("sigaction - SIGQUIT");

    // 提示用戶 PID
    printf("程式 PID: %ld\n", (long)getpid());
    printf("請在其他終端機使用 kill -2 或 kill -3 測試...\n");

    // --- 主迴圈：等待 SIGQUIT ---

    for (loopNum = 1; !gotSigquit; loopNum++) {
        printf("\n=== LOOP %d\n", loopNum);

        // 顯示當前遮罩 (應顯示 SIGINT/SIGQUIT 被阻擋)
        printSigMask(stdout, "Starting critical section, signal mask is:\n");

        /* 模擬執行關鍵程式碼 (4 秒) */
        for (startTime = time(NULL); time(NULL) < startTime + 4; )
            continue; 

        // 顯示當前待處理訊號
        printPendingSigs(stdout, "Before sigsuspend() - pending signals:\n");

        // [Step 3] 原子性等待：解除阻擋訊號並掛起
        // sigsuspend(&origMask) 會暫時將遮罩設為 origMask (解除阻擋)，然後等待訊號
        printf("[Action] Calling sigsuspend(), unblocking SIGINT/SIGQUIT...\n");
        if (sigsuspend(&origMask) == -1) {
            if (errno != EINTR) // 正常返回會是 -1 且 errno=EINTR
                errExit("sigsuspend");
        }
        
        // sigsuspend 返回後，遮罩會自動還原為 SIGINT/SIGQUIT 阻擋的狀態
        printf("[Action] sigsuspend() returned. Mask restored.\n");
    }

    // --- 迴圈結束與清理 ---

    // [Step 4] 最終還原訊號遮罩到程式啟動時的狀態
    if (sigprocmask(SIG_SETMASK, &origMask, NULL) == -1)
        errExit("sigprocmask - SIG_SETMASK final restore");
    
    printSigMask(stdout, "=== Exited loop\nRestored final signal mask to:\n");

    exit(EXIT_SUCCESS);
}