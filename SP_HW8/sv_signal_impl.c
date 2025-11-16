//Implement the System V functions sigset(), sighold(), sigrelse(), sigignore(), andsigpause() using the POSIX signal API.
#define _GNU_SOURCE
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "tlpi_hdr.h"


//用於通知main函式
static volatile sig_atomic_t sigCaught = 0;

typedef void (*sighandler_t)(int);

static void handler(int signo)
{
    sigCaught = 1;
}

//函式宣告
sighandler_t sigset(int sig, sighandler_t handler);
int sighold(int sig);
int sigrelse(int sig);
int sigignore(int sig); 
int sigpause(int sig);

int main(int argc, char *argv[])
{
    // --- 測試 1: sigignore() ---
    printf("測試 1: sigignore(SIGINT)\n");
    if (sigignore(SIGINT) == -1)
        errExit("sigignore");
    
    if (raise(SIGINT) == -1) // 發送 SIGINT
        errExit("raise 1");
    
    printf("成功 (程式未因 SIGINT 終止)。\n\n");

    // --- 測試 2: sigset() (設定 Handler) ---
    printf("測試 2: sigset(SIGUSR1, handler)\n");
    sigCaught = 0; // 重設旗標
    if (sigset(SIGUSR1, handler) == SIG_ERR) 
        errExit("sigset 1");

    if (raise(SIGUSR1) == -1) // 發送 SIGUSR1
        errExit("raise 2");

    if (sigCaught == 1)
        printf("成功 (sigset 正確設定 handler 並解除阻擋)。\n\n");
    else
        printf("失敗 (handler 未被呼叫)。\n\n");

    // --- 測試 3: sighold() 和 sigrelse() ---
    printf("測試 3: sighold(SIGUSR1) + sigrelse(SIGUSR1)\n");
    sigCaught = 0; // 重設旗標

    if (sighold(SIGUSR1) == -1) // 阻擋 SIGUSR1
        errExit("sighold");
    
    printf("(已呼叫 sighold 訊號被阻擋)\n");
    if (raise(SIGUSR1) == -1) // 再次發送
        errExit("raise 3");

    if (sigCaught == 0)
        printf("成功 (sighold 成功阻擋訊號)。\n");
    else
        printf("失敗 (sighold 未能阻擋訊號)。\n");

    if (sigrelse(SIGUSR1) == -1) // 解除阻擋 (此時 Pending 的訊號應被遞送)
        errExit("sigrelse");
    
    if (sigCaught == 1)
        printf("成功 (sigrelse 成功解除阻擋並遞送訊號)。\n\n");
    else
        printf("失敗 (sigrelse 未觸發訊號遞送)。\n\n");

    // --- 測試 4: sigset(SIG_HOLD) ---
    printf("測試 4: sigset(SIGUSR1, SIG_HOLD)\n");
    sigCaught = 0; // 重設旗標
    
    if (sigset(SIGUSR1, SIG_HOLD) == SIG_ERR) // 應阻擋訊號 
        errExit("sigset 2");

    if (raise(SIGUSR1) == -1)
        errExit("raise 4");

    if (sigCaught == 0)
        printf("成功 (sigset(SIG_HOLD) 成功阻擋訊號)。\n\n");
    else
        printf("失敗 (sigset(SIG_HOLD) 未能阻擋訊號)。\n\n");

        return 0;
}

sighandler_t sigset(int sig, sighandler_t handler)
{   
    struct sigaction sa, old_sa;    
    if ((sigaction(sig ,NULL, &old_sa)) == -1)
        return SIG_ERR;    
    
    if (handler == SIG_HOLD){
        if (sighold(sig) == -1)
            return SIG_ERR;    
    }
    else{
        sa.sa_handler = handler;
        sa.sa_flags = SA_RESTART;
        sigemptyset(&sa.sa_mask);

        if (sigaction(sig, &sa, NULL) == -1)
            return SIG_ERR;
        if (sigrelse(sig) == -1) 
            return SIG_ERR;    
    }
        

    return old_sa.sa_handler;
}


int sighold(int sig)
{
    sigset_t blockmask;
    sigemptyset(&blockmask);
    sigaddset(&blockmask, sig);
    
    return sigprocmask(SIG_BLOCK, &blockmask, NULL);
}

int sigrelse(int sig)
{
    sigset_t unblockmask;
    sigemptyset(&unblockmask);
    sigaddset(&unblockmask, sig);

    return sigprocmask(SIG_UNBLOCK, &unblockmask, NULL);
}

int sigignore(int sig)
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask); 
    sa.sa_flags = 0;
    sa.sa_handler=SIG_IGN;

    return sigaction(sig, &sa, NULL);
}


int sigpause(int sig)
{
    sigset_t currmask;
    if (sigprocmask(SIG_SETMASK, NULL, &currmask) == -1)
        return -1;
    sigdelset(&currmask, sig);

    return sigsuspend(&currmask);

}