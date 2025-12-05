#define _GNU_SOURCE
#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L
#include <asm-generic/errno-base.h>
#include <asm-generic/errno-base.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>


#define BUF_SIZE_MAX 1024
#define CONSUMER_MAX 2048 

typedef struct{
    int seq;
    char text[80];
} message;

struct shm_layout {
    int D, R, C, B;
    int finished;
    message buffer[BUF_SIZE_MAX];
    long recv_total[CONSUMER_MAX];
};

struct shm_layout *shmp;
int cnt_total = 0;

void handler(int sig, siginfo_t *info, void *ucontext) {
    int seq = info->si_value.sival_int;
    int idx = seq % shmp->B;
    if (seq < 0) {
        return;
    }
    
    if (shmp->buffer[idx].seq == seq) {
        cnt_total++;  // 表示有收到data
    }
}

void consumer_main(int child_num){
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(1);
    }

    // 迴圈等待 signal
    while (!shmp->finished) {
        pause();   // 等待 SIGUSR1
    }

    shmp->recv_total[child_num] = cnt_total;
    exit(0);
}

void producer_main(pid_t *consumer_pids, int D, int C, int B, int R){
    int seq = 0;
    int idx;
    union sigval value;

    while (seq < D){
        idx = seq % B;
        shmp->buffer[idx].seq = seq;

        snprintf(shmp->buffer[idx].text, 80, "This is message %d", seq);
        value.sival_int = seq;

        for (int i = 0 ; i < C ; i++){
            if (sigqueue(consumer_pids[i], SIGUSR1, value) == -1) {
                // 只有在 sigqueue 失敗時才檢查 errno
                if (errno != ESRCH) {
                    perror("sigqueue error");
                }
            }
        }

        seq++;
        usleep(R * 1000);  //// 暫停 R 毫秒 (R ms = R * 1000 us)
    } 

    shmp->finished = 1;
    value.sival_int = -1;
    for(int i = 0; i < C; i++)
        sigqueue(consumer_pids[i], SIGUSR1, value);
}

int main(int argc, char *argv[]){
    // 測試參數輸入數量
    if (argc != 5 || (strcmp(argv[1], "--help") == 0)){
        fprintf(stderr, "Usage: %s [data數量 D] [傳送速率R] [consumer數量 C] [buffer size B]\n", argv[0]);
        exit(1);
    }

    // 參數轉int
    int D = atoi(argv[1]);  //data數量
    int R = atoi(argv[2]);  //傳送速率
    int C = atoi(argv[3]);  //consumer數量
    int B = atoi(argv[4]);  //buffer size

    // 變數宣告
    key_t shm_key;
    int shmid = -1;
    int cnt_C = C;
    pid_t consumer_pids[C];
    int cnt_child = 0;
 
    if ((shm_key = ftok(".", 'A')) == -1){
        perror("ftok shm");
        exit(1);
    }
    
    if ((shmid = shmget(shm_key, sizeof(struct shm_layout), IPC_CREAT | IPC_EXCL | 0666)) == -1){
        if (errno == EEXIST){
            shmid = shmget(shm_key, sizeof(struct shm_layout), 0666);
            if (shmid != -1){
                if (shmctl(shmid, IPC_RMID, NULL) == -1){
                    perror("Failed to remove old shared memory");
                    exit(1);
                }
            }
            printf("Removed stale shared memory\n");
            shmid = shmget(shm_key, sizeof(struct shm_layout), IPC_CREAT | IPC_EXCL | 0666);
        }
    
        if (shmid == -1){
            perror("shmget");
            exit(1);
        }
    
    }

    shmp = shmat(shmid, NULL, 0);
    shmp->B = B;
    shmp->D = D;
    shmp->R = R;
    shmp->C = C;
    shmp->finished = 0;
     
    memset(shmp->recv_total, 0, sizeof(long) * C);

    struct sigaction sa_ign;
    sigemptyset(&sa_ign.sa_mask);
    sa_ign.sa_flags = 0;
    sa_ign.sa_handler = SIG_IGN;
    sigaction(SIGUSR1, &sa_ign, NULL);
    // 產生 C 筆 Consumer
    while (cnt_C != 0){
        int pid = fork();
        
        if (pid < 0){
            perror("fork error");
            exit(1);
        }else if (pid == 0){
            // child process
            consumer_main(cnt_child);
        }else{
            consumer_pids[C-cnt_C] = pid;
        }
        cnt_child++;
        cnt_C--;
    }

    producer_main(consumer_pids, D, C, B, R);
    

    // 等待child結束
    for (int i = 0; i < C; i++){
        if (wait(NULL) == -1){
            perror("wait error");
        }
    }

    long sum_received = 0;
    long total_expected = (long)D * C;
    double loss_rate;

    for (int i = 0; i < C; i++) {
        sum_received += shmp->recv_total[i];
    }

    loss_rate = 1.0 - ((double)sum_received / total_expected);

    // 輸出
    printf("D=%d R=%d ms C=%d\n", D, R, C);
    printf("----------------------------\n");
    printf("Total messages: %ld (%d*%d 的意思)\n", total_expected, D, C);
    printf("Sum of received messages by all consumers: %ld\n", sum_received);
    printf("Loss rate: 1 - (%ld/%ld) = %f\n", sum_received ,total_expected, loss_rate);
    printf("----------------------------\n");

    if (shmdt(shmp) == -1) {
        perror("shmdt failed");
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID failed");
    }
    return 0;
}