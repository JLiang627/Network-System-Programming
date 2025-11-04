#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For usleep()
#include <time.h>   // For srand()
#include <string.h> // For strcmp()
#include <time.h>       // time, srand, rand
#include <sys/time.h>   // setitimer, struct itimerval

// 共享緩衝區
typedef struct {
    char data;                    // 共享數據 (1-9 或 '#')
    int num_consumers;            // K 的總數
    int consumers_finished_round; // 計數器：多少個 Consumer 已完成本輪讀取
    
    pthread_mutex_t lock;
    pthread_cond_t cond_producer; // Producer 等待的條件變數
    pthread_cond_t cond_consumer; // Consumer 等待的條件變數
} SharedBuffer;

SharedBuffer buffer;

void *consumer_func(void *arg) {
    int id = *(int*)arg;
    int sum = 0;

    // 鎖定 Mutex 進入while
    pthread_mutex_lock(&buffer.lock);

    while (1) {
        // 等待 Producer 釋放lock
        pthread_cond_wait(&buffer.cond_consumer, &buffer.lock);

        // 檢查是否為終止訊號
        if (buffer.data == '#') {
            break; // 跳出迴圈，準備終止
        }

        // 是數字，加到私有的 sum 中
        int value = buffer.data - '0'; // 將 '1' 轉為 1
        sum += value;

        // 回報 Producer：thread已完成本輪
        buffer.consumers_finished_round++;

        // 如果是「最後一個」完成的 Consumer，
        // 則由它負責喚醒 Producer
        if (buffer.consumers_finished_round == buffer.num_consumers) {
            pthread_cond_signal(&buffer.cond_producer);
        }
    }

    // 收到終止訊號，解鎖並印出總和
    pthread_mutex_unlock(&buffer.lock);
    printf("Consumer %d: %d\n", id, sum);
    
    return NULL;
}

int is_valid_int(char *str) {
    if (str == NULL || *str == '\0') return 0;
    for (; *str; str++) {
        if (*str < '0' || *str > '9') return 0;
    }
    return 1;
}

int is_valid_float(char *str) {
    if (str == NULL || *str == '\0') return 0;
    int dot_count = 0;
    for (int i = 0; str[i]; i++) {
        if (str[i] == '.') {
            dot_count++;
        } else if (str[i] < '0' || str[i] > '9') {
            return 0;
        }
        if (dot_count > 1) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {  // 同時擔任producer
    
    // 檢查輸入參數
    if (argc != 3) {
        fprintf(stderr, "Usage: %s num_consumer interval\n", argv[0]);
        return 1;
    }

    // 檢查參數正確性
    if (!is_valid_int(argv[1]) || !is_valid_float(argv[2])) {
        fprintf(stderr, "Error: Invalid arguments.\n");
        fprintf(stderr, "Usage: %s num_consumer interval\n", argv[0]);
        return 1;
    }

    int k_consumers = atoi(argv[1]);
    double t_interval = atof(argv[2]);

    if (k_consumers <= 0 || k_consumers > 50) { // K最多可以接受到50個
        fprintf(stderr, "Error: num_consumer must be between 1 and 50.\n");
        return 1;
    }
    if (t_interval <= 0) {
        fprintf(stderr, "Error: interval must be positive.\n");
        return 1;
    }
    
    // 將秒轉換為微秒
    useconds_t sleep_usec = (useconds_t)(t_interval * 1000000);

    // 初始化共享緩衝區、Mutex 和條件變數
    buffer.num_consumers = k_consumers;
    buffer.consumers_finished_round = 0;
    pthread_mutex_init(&buffer.lock, NULL);
    pthread_cond_init(&buffer.cond_producer, NULL);
    pthread_cond_init(&buffer.cond_consumer, NULL);

    // 建立 K 個 Consumer 執行緒
    pthread_t *tids = malloc(k_consumers * sizeof(pthread_t));
    int *consumer_ids = malloc(k_consumers * sizeof(int));

    for (int i = 0; i < k_consumers; i++) {
        consumer_ids[i] = i + 1; // ID 從 1 開始
        if (pthread_create(&tids[i], NULL, consumer_func, &consumer_ids[i]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }

    // 老師提供的seed hint
    srand(time(NULL)); // 設置隨機數種子
    // printf("Producer starting... (K=%d, t=%.2fs)\n", k_consumers, t_interval); // for測試
    
    struct itimerval timer;
    long sec = (long)t_interval;
    long usec = (long)((t_interval) * 1000000.0);
    timer.it_value.tv_sec = sec;
    timer.it_value.tv_usec = usec;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        exit(1);
    }

    // 產生 5 個隨機數
    for (int i = 0; i < 5; i++) {
        // 等待 t 秒
        usleep(timer.it_value.tv_sec);

        // --- 進入critical section ---
        pthread_mutex_lock(&buffer.lock);

        // 產生 1-9 的隨機數
        int random_num = (rand() % 9) + 1;
        buffer.data = (char)(random_num + '0'); // 轉為 '1'-'9'
        // printf("Producer generated: %c\n", buffer.data); //for 測試

        // 重設計數器並喚醒所有 Consumer
        buffer.consumers_finished_round = 0;
        pthread_cond_broadcast(&buffer.cond_consumer);

        // lock在這裡才釋放
        while (buffer.consumers_finished_round < buffer.num_consumers) {
            pthread_cond_wait(&buffer.cond_producer, &buffer.lock);
        }
        
        // --- 離開critical section ---
        pthread_mutex_unlock(&buffer.lock);
    }
    
    pthread_mutex_lock(&buffer.lock);
    buffer.data = '#'; // 
    // printf("Producer generated: %c (Termination)\n", buffer.data); //for 測試
    
    // 喚醒所有 Consumer 讓它們terminate
    pthread_cond_broadcast(&buffer.cond_consumer);
    pthread_mutex_unlock(&buffer.lock);


    // 等待所有 Consumer thread結束
    for (int i = 0; i < k_consumers; i++) {
        pthread_join(tids[i], NULL);
    }

    // 清理資源
    pthread_mutex_destroy(&buffer.lock);
    pthread_cond_destroy(&buffer.cond_producer);
    pthread_cond_destroy(&buffer.cond_consumer);
    free(tids);
    free(consumer_ids);

    printf("All consumers finished.\n");
    return 0;
}