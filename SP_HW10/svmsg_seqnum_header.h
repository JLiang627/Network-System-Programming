#ifndef SVMSG_SEQNUM_H
#define SVMSG_SEQNUM_H

#include <sys/types.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <stddef.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "tlpi_hdr.h"

#define SERVER_KEY_PATH "/tmp/svmsg_seqnum_key"
#define SERVER_KEY_ID   100

#define REQ_MTYPE 1L

struct requestMsg {
    long mtype;
    pid_t pid;
    int seqLen;
};

#define REQ_MSG_SIZE (sizeof(pid_t) + sizeof(int))

struct responseMsg {
    long mtype;
    int seqNum;
};

#define RESP_MSG_SIZE (sizeof(int))
#endif