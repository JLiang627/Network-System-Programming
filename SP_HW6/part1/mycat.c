#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     
#include <fcntl.h>      // For open() and file flags

#define BUFFER_SIZE 4096 

int main(int argc, char *argv[]) {
    int fd;                          
    ssize_t bytes_read;              
    char buffer[BUFFER_SIZE];        

    if (argc == 1) {
        fd = STDIN_FILENO;
    } else if (argc == 2) {
        // 有一個參數，開啟檔案
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            perror(argv[1]);
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Usage: mycat [filename]\n");
        return EXIT_FAILURE;
    }

    // 循環讀取並寫入標準輸出
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        ssize_t bytes_written = 0;
        ssize_t total_written = 0;

        while (total_written < bytes_read) {
            bytes_written = write(STDOUT_FILENO, buffer + total_written, bytes_read - total_written);
            if (bytes_written < 0) {
                perror("write error");
                if (fd != STDIN_FILENO)
                    close(fd);
                return EXIT_FAILURE;
            }
            total_written += bytes_written;
        }
    }

    if (bytes_read < 0) {
        perror("read error");
        if (fd != STDIN_FILENO)
            close(fd);
        return EXIT_FAILURE;
    }

    if (fd != STDIN_FILENO) {
        if (close(fd) < 0) {
            perror("close error");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
