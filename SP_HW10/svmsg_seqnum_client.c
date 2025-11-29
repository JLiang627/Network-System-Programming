#include "error_functions.h"
#include "get_num.h"
#include "svmsg_seqnum_header.h"
#include <sys/types.h>
#include <unistd.h>


int
main(int argc, char *argv[])
{
    struct requestMsg req;
    struct responseMsg resp;
    int serverId, numMsgs;
    ssize_t msgLen;
    key_t SERVER_KEY;
    int seqLen; 

    if (argc != 2 || strcmp(argv[1], "--help") == 0)
        usageErr("%s [seq-len]\n", argv[0]);

    seqLen = (argc>1) ? getInt(argv[1], 0 , "seq_len") : 1;
    /* Get server's queue identifier; create queue for response */
    SERVER_KEY = ftok(SERVER_KEY_PATH, SERVER_KEY_ID);
    if (SERVER_KEY == -1) errExit("ftok");
    
    serverId = msgget(SERVER_KEY, S_IWUSR);
    if (serverId == -1)
        errExit("msgget - server message queue");

    /* Send message asking for file named in argv[1] */
    req.mtype = REQ_MTYPE;                      /* Any type will do */
    req.pid = getpid();
    req.seqLen = seqLen;

    if (msgsnd(serverId, &req, REQ_MSG_SIZE, 0) == -1)
        errExit("msgsnd");

    /* Get first response, which may be failure notification */
    msgLen = msgrcv(serverId, &resp, RESP_MSG_SIZE, (long)getpid(), 0);
    if (msgLen == -1)
        errExit("msgrcv");

    printf("Allocated sequence starts at: %d\n", resp.seqNum);    
    exit(EXIT_SUCCESS);
}
