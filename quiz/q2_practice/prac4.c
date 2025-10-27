// 撰寫一個「子行程監控器」父行程，其工作流程如下：

// 1.  父行程 (Manager) `fork` 出一個子行程 (Worker)。
// 2.  子行程 (Worker) 不會立刻開始工作，它會先 `raise(SIGSTOP)` 讓自己暫停，進入「準備就緒」狀態。
// 3.  父行程 (Manager) 會在 3 秒後，發送 `SIGCONT` 信號「啟動」子行程。
// 4.  在啟動子行程的**同時**，父行程設定一個 **5 秒**的 `alarm()` 作為「工作逾時」監控。
// 5.  子行程 (Worker) 的實際工作內容是 `sleep(10)`，這代表它**「注定會逾時」**。
// 6.  父行程 (Manager) 必須**安全地等待** `SIGCHLD` (子行程提早完成) 或 `SIGALRM` (子行程逾時) 這兩個事件的*其中之一*。

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // fork, sleep, alarm, kill, getpid, write
#include <signal.h>     // sigaction, sigset_t, sig...
#include <sys/wait.h>   // waitpid, WNOHANG
#include <errno.h>      // errno, ECHILD

/* --- 1. 全域變數 --- */
// 子行程 PID，必須是全域的，SIGALRM handler 才能存取到
static pid_t child_pid = 0;

// SIGCHLD handler 設定此旗標
static volatile sig_atomic_t child_reaped_flag = 0;
// SIGALRM handler 設定此旗標
static volatile sig_atomic_t timeout_flag = 0;


/* --- 2. 訊號處理函式 (父行程使用) --- */
void handler_int_parent(int signo) {
    // write() 是 async-signal-safe，在 handler 中使用 printf() 不安全
    write(STDOUT_FILENO, "\n[Parent] Ignored Ctrl+C.\n", 28);
}

void handler_chld(int signo) {
    int old_errno = errno; // 儲存 errno，避免 waitpid 汙染
    int status;
    pid_t pid;

    // 使用 WNOHANG 迴圈，回收所有可能已終止的子行程
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (pid == child_pid) {
            child_reaped_flag = 1;
            write(STDOUT_FILENO, "[Handler] SIGCHLD: Reaped child.\n", 33);
        }
    }
    
    // 恢復 errno
    errno = old_errno;
}

void handler_alrm(int signo) {
    int old_errno = errno;
    
    write(STDOUT_FILENO, "\n[Handler] SIGALRM: Timeout! Killing child...\n", 47);
    
    // 強制終止子行程 (SIGKILL 無法被攔截)
    if (child_pid > 0) {
        kill(child_pid, SIGKILL);
    }
    
    timeout_flag = 1;
    errno = old_errno;
}

void child_main() {
    // 2. Handler (子行程)：重設 SIGINT 為預設行為 (SIG_DFL)
    struct sigaction sa_child_int;
    sa_child_int.sa_handler = SIG_DFL; // 預設行為 (終止)
    sigemptyset(&sa_child_int.sa_mask);
    sa_child_int.sa_flags = 0;
    sigaction(SIGINT, &sa_child_int, NULL);

    // 3. 子行程邏輯：印出 PID 並自我暫停
    printf("[Child] PID %d: Ready. Stopping (waiting for SIGCONT)...\n", getpid());
    fflush(stdout); 
    
    raise(SIGSTOP); // 程式會在這裡暫停

    // --- 程式被 SIGCONT 喚醒後，會從這裡繼續 ---
    
    printf("[Child] PID %d: Resumed by SIGCONT. Working for 10 sec...\n", getpid());
    fflush(stdout);

    // 執行工作 (模擬)
    sleep(10); // 這裡會被父行程的 SIGKILL (來自 SIGALRM) 中斷

    printf("[Child] PID %d: Work done. Exiting.\n", getpid());
    exit(0);
}


/* --- 4. 父行程主邏輯 --- */

int main() {
    struct sigaction sa_int, sa_chld, sa_alrm;
    sigset_t block_mask, old_mask;

    printf("[Parent] Manager started. PID %d.\n", getpid());
    
    // 1. SIGINT Handler
    sa_int.sa_handler = handler_int_parent;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0; // 不使用 SA_RESTART，讓 sigsuspend 被中斷
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction(SIGINT)"); exit(1);
    }

    // 2. SIGCHLD Handler
    sa_chld.sa_handler = handler_chld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART; // 好習慣
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction(SIGCHLD)"); exit(1);
    }

    // 3. SIGALRM Handler
    sa_alrm.sa_handler = handler_alrm;
    sigemptyset(&sa_alrm.sa_mask);
    sa_alrm.sa_flags = 0;
    if (sigaction(SIGALRM, &sa_alrm, NULL) == -1) {
        perror("sigaction(SIGALRM)"); exit(1);
    }

    // --- (B) 封鎖關鍵訊號 ---
    // 在 fork 之前封鎖 SIGCHLD 和 SIGALRM
    // 防止子行程太快結束/逾時，在父行程呼叫 sigsuspend 之前就發送訊號
    printf("[Parent] Blocking SIGCHLD and SIGALRM...\n");
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGCHLD);
    sigaddset(&block_mask, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &block_mask, &old_mask) == -1) {
        perror("sigprocmask(SIG_BLOCK)"); exit(1);
    }

    // --- (C) Fork ---
    child_pid = fork();

    if (child_pid < 0) {
        perror("fork");
        exit(1);
    }

    if (child_pid == 0) {
        // --- 子行程 ---
        child_main(); // 執行子行程邏輯
        // (child_main 永遠不會返回)
        exit(1); // 備用
    }

    // --- (D) 啟動子行程 ---
    printf("[Parent] Child PID %d.\n", child_pid);
    printf("[Parent] Waiting 3 sec before starting child...\n");
    sleep(3);
    
    printf("[Parent] Sending SIGCONT to child.\n");
    kill(child_pid, SIGCONT);

    // --- (E) 計時 ---
    printf("[Parent] Setting 5 sec timeout alarm.\n");
    alarm(5);
    
    printf("[Parent] Waiting for SIGCHLD or SIGALRM... (Ctrl+C is ignored)\n");
    
    while (timeout_flag == 0 && child_reaped_flag == 0) {
        // 原子性地解除封鎖 (CHLD/ALRM) 並等待
        sigsuspend(&old_mask); 
    }
    
    printf("[Parent] Woke up. Event occurred.\n");
    
    // 第一時間取消鬧鐘 (如果子行程提早完成，鬧鐘可能還在跑)
    alarm(0);

    // 檢查旗標 (此時 CHLD/ALRM 仍被封鎖，可安全檢查)
    if (timeout_flag) {
        printf("[Parent] Result: Child timed out. Killed.\n");
        // 必須手動回收被 SIGKILL 終止的子行程
        // (因為 handler_alrm 發生時，子行程可能還沒死，
        // SIGCHLD 可能在 sigsuspend 迴圈外才到)
        handler_chld(0); 
    } else if (child_reaped_flag) {
        printf("[Parent] Result: Child finished on time.\n");
    }

    // --- (H) 恢復 ---
    printf("[Parent] Restoring original mask and exiting.\n");
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    
    return 0;
}
