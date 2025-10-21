#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     
#include <sys/wait.h>  

#define READ_END 0       
#define WRITE_END 1      
#define BUFFER_SIZE 4096 
#define STDOUT_FILENO 1 

int main(void) {
    int pipefds[2];     // pipefds[0] = 讀取端, pipefds[1] = 寫入端
    pid_t pid;
    int status;

    // 建立 Pipe
    if (pipe(pipefds) == -1) {
        perror("pipe creation failed");
        return EXIT_FAILURE;
    }

    // fork child
    switch (pid = fork()) {
        case -1: 
            perror("fork failed");
            close(pipefds[READ_END]);
            close(pipefds[WRITE_END]);
            return EXIT_FAILURE;

        case 0: 
            if (close(pipefds[READ_END]) == -1) {
                perror("child close read end failed");
                exit(EXIT_FAILURE);
            }

            // 將 Pipe 的寫入端dup到標準輸出 1
            if (dup2(pipefds[WRITE_END], STDOUT_FILENO) == -1) {
                perror("dup2 failed");
                exit(EXIT_FAILURE);
            }

            // 關閉原來的寫入端，因為 STDOUT_FILENO 現在指向它了
            if (close(pipefds[WRITE_END]) == -1) {
                perror("child close write end failed");
                exit(EXIT_FAILURE);
            }
            
            // 執行 'ls -l' 命令
            execlp("ls", "ls", "-l", (char *)NULL);
            
            // 如果 exec 失敗，則執行到這裡
            perror("execlp failed");
            exit(EXIT_FAILURE);

        default: // parent (讀取 'ls -l' 的輸出)
            {
            char buffer[BUFFER_SIZE];
            ssize_t bytes_read;

            // parent不需要寫入，關閉 Pipe 的寫入端
            if (close(pipefds[WRITE_END]) == -1) {
                perror("parent close write end failed");
                return EXIT_FAILURE;
            }

            // 循環從 Pipe 讀取數據，直到讀完
            while ((bytes_read = read(pipefds[READ_END], buffer, BUFFER_SIZE)) > 0) {
                // 將讀取到的數據寫入父進程的標準輸出
                if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
                    perror("write to stdout failed");
                    close(pipefds[READ_END]);
                    return EXIT_FAILURE;
                }
            }
            
            // 檢查 read 是否出錯
            if (bytes_read < 0) {
                perror("read from pipe failed");
            }
            
            // 關閉 Pipe 的讀取端
            if (close(pipefds[READ_END]) == -1) {
                perror("parent close read end failed");
                return EXIT_FAILURE;
            }

            // 等待child process結束
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid failed");
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
            }
    }
}
