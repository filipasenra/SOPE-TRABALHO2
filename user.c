// User Program

#include "userMessage.h"

int main(int argc, char *argv[])
{
    tlv_request_t user_request;
    pthread_t t;

    // CREATE FIFO REPLY
    char *fifo_reply = malloc(sizeof(USER_FIFO_PATH_PREFIX) + sizeof(pid_t));
    sprintf(fifo_reply, "%s%d", USER_FIFO_PATH_PREFIX, getpid());

    if (mkfifo(fifo_reply, 0666))
        return RC_OTHER;

    free(fifo_reply);

    // MAKE REQUEST
    if (requestMessageTLV(argc, argv, &user_request))
        return RC_OTHER;

    // SEND REQUEST
    if (send_request(&user_request))
        return RC_OTHER;

    // START TIME
    clock_t initial = clock();

    // RECEIVE REPLY
    thread_arg_t thread_arg;
    thread_arg.pid = getpid();
    thread_arg.completed = 0;

    pthread_create(&t, NULL, get_reply_thread, (void *)(&thread_arg));

    while (1)
    {
        if (((clock() - initial) / CLOCKS_PER_SEC) == FIFO_TIMEOUT_SECS)
        {
            pthread_cancel(t);
            thread_arg.reply.value.header.ret_code = RC_SRV_TIMEOUT;
            break;
        }
        if (thread_arg.completed)
            break;
    }

    // READ REPLY
    int fd = open(USER_LOGFILE, O_WRONLY | O_APPEND | O_CREAT, 0777);
    logReply(STDOUT_FILENO, getpid(), &thread_arg.reply);
    logReply(fd, getpid(), &thread_arg.reply);

    close(fd);

    return RC_OK;
}