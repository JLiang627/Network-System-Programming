#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

// 宣告信號處理函式
void alarm_handler(int signo) {
    // 這裡只是簡單地印出訊息，通常處理器中會有更複雜的邏輯
    // 在信號處理器中使用非同步安全函式 write
    write(STDOUT_FILENO, "SIGALRM received! (10 seconds passed)\n", 37);
}

int main() {
    sigset_t oldmask;     // 用於儲存原始的信號遮罩
    sigset_t blockmask;   // 用於 sigsuspend()，只允許 SIGALRM
    sigset_t noracemask;  // 用於在 alarm() 期間阻塞所有信號
    
    // --- 【修正：使用 sigaction 替換 sigset】 ---
    struct sigaction sa;

    // 1. 安裝 alarm_handler 作為 SIGALRM 的處理器
    sa.sa_handler = alarm_handler;
    // 在處理器執行期間，不阻塞任何額外信號
    sigemptyset(&sa.sa_mask);
    // 設置旗標：SA_RESTART 確保被信號中斷的系統呼叫會自動重啟 (這是良好的實踐)
    sa.sa_flags = SA_RESTART; 

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    // --- 【修正結束】 ---

    // 2. 準備信號遮罩
    /* 將遮罩填滿 1's */
    sigfillset(&blockmask);   // blockmask: 阻塞所有信號
    sigfillset(&noracemask);  // noracemask: 阻塞所有信號

    /* blockmask: 阻塞所有信號，除了 SIGALRM */
    sigdelset(&blockmask, SIGALRM); 

    // 3. 進入防競態條件區塊
    /* 避免競態條件；阻塞所有信號；儲存原始遮罩 */
    // 暫時阻塞所有信號 (noracemask)，並儲存原始遮罩到 oldmask
    sigprocmask(SIG_BLOCK, &noracemask, &oldmask);

    // 4. 設定鬧鐘 (10 秒後發送 SIGALRM)
    alarm(10);

    // 5. 原子性地等待信號
    /* 暫停程序直到 SIGALRM (10 秒) 到來並呼叫 alarm_handler() */
    // 暫時將遮罩換成 blockmask (允許 SIGALRM)，然後進入睡眠
    sigsuspend(&blockmask);
    
    // sigsuspend 返回後，會自動將信號遮罩恢復到呼叫它之前的狀態 (block all)。

    // 6. 恢復原始遮罩
    // 將信號遮罩恢復到程序啟動時的狀態 (oldmask)
    sigprocmask(SIG_SETMASK, &oldmask, NULL);

    printf("Program finished successfully after alarm.\n");

    return 0;
}
