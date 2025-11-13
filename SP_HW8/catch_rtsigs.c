#define _GNU_SOURCE           // 啟用 strsignal, sigqueue 等相關 GNU 擴展
#define _XOPEN_SOURCE 700     // 確保 POSIX 函式可用

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// --- 輔助函式：替換 tlpi_hdr.h 中的功能 ---

static void 
errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

// 簡單的字符串轉整數，帶有錯誤檢查 (僅用於命令行參數)
static long 
getLongArg(const char *s, int flags)
{
    char *endptr;
    long res = strtol(s, &endptr, 10);
    
    if (*endptr != '\0' || s == endptr) {
        fprintf(stderr, "Invalid number: %s\n", s);
        exit(EXIT_FAILURE);
    }
    if ((flags & 1) && res < 0) { // GN_NONNEG is 0x01, checking non-negative
        fprintf(stderr, "Value must be non-negative: %s\n", s);
        exit(EXIT_FAILURE);
    }
    if ((flags & 2) && res <= 0) { // GN_GT_0 is 0x02, checking greater than 0
        fprintf(stderr, "Value must be greater than 0: %s\n", s);
        exit(EXIT_FAILURE);
    }
    return res;
}

static void 
usageErr(const char *format)
{
    fprintf(stderr, format, "./catch_rtsigs");
    fprintf(stderr, " [block-time [handler-sleep-time]]\n");
    exit(EXIT_FAILURE);
}

// --- 核心變數與處理器 ---

static volatile int handlerSleepTime;
static volatile int allDone = 0;

static void
siginfoHandler(int sig, siginfo_t *si, void *ucontext)
{    
    if (sig == SIGINT || sig == SIGTERM) {
        allDone = 1;
        return;
    }

    printf("caught signal %d\n", sig);

    printf("    si_signo=%d, si_code=%d (%s), ", si->si_signo, si->si_code,
           (si->si_code == SI_USER) ? "SI_USER" :
           (si->si_code == SI_QUEUE) ? "SI_QUEUE" : "other");

    printf("si_value=%d\n", si->si_value.sival_int);
    printf("    si_pid=%ld, si_uid=%ld\n", (long) si->si_pid, (long) si->si_uid);
    
    sleep(handlerSleepTime);
}

// --- 主程式 ---

int
main(int argc, char *argv[])
{
    struct sigaction sa;
    int sig;
    sigset_t prevMask, blockMask;
    long blockTime;

    if (argc > 1 && strcmp(argv[1], "--help") == 0)
        usageErr("%s [block-time [handler-sleep-time]]\n");

    printf("%s: PID is %ld\n", argv[0], (long) getpid());

    // 參數 2: 處理器執行時長 (預設 1)
    handlerSleepTime = (argc > 2) ?
        (int) getLongArg(argv[2], 1) : 1; // 1 for GN_NONNEG (non-negative)

    // 設定處理器
    sa.sa_sigaction = siginfoHandler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART; 
    
    // 處理器執行期間，阻擋所有其他訊號
    sigfillset(&sa.sa_mask);

    // 為大多數訊號建立處理器 (跳過 SIGTSTP, SIGQUIT)
    for (sig = 1; sig < NSIG; sig++) {
        // NSIG 是系統定義的訊號總數，可能因系統而異。
        if (sig != SIGTSTP && sig != SIGQUIT) {
            // 使用 &sa 註冊處理器，NULL 意為不儲存舊設定
            if (sigaction(sig, &sa, NULL) == -1) { 
                // 忽略 SIGKILL 和 SIGSTOP 的錯誤
                if (errno != EINVAL) 
                    errExit("sigaction failed");
            }
        }
    }

    // 參數 1: 選擇性阻擋訊號並睡眠
    if (argc > 1) {
        blockTime = getLongArg(argv[1], 2); // 2 for GN_GT_0 (greater than 0)
        
        // 設定要阻擋的訊號遮罩
        sigfillset(&blockMask);
        sigdelset(&blockMask, SIGINT);   // 不阻擋 SIGINT
        sigdelset(&blockMask, SIGTERM);  // 不阻擋 SIGTERM

        // 實際阻擋訊號並儲存舊遮罩
        if (sigprocmask(SIG_SETMASK, &blockMask, &prevMask) == -1)
            errExit("sigprocmask - SIG_SETMASK");

        printf("%s: signals blocked - sleeping %ld seconds\n", argv[0], blockTime);
        
        // 程式在此處阻塞
        sleep(blockTime);

        printf("%s: sleep complete\n", argv[0]);

        // 還原舊的訊號遮罩，此時所有 Pending 訊號會被遞送
        if (sigprocmask(SIG_SETMASK, &prevMask, NULL) == -1)
            errExit("sigprocmask - Restore");
    }

    // 主迴圈，等待訊號喚醒
    while (!allDone)
        pause();

    printf("\n%s: Exited main loop.\n", argv[0]);
    return EXIT_SUCCESS;
}