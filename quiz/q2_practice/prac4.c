// #### 題目情境

// 您要撰寫一個「子行程監控器」父行程，其工作流程如下：

// 1.  父行程 (Manager) `fork` 出一個子行程 (Worker)。
// 2.  子行程 (Worker) 不會立刻開始工作，它會先 `raise(SIGSTOP)` 讓自己暫停，進入「準備就緒」狀態。
// 3.  父行程 (Manager) 會在 3 秒後，發送 `SIGCONT` 信號「啟動」子行程。
// 4.  在啟動子行程的**同時**，父行程設定一個 **5 秒**的 `alarm()` 作為「工作逾時」監控。
// 5.  子行程 (Worker) 的實際工作內容是 `sleep(10)`，這代表它**「注定會逾時」**。
// 6.  父行程 (Manager) 必須**安全地等待** `SIGCHLD` (子行程提早完成) 或 `SIGALRM` (子行程逾時) 這兩個事件的*其中之一*。

// #### 具體要求 (必須全部滿足)

// 1.  **Handler (父行程)**：
//     * **`SIGINT` (Ctrl+C)**：安裝一個 `SIGINT` 處理函式，它只會印出訊息「`[Parent] Ignored Ctrl+C.`」。（父行程必須能抵抗 Ctrl+C）
//     * **`SIGCHLD`**：安裝一個 `SIGCHLD` 處理函式。它**必須**使用 `waitpid(..., WNOHANG)` 迴圈來回收子行程，並設定一個 `volatile sig_atomic_t child_reaped_flag = 1;`
//     * **`SIGALRM`**：安裝一個 `SIGALRM` 處理函式。此函式是「逾時處理器」，它必須**強制終止**子行程 (`kill(child_pid, SIGKILL)`)，並設定一個 `volatile sig_atomic_t timeout_flag = 1;`

// 2.  **Handler (子行程)**：
//     * **`SIGINT`**：子行程**必須**重設 (reset) `SIGINT` 的處理方式為 `SIG_DFL` (預設行為)，這樣 Ctrl+C 才會終止子行程。

// 3.  **子行程邏G (Worker)**：
//     * `fork` 之後，子行程第一件事是重設 `SIGINT` handler。
//     * 接著印出自己的 PID 並 `raise(SIGSTOP)` 讓自己暫停。
//     * 當它收到 `SIGCONT` 恢復執行後，它會印出「開始工作」，執行 `sleep(10)`，然後 `exit(0)`。

// 4.  **父行程主邏輯 (Manager) - (本題核心)**：
//     * **(A) 安裝**：安裝上述 3 個 handler。
//     * **(B) 封鎖**：**在 `fork` 之前**，使用 `sigprocmask(SIG_BLOCK, ...)` 封鎖 `SIGCHLD` 和 `SIGALRM`。 (這是為了防止 `fork` 和 `sigsuspend` 之間的競態條件)。
//     * **(C) Fork**：`fork()` 並儲存 `child_pid` (全域變數或傳入 handler)。
//     * **(D) 啟動**：印出 PID，`sleep(3)`，然後 `kill(child_pid, SIGCONT)` 啟動子行程。
//     * **(E) 計時**：`alarm(5)` 設定 5 秒逾時。
//     * **(F) 安全等待 (最關鍵)**：
//         * 建立一個 `suspend_mask`，它**允許** `SIGCHLD` 和 `SIGALRM`，但**封鎖** `SIGINT`。
//         * 呼叫 `sigsuspend(&suspend_mask)` 讓父行程**原子性地解除封鎖並開始等待**。
//     * **(G) 判斷結果**：
//         * `sigsuspend` 返回後 (代表 `SIGCHLD` 或 `SIGALRM` 發生了)，**第一時間取消鬧鐘** `alarm(0)`。
//         * **重新封鎖** `SIGCHLD` 和 `SIGALRM` (使用 `sigprocmask`) 以便安全地檢查 flag。
//         * 檢查 `child_reaped_flag` 和 `timeout_flag`。
//         * 如果 `timeout_flag` 為 1，印出「監控結果：子行程逾時，已終止。」
//         * 如果 `child_reaped_flag` 為 1，印出「監控結果：子行程準時完成。」(在本題情境下不應發生)
//     * **(H) 恢復**：最後，恢復 (unblock) `SIGCHLD` 和 `SIGALRM`。

