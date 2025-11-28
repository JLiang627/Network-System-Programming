// leveraging Shared Memory (System V) for data transfer and Semaphores (System V) for synchronization.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define M_MAX 1024
#define MUTEX_SEM 0
#define FULL_SEM 1

typedef struct{
    pid_t PID;
    int value; 
} Pid_Val;

typedef struct{
    Pid_Val data[M_MAX];
    int index;
} SharedStack;

// 手動定義 union semun
union semun {
    int val;                  /* Value for SETVAL */
    struct semid_ds *buf;     /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array;    /* Array for GETALL, SETALL */
    struct seminfo *__buf;    /* Buffer for IPC_INFO (Linux specific) */
};

void consumer_process(SharedStack *stack, int semid, int limit){
    int processed_count = 0;
    long long total_sum = 0;
    struct sembuf p_full  = {FULL_SEM, -1, 0};  // P(Full)
    struct sembuf p_mutex = {MUTEX_SEM, -1, 0}; // P(Mutex)
    struct sembuf v_mutex = {MUTEX_SEM, 1, 0};  // V(Mutex)
    printf("Consumer (PID %d) waits on sembuf (p_full)\n", getpid());

    while (processed_count < limit) {
        // 等待 Full Count (是否有資料?)
        if (semop(semid, &p_full, 1) == -1) {
            perror("Consumer P(Full) failed");
            exit(1);
        }

        // 獲取 Mutex (保護 index)
        if (semop(semid, &p_mutex, 1) == -1) {
            perror("Consumer P(Mutex) failed");
            exit(1);
        }

        stack->index--; // Stack Pop
        int val = stack->data[stack->index].value;

        // 釋放 Mutex
        if (semop(semid, &v_mutex, 1) == -1) {
            perror("Consumer V(Mutex) failed");
            exit(1);
        }
        total_sum += val;
        processed_count++;
        printf("Consumer reads (%d, %d). Sum = %lld.\n", stack->data[stack->index].PID, stack->data[stack->index].value, total_sum);

    }

    printf("Total Data Points Processed: %d\n", processed_count);
    printf("Final Cumulative Sum: %lld\n", total_sum);
}

void producer_process(SharedStack *stack, int semid, int N, int cnt){
    srand(time(NULL) ^ getpid());
    int random_num;
    printf("Producer P%d (PID %d) starts writing...\n", cnt+1, getpid());
    struct sembuf p_mutex = {MUTEX_SEM, -1, 0}; // P(Mutex)
    struct sembuf v_mutex = {MUTEX_SEM, 1, 0};  // V(Mutex)
    struct sembuf v_full  = {FULL_SEM, 1, 0};   // V(Full)

    for (int i = 0; i < N; i++) {
        random_num = rand() % 10 + 1;

        // 獲取 Mutex
        if (semop(semid, &p_mutex, 1) == -1) {
            perror("Producer P(Mutex) failed");
            exit(1);
        }

        // Stack Push
        stack->data[stack->index].PID = getpid();
        stack->data[stack->index].value = random_num;
        stack->index++; 
        // ------------------------

        // 釋放 Mutex
        if (semop(semid, &v_mutex, 1) == -1) {
            perror("Producer V(Mutex) failed");
            exit(1);
        }

        // 增加 Full Count (通知 Consumer)
        if (semop(semid, &v_full, 1) == -1) {
            perror("Producer V(Full) failed");
            exit(1);
        }

        printf("P%d writes (%d, %d), increments Sem 1.\n", cnt+1, getpid(), random_num);
    }
}

