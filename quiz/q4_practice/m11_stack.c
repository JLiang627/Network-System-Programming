#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STACK_SIZE 3  // 設定很小以容易觸發 "Stack Full"

char buffer[STACK_SIZE];
int count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

/* Push: 放入資料 (生產者) */
void push(char c) {
    pthread_mutex_lock(&mutex);

    // 重點 1: 使用 while 而非 if，防止 Spurious Wakeup (假喚醒)
    while (count == STACK_SIZE) {
        printf("Stack FULL. Thread waiting to push '%c'...\n", c);
        // 重點 2: wait 會自動釋放 mutex 並進入睡眠；醒來時會重新取得 mutex
        pthread_cond_wait(&cond_var, &mutex);
    }

    buffer[count] = c;
    count++;
    printf("PUSH: '%c' (Count: %d)\n", c, count);

    // 重點 3: 喚醒可能正在等待 "Stack 非空" 的 Pop 執行緒
    pthread_cond_signal(&cond_var);
    
    pthread_mutex_unlock(&mutex);
}

/* Pop: 取出資料 (消費者) */
char pop() {
    pthread_mutex_lock(&mutex);

    while (count == 0) {
        printf("Stack EMPTY. Thread waiting to pop...\n");
        pthread_cond_wait(&cond_var, &mutex);
    }

    count--;
    char c = buffer[count];
    printf("POP:  '%c' (Count: %d)\n", c, count);

    // 喚醒可能正在等待 "Stack 非滿" 的 Push 執行緒
    pthread_cond_signal(&cond_var);

    pthread_mutex_unlock(&mutex);
    return c;
}

void *producer(void *arg) {
    for (char c = 'A'; c <= 'E'; c++) {
        push(c);
        sleep(1); // 模擬工作時間
    }
    return NULL;
}

void *consumer(void *arg) {
    for (int i = 0; i < 5; i++) {
        sleep(2); // 讓消費者慢一點，容易觸發 Stack Full
        pop();
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, producer, NULL);
    pthread_create(&t2, NULL, consumer, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}