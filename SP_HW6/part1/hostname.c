#include <stdio.h>
#include <unistd.h>     
#include <sys/utsname.h> 

#define HOSTNAME_MAX 256

int main(void) {
    char hostname[HOSTNAME_MAX];
    struct utsname os_info;
    long hostid_val;

    if (gethostname(hostname, sizeof(hostname)) == 0) 
        printf("hostname: %s\n", hostname);
    else 
        perror("gethostname failed");
    
    if (uname(&os_info) == 0) 
        printf("%s\n", os_info.release);
    else 
        perror("uname failed");
    
    hostid_val = gethostid();
    printf("hostid: %ld\n", hostid_val);
    
    return 0;
}