int main(int argc, char *argv[])
{
    // 錯誤檢查
    if (argc != 3 || strcmp(argv[1],"--help") == 0){
        fprintf(stderr, "Usage: %s M(Producers) N(Number of data points)\n", argv[0]);
        exit(1);
    }

    // 變數宣告
    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    int limit = M * N;

    if (limit > M_MAX) {
        fprintf(stderr, "Error: Total data points (%d) exceed buffer size (%d)\n", limit, M_MAX);
        exit(1);
    }

    key_t shm_key, sem_key;
    SharedStack *stack = (void *)-1;
    int shmid = -1, semid = -1;

    // 產生 Key
    if ((shm_key = ftok(".", 'A')) == -1){
        perror("ftok shm");
        exit(1);
    }

    if ((sem_key = ftok(".", 'B')) == -1){
        perror("ftok sem");
        exit(1);
    }

    // 建立共享記憶體
    if ((shmid = shmget(shm_key, sizeof(SharedStack), IPC_CREAT | IPC_EXCL | 0666)) == -1){
        if (errno == EEXIST) {
            // 如果已存在，嘗試獲取舊的並刪除
            shmid = shmget(shm_key, sizeof(SharedStack), 0666);
            if (shmid != -1) {
                // 刪除舊的
                if (shmctl(shmid, IPC_RMID, NULL) == -1) {
                    perror("Failed to remove old shared memory");
                    exit(1);
                }
                printf("[Info] Removed stale shared memory segment.\n");
                // 重新嘗試建立
                shmid = shmget(shm_key, sizeof(SharedStack), IPC_CREAT | IPC_EXCL | 0666);
            }
        }
        
        // 如果重試後還是失敗
        if (shmid == -1) {
            perror("shmget");
            exit(1);
        }
    }

    // 建立號誌集合
    if ((semid = semget(sem_key, 2, IPC_CREAT | IPC_EXCL | 0666)) == -1) { 
        if (errno == EEXIST) {
            // 如果已存在，獲取舊的並刪除
            semid = semget(sem_key, 2, 0666);
            if (semid != -1) {
                if (semctl(semid, 0, IPC_RMID) == -1) {
                    perror("Failed to remove old semaphore set");
                    shmctl(shmid, IPC_RMID, NULL); 
                    exit(1);
                }
                printf("[Info] Removed stale semaphore set.\n");
                semid = semget(sem_key, 2, IPC_CREAT | IPC_EXCL | 0666);
            }
        }

        // 如果重試後還是失敗
        if (semid == -1) { 
            perror("semget"); 
            shmctl(shmid, IPC_RMID, NULL); 
            exit(1); 
        }
    }

    // 初始化號誌
    union semun arg;
    
    // Sem 0: Mutex -> 初始值 1
    arg.val = 1;
    if (semctl(semid, MUTEX_SEM, SETVAL, arg) == -1) {
        perror("semctl SETVAL Mutex");
        goto cleanup;
    }

    // Sem 1: Full Count -> 初始值 0
    arg.val = 0;
    if (semctl(semid, FULL_SEM, SETVAL, arg) == -1) {
        perror("semctl SETVAL Full");
        goto cleanup;
    }

    // 附加共享記憶體
    if ((stack = (SharedStack *)shmat(shmid, NULL, 0)) == (void *)-1){
        perror("shmat");
        goto cleanup;
    }

    stack->index = 0; // 初始化 Stack 索引
    
    // 創建 Consumer
    pid_t pid;
    if ((pid = fork()) < 0) {
        perror("fork consumer");
        goto cleanup;
    } else if (pid == 0) {
        // Child: Consumer
        consumer_process(stack, semid, limit);
        shmdt(stack);
        exit(0);
    }

    // 創建 M 個 Producers
    for (int i = 0 ; i < M ; i++){
        pid = fork();    
        if (pid < 0){
            perror("fork producer");
            goto cleanup;
        } else if (pid == 0){
            // Child: Producer
            producer_process(stack, semid, N, i);
            shmdt(stack);
            exit(0);
        }
    }

    // 父程序等待所有子程序結束
    for (int i = 0; i < M + 1; i++) {
        wait(NULL);
    }

    // 清理資源 (正常結束)
    shmdt(stack);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    return 0;

// 錯誤處理標籤
cleanup:
    if (stack != (void *)-1) shmdt(stack);
    if (shmid != -1) shmctl(shmid, IPC_RMID, NULL);
    if (semid != -1) semctl(semid, 0, IPC_RMID);
    exit(1);
}