// #### 🌟 額外挑戰 (Bonus)

// 如果您成功完成了上述要求 (子行程被 `SIGALRM` 終止)，請修改子行程的 `sleep(10)` 為 `sleep(2)`。

// 在這種情況下，您的程式**必須**正確地顯示「監控結果：子行程準時完成。」—— 這將考驗您的 `SIGCHLD` 處理邏輯是否同樣健壯。

/*
 * 🎛️ 全方位綜合練習：限時執行的子行程監控器
 *
 * 這份程式碼實作了一個父行程 (Manager) 監控一個子行程 (Worker) 的場景。
 * * 核心技術：
 * 1. sigaction: 安裝三個不同的 handler (INT, CHLD, ALRM)。
 * 2. fork/kill: 跨行程訊號通訊 (SIGSTOP, SIGCONT, SIGKILL)。
 * 3. sigprocmask/sigsuspend:
 * - 在 fork 前封鎖 SIGCHLD/SIGALRM，防止競態條件。
 * - 使用 sigsuspend() 安全地、原子性地解除封鎖並等待訊號。
 * 4. volatile sig_atomic_t: 在 handler 和 main 之間安全地傳遞旗標。
 * 5. waitpid(WNOHANG): 在 SIGCHLD handler 中正確回收殭屍行程。
 */

// 啟用 POSIX 擴充功能 (例如 sigaction)，必須在所有 #include 之前
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // fork, sleep, alarm, kill, getpid, write
#include <signal.h>     // sigaction, sigset_t, sig...
#include <sys/wait.h>   // waitpid, WNOHANG
#include <errno.h>      // errno, ECHILD

/* --- 1. 全域變數 --- */
// 必須使用 volatile sig_atomic_t 確保在 handler 中
// 對其的修改是原子的，且不會被編譯器最佳化掉。

// 子行程 PID，必須是全域的，SIGALRM handler 才能存取到
static pid_t child_pid = 0;

// SIGCHLD handler 設定此旗標
static volatile sig_atomic_t child_reaped_flag = 0;
// SIGALRM handler 設定此旗標
static volatile sig_atomic_t timeout_flag = 0;


/* --- 2. 訊號處理函式 (父行程使用) --- */

/**
 * @brief SIGINT (Ctrl+C) 處理函式 (父行程用)
 * 題目要求：僅印出訊息，父行程必須能抵抗 Ctrl+C
 */
void handler_int_parent(int signo) {
    // write() 是 async-signal-safe，在 handler 中使用 printf() 不安全
    write(STDOUT_FILENO, "\n[Parent] Ignored Ctrl+C.\n", 28);
}

/**
 * @brief SIGCHLD 處理函式 (父行程用)
 * 題目要求：使用 WNOHANG 迴圈回收子行程，並設定旗標
 */
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

/**
 * @brief SIGALRM (逾時) 處理函式 (父行程用)
 * 題目要求：強制終止子行程，並設定旗標
 */
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


/* --- 3. 子行程邏輯 --- */

/**
 * @brief 子行程 (Worker) 的主函式
 * 題目要求：
 * 1. 重設 SIGINT handler
 * 2. raise(SIGSTOP) 讓自己暫停
 * 3. 收到 SIGCONT 後，執行 sleep(10) (注定逾時)
 * 4. 結束
 */
