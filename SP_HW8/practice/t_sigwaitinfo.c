#define _GNU_SOURCE           // 啟用 strsignal, sigqueue 等相關 GNU 擴展
#define _XOPEN_SOURCE 700     // 確保 POSIX 函式可用

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// --- 輔助函式：替代 tlpi_hdr.h 中的功能 ---

static void 
errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

// 簡單的字符串轉整數，帶有錯誤檢查 (僅用於命令行參數)
// 替代 getInt(argv[1], GN_GT_0, "delay-secs")
static long 
getDelaySecs(const char *s)
{
    char *endptr;
    long res = strtol(s, &endptr, 10);
    
    if (*endptr != '\0' || s == endptr) {
        fprintf(stderr, "Invalid number: %s\n", s);
        exit(EXIT_FAILURE);
    }
    // 檢查 GN_GT_0 (greater than 0)
    if (res <= 0) {
        fprintf(stderr, "Delay seconds must be greater than 0: %s\n", s);
        exit(EXIT_FAILURE);
    }
    return res;
}

static void 
usageErr(const char *progName)
{
    fprintf(stderr, "Usage: %s [delay-secs]\n", progName);
    exit(EXIT_FAILURE);
}

// --- 核心程式碼：同步等待訊號 ---

int
main(int argc, char *argv[])
{
    int sig;
    siginfo_t si;       // 用於接收訊號詳細資訊
    sigset_t allSigs;   // 包含所有訊號的集合
    long delaySecs = 0;

    // 檢查參數，並解析延遲時間
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0)
            usageErr(argv[0]);
        
        delaySecs = getDelaySecs(argv[1]);
    }

    printf("%s: PID is %ld\n", argv[0], (long) getpid());

    /* Block all signals (except SIGKILL and SIGSTOP) */
    // 初始化並填滿 allSigs 集合
    if (sigfillset(&allSigs) == -1) 
        errExit("sigfillset");

    // 實際阻擋所有訊號 (除了無法被阻擋的 SIGKILL 和 SIGSTOP)
    if (sigprocmask(SIG_SETMASK, &allSigs, NULL) == -1)
        errExit("sigprocmask");

    printf("%s: signals blocked\n", argv[0]);

    if (argc > 1) {
        /* Delay so that signals can be sent to us and enter pending state */
        printf("%s: about to delay %ld seconds\n", argv[0], delaySecs);

        // 阻塞性睡眠，讓訊號在此期間累積在 Pending 隊列中
        sleep(delaySecs);

        printf("%s: finished delay\n", argv[0]);
    }

    for (;;) {
        /* Fetch signals until SIGINT (^C) or SIGTERM */
        
        // 核心操作：同步等待 allSigs 集合中的任一訊號到達 [cite: 633-635]
        // 即使訊號已被阻擋，sigwaitinfo() 仍可接受它
        sig = sigwaitinfo(&allSigs, &si); 

        if (sig == -1) {
            // sigwaitinfo 不應該失敗，除非發生嚴重錯誤
            errExit("sigwaitinfo"); 
        }

        // 檢查是否收到終止訊號 [cite: 662]
        if (sig == SIGINT || sig == SIGTERM)
            exit(EXIT_SUCCESS);

        // 輸出接收到的訊號編號及名稱
        printf("got signal: %d (%s)\n", sig, strsignal(sig));
        
        // 輸出 siginfo_t 中的詳細資訊 [cite: 665-678]
        printf("    si_signo=%d, si_code=%d (%s), si_value=%d\n",
               si.si_signo, si.si_code,
               (si.si_code == SI_USER) ? "SI_USER" :
               (si.si_code == SI_QUEUE) ? "SI_QUEUE" : "other",
               si.si_value.sival_int);

        printf("    si_pid=%ld, si_uid=%ld\n",
               (long) si.si_pid, (long) si.si_uid);
    }
}