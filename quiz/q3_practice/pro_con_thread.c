#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For usleep()
#include <time.h>   // For srand()
#include <string.h> // For strcmp()

// 共享緩衝區和同步原語
typedef struct {
    char data;                    // 共享數據 (1-9 或 '#')
    int num_consumers;            // K 的總數
    int consumers_finished_round; // 計數器：多少個 Consumer 已完成本輪讀取
    
    pthread_mutex_t lock;
    pthread_cond_t cond_producer; // Producer 等待的條件變數
    pthread_cond_t cond_consumer; // Consumer 等待的條件變數
} SharedBuffer;

SharedBuffer buffer;

/**
 * @brief Consumer 執行緒函式
 */
void *consumer_func(void *arg) {
    int id = *(int*)arg;
    int sum = 0;

    // 鎖定 Mutex 進入主迴圈
    pthread_mutex_lock(&buffer.lock);

    while (1) {
        // 1. 等待 Producer 發出新數據的廣播
        pthread_cond_wait(&buffer.cond_consumer, &buffer.lock);

        // 2. 被喚醒，檢查是否為終止訊號
        if (buffer.data == '#') {
            break; // 跳出迴圈，準備終止
        }

        // 3. 是數字，加到私有的 sum 中
        int value = buffer.data - '0'; // 將 '1' 轉為 1
        sum += value;

        // 4. 回報 Producer：本執行緒已完成本輪
        buffer.consumers_finished_round++;

        // 5. 如果是「最後一個」完成的 Consumer，
        //    則由它負責喚醒 Producer
        if (buffer.consumers_finished_round == buffer.num_consumers) {
            pthread_cond_signal(&buffer.cond_producer);
        }
    }

    // 6. 收到終止訊號，解鎖並印出總和
    pthread_mutex_unlock(&buffer.lock);
    printf("Consumer %d: %d\n", id, sum);
    
    return NULL;
}

/**
 * @brief 檢查輸入是否為有效的正整數
 */
int is_valid_int(char *str) {
    if (str == NULL || *str == '\0') return 0;
    for (; *str; str++) {
        if (*str < '0' || *str > '9') return 0;
    }
    return 1;
}

/**
 * @brief 檢查輸入是否為有效的浮點數
 */
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


/**
 * @brief Main 執行緒 (同時扮演 Producer)
 */
int main(int argc, char *argv[]) {
    // 1. 檢查並解析命令列參數 [cite: 193, 207]
    if (argc != 3) {
        fprintf(stderr, "Usage: %s num_consumer interval\n", argv[0]);
        return 1;
    }

    if (!is_valid_int(argv[1]) || !is_valid_float(argv[2])) {
        fprintf(stderr, "Error: Invalid arguments.\n");
        fprintf(stderr, "Usage: %s num_consumer interval\n", argv[0]);
        return 1;
    }

    int k_consumers = atoi(argv[1]);
    double t_interval = atof(argv[2]);

    if (k_consumers <= 0 || k_consumers > 50) { // K=50 [cite: 190]
        fprintf(stderr, "Error: num_consumer must be between 1 and 50.\n");
        return 1;
    }
    if (t_interval <= 0) {
        fprintf(stderr, "Error: interval must be positive.\n");
        return 1;
    }
    
    // 將秒轉換為微秒 (useconds_t)
    useconds_t sleep_usec = (useconds_t)(t_interval * 1000000);

    // 2. 初始化共享緩衝區、Mutex 和條件變數
    buffer.num_consumers = k_consumers;
    buffer.consumers_finished_round = 0;
    pthread_mutex_init(&buffer.lock, NULL);
    pthread_cond_init(&buffer.cond_producer, NULL);
    pthread_cond_init(&buffer.cond_consumer, NULL);

    // 3. 建立 K 個 Consumer 執行緒
    pthread_t *tids = malloc(k_consumers * sizeof(pthread_t));
    int *consumer_ids = malloc(k_consumers * sizeof(int));

    for (int i = 0; i < k_consumers; i++) {
        consumer_ids[i] = i + 1; // ID 從 1 開始
        if (pthread_create(&tids[i], NULL, consumer_func, &consumer_ids[i]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }

    // 4. Main 執行緒扮演 Producer
    srand(time(NULL)); // 設置隨機數種子 [cite: 205]
    printf("Producer starting... (K=%d, t=%.2fs)\n", k_consumers, t_interval);

    // 讓我們產生 5 個隨機數
    for (int i = 0; i < 5; i++) {
        // 等待 t 秒
        usleep(sleep_usec);

        // --- 進入臨界區 ---
        pthread_mutex_lock(&buffer.lock);

        // a. 產生 1-9 的隨機數 [cite: 191, 206]
        int random_num = (rand() % 9) + 1;
        buffer.data = (char)(random_num + '0'); // 轉為 '1'-'9'
        printf("Producer generated: %c\n", buffer.data);

        // b. 重設計數器並廣播 (喚醒) 所有 Consumer
        buffer.consumers_finished_round = 0;
        pthread_cond_broadcast(&buffer.cond_consumer);

        // c. 等待，直到所有 Consumer 都回報
        while (buffer.consumers_finished_round < buffer.num_consumers) {
            pthread_cond_wait(&buffer.cond_producer, &buffer.lock);
        }
        
        // --- 離開臨界區 ---
        pthread_mutex_unlock(&buffer.lock);
    }

    // 5. 傳送終止訊號
    usleep(sleep_usec); // 模擬最後一次延遲
    
    pthread_mutex_lock(&buffer.lock);
    buffer.data = '#'; // 
    printf("Producer generated: %c (Termination)\n", buffer.data);
    
    // 廣播，喚醒所有 Consumer 讓它們終止
    pthread_cond_broadcast(&buffer.cond_consumer);
    pthread_mutex_unlock(&buffer.lock);


    // 6. 等待 (Join) 所有 Consumer 執行緒結束
    for (int i = 0; i < k_consumers; i++) {
        pthread_join(tids[i], NULL);
    }

    // 7. 清理資源
    pthread_mutex_destroy(&buffer.lock);
    pthread_cond_destroy(&buffer.cond_producer);
    pthread_cond_destroy(&buffer.cond_consumer);
    free(tids);
    free(consumer_ids);

    printf("All consumers finished. Producer exiting.\n");
    return 0;
}