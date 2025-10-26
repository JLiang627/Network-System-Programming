#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h> // 包含 struct itimerval
#include <stdlib.h> // 包含 exit, atoi

#define REPEAT_RATE 2 /* seconds. */

static char beep = '\a'; // ASCII BEL 字符

// SIGALRM 信號處理器
void handler(int i) {
    write(STDOUT_FILENO, &beep, sizeof(beep));
}

int main(int argc, char *argv[]) {
    struct itimerval it; /* Structure for loading the timer. */
    sigset_t mask;      /* Mask for blocking signals. */
    int seconds;

    if ((argc != 2) || ((seconds = atoi(argv[1])) <= 0)) {
        fprintf(stderr, "Usage: %s <seconds>\n", argv[0]);
        exit(1);
    }

    // 設定信號遮罩 (Mask)
    // 阻塞所有信號，除了 SIGALRM 和 SIGINT (Ctrl-C)
    sigfillset(&mask);
    sigdelset(&mask, SIGALRM);
    sigdelset(&mask, SIGINT);

    struct sigaction sa;
    // 設置處理函式
	sa.sa_handler = handler;

    // 設置信號遮罩：在處理函式執行期間，不阻塞任何額外信號
	sigemptyset(&sa.sa_mask); 
	sa.sa_flags = SA_RESTART; 

    // 安裝處理器
	if (sigaction(SIGALRM, &sa, NULL) == -1) {
    		perror("sigaction");
    		exit(1);
	}
    // 設定定時器結構 (struct itimerval)
    /* 第一次超時時間 */
    it.it_value.tv_sec = seconds;
    it.it_value.tv_usec = 0;
    /* 週期性重複間隔 */
    it.it_interval.tv_sec = REPEAT_RATE;
    it.it_interval.tv_usec = 0;

    // 啟動定時器
    if (setitimer(ITIMER_REAL, &it, (struct itimerval *)NULL) == -1) {
        perror("setitimer");
        exit(1);
    }

    // 反覆等待信號
    /* Repeatedly block then call handler. */
    while (1) {
        // 程序在這裡睡眠，等待 SIGALRM 或 SIGINT 喚醒
        sigsuspend(&mask);
    }

    return 0;
}
