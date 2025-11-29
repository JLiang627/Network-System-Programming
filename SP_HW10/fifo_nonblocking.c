#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

#define FIFO_PATH "/tmp/test_fifo_nonblocking"
#define TEST_BUF_SIZE 4096
#define FILL_CHAR 'X'

static void remove_fifo(void) {
    if (unlink(FIFO_PATH) == -1 && errno != ENOENT) {
        perror("unlink");
    }
}

int main() {
    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return 1;
    }
    atexit(remove_fifo);
    
    pid_t pid;
    int status;

    pid = fork();
    if(pid < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }else if(pid == 0){
        printf("[Child %ld] 啟動，準備輔助測試。\n", (long)getpid());

        int fd_w_test = open(FIFO_PATH, O_WRONLY); 
        if (fd_w_test == -1) { perror("Child open O_WRONLY"); exit(EXIT_FAILURE); }
        printf("[Child] 1. 為 READ 測試開啟寫入端。\n");
        
        sleep(2);
        
        if (write(fd_w_test, "OK", 2) == 2) {
             printf("[Child] 已寫入 'OK' (解除 Parent READ 阻塞)。\n");
        }
        close(fd_w_test);

        int fd_r_test = open(FIFO_PATH, O_RDONLY);
        if (fd_r_test == -1) { perror("Child open O_RDONLY for WRITE test"); exit(EXIT_FAILURE); }
        printf("[Child] 2. 為 WRITE 測試開啟讀取端 (保持存活)。\n");
        
        sleep(10);
        
        close(fd_r_test);
        exit(EXIT_SUCCESS); 
   }else{
        printf("[Parent ] 這裡是 parent %ld process 執行.\n", (long)getpid());

        ssize_t s;
        char buf[TEST_BUF_SIZE];
        
        memset(buf, FILL_CHAR, TEST_BUF_SIZE);

        int fd_r = open(FIFO_PATH, O_RDONLY);
        if (fd_r == -1) { perror("Parent open O_RDONLY"); exit(EXIT_FAILURE); }

        int flags = fcntl(fd_r, F_GETFL);
        if (fcntl(fd_r, F_SETFL, flags | O_NONBLOCK) == -1) { perror("fcntl set O_NONBLOCK"); exit(EXIT_FAILURE); } 

        sleep(1);

        printf("[Parent] 1. 測試 nonblocking read(FIFO 空)... 預期EAGAIN\n");
        s = read(fd_r, buf, 1);

        if (s == -1 && errno == EAGAIN) {
            printf("[PASS] READ 失敗，返回預期的 EAGAIN。\n");
        } else {
            printf("[FAIL] READ 測試失敗。實際返回 %zd, errno %d (%s)。\n", s, errno, strerror(errno));
        }
        
        sleep(3);

        printf("\n[Parent] 2. 測試 nonblocking READ (FIFO 有數據)... 預期成功\n");
        s = read(fd_r, buf, TEST_BUF_SIZE); 

        if (s > 0) {
            printf("[PASS] READ 成功讀取 %zd character (數據起始: %c)。\n", s, buf[0]);
        } else {
            printf("[FAIL] READ 測試失敗。實際返回 %zd, errno %d (%s)。\n", s, errno, strerror(errno));
        }
        
        close(fd_r);
        
        int fd_w;
        
        fd_w = open(FIFO_PATH, O_WRONLY);
        if (fd_w == -1) { perror("Parent open O_WRONLY for WRITE test"); exit(EXIT_FAILURE); }
        
        int flags_w = fcntl(fd_w, F_GETFL);
        if (fcntl(fd_w, F_SETFL, flags_w | O_NONBLOCK) == -1) { perror("fcntl set O_NONBLOCK"); exit(EXIT_FAILURE); }
        
        sleep(1);

        ssize_t s_write;
        int filled = 0;
        printf("\n[Parent] 3.1 開始填充 FIFO (非阻塞模式)... 預期返回 EAGAIN\n");

        while (1) {
            s_write = write(fd_w, buf, TEST_BUF_SIZE);
            if (s_write == -1 && errno == EAGAIN) {
                printf("[PASS] FIFO 滿載, write() 返回預期的 EAGAIN。\n");
                filled = 1;
                break;
            }
            if (s_write == -1) {
                perror("填充 FIFO 時發生非 EAGAIN 錯誤");
                break;
            }
        }
        
        if (filled) {
            printf("3.2 測試非阻塞 WRITE (已滿載, 1 字節)... 預期 EAGAIN\n");
            s_write = write(fd_w, buf, 1); 

            if (s_write == -1 && errno == EAGAIN) {
                printf("[PASS] write(1) 失敗，返回預期的 EAGAIN。\n");
            } else {
                printf("[FAIL] write(1) 測試失敗。實際返回 %zd, errno %d (%s)。\n", s_write, errno, strerror(errno));
            }
        }

        close(fd_w);
        
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }
        printf("\n[Parent] 測試完成。\n");
    } 

    return 0;
}