void child_main() {
    // 2. Handler (子行程)：重設 SIGINT 為預設行為 (SIG_DFL)
    struct sigaction sa_child_int;
    sa_child_int.sa_handler = SIG_DFL; // 預設行為 (終止)
    sigemptyset(&sa_child_int.sa_mask);
    sa_child_int.sa_flags = 0;
    sigaction(SIGINT, &sa_child_int, NULL);

    // 3. 子行程邏輯：印出 PID 並自我暫停
    printf("[Child] PID %d: Ready. Stopping (waiting for SIGCONT)...\n", getpid());
    // fflush 確保 printf 的緩衝區在 raise 之前被清空
    fflush(stdout); 
    
    raise(SIGSTOP); // 程式會在這裡暫停

    // --- 程式被 SIGCONT 喚醒後，會從這裡繼續 ---
    
    printf("[Child] PID %d: Resumed by SIGCONT. Working for 10 sec...\n", getpid());
    fflush(stdout);

    // 5. 執行工作 (模擬)
    // ------------------------------------
    // 挑戰 (Bonus) 1: sleep(10); // 會逾時
    // 挑戰 (Bonus) 2: sleep(2);  // 不會逾時
    // ------------------------------------
    sleep(10); // 這裡會被父行程的 SIGKILL (來自 SIGALRM) 中斷

    printf("[Child] PID %d: Work done. Exiting.\n", getpid());
    exit(0);
}


/* --- 4. 父行程主邏輯 --- */

int main() {
    struct sigaction sa_int, sa_chld, sa_alrm;
    sigset_t block_mask, old_mask;

    printf("[Parent] Manager started. PID %d.\n", getpid());

    // --- (A) 安裝 Handlers ---
    
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

    // --- (F) 安全等待 (本題核心) ---
    // 我們必須在迴圈中呼叫 sigsuspend，
    // 因為 SIGINT 也會中斷 sigsuspend，但我們不想因此停止。
    //
    // old_mask = 呼叫 (B) 之前的原始 mask (不封鎖 CHLD, ALRM, INT)
    //
    // sigsuspend(&old_mask) 的原子操作：
    // 1. 將 mask 暫時設定為 old_mask (即「解除」對 CHLD/ALRM 的封鎖)
    // 2. 睡眠，等待訊號
    // 3. (當 CHLD/ALRM/INT 任一訊號抵達)
    // 4. 執行對應的 handler (handler 會設定旗標)
    // 5. sigsuspend 將 mask「恢復」為呼叫前的狀態 (即 `block_mask`)
    // 6. sigsuspend 返回
    
    printf("[Parent] Waiting for SIGCHLD or SIGALRM... (Ctrl+C is ignored)\n");
    
    while (timeout_flag == 0 && child_reaped_flag == 0) {
        // 原子性地解除封鎖 (CHLD/ALRM) 並等待
        sigsuspend(&old_mask); 
    }

    // --- (G) 判斷結果 ---
    // 程式執行到這裡，代表 `timeout_flag` 或 `child_reaped_flag` 至少
    // 有一個為 1。
    //
    // 並且，因為 sigsuspend 已返回，目前的 process mask 已經
    // "自動" 恢復為 `block_mask` (CHLD 和 ALRM 仍被封鎖)。
    // 這為我們提供了檢查旗標的「臨界區段 (Critical Section)」。
    
    printf("[Parent] Woke up. Event occurred.\n");
    
    // 第一時間取消鬧鐘 (如果子行程提早完成，鬧鐘可能還在跑)
    alarm(0);

    // 檢查旗標 (此時 CHLD/ALRM 仍被封鎖，可安全檢查)
    if (timeout_flag) {
        printf("[Parent] Result: Child timed out. Killed.\n");
        // 我們必須手動回收被 SIGKILL 終止的子行程
        // (因為 handler_alrm 發生時，子行程可能還沒死，
        // SIGCHLD 可能在 sigsuspend 迴圈外才到)
        // 為了健壯性，在這裡再次檢查回收
        handler_chld(0); 
    } else if (child_reaped_flag) {
        printf("[Parent] Result: Child finished on time.\n");
    }

    // --- (H) 恢復 ---
    printf("[Parent] Restoring original mask and exiting.\n");
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    
    return 0;
}
