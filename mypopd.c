#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>

#include <limits.h>

#define MAX_LINE_LENGTH 1024

const char *get_last_added_user(user_list_t list);

typedef enum state {
    Undefined,
    // TODO: Add additional states as necessary

    AUTHORIZATION,
    TRANSACTION,
    UPDATE
} State;

typedef struct serverstate {
    int fd;
    net_buffer_t nb;
    char recvbuf[MAX_LINE_LENGTH + 1];
    char *words[MAX_LINE_LENGTH];
    int nwords;
    State state;
    struct utsname my_uname;
    // TODO: Add additional fields as necessary
    char* curr_user;  
    mail_list_t curr_mail_list; //
    mail_item_t curr_mail_item; // 
} serverstate;
static void handle_client(int fd);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
        return 1;
    }
    run_server(argv[1], handle_client);
    return 0;
}

// syntax_error returns
//   -1 if the server should exit
//    1 otherwise
int syntax_error(serverstate *ss) {
    if (send_formatted(ss->fd, "-ERR %s\r\n", "Syntax error in parameters or arguments") <= 0) return -1;
    return 1;
}

// checkstate returns
//   -1 if the server should exit
//    0 if the server is in the appropriate state
//    1 if the server is not in the appropriate state
int checkstate(serverstate *ss, State s) {
    if (ss->state != s) {
        if (send_formatted(ss->fd, "-ERR %s\r\n", "Bad sequence of commands") <= 0) return -1;
        return 1;
    }
    return 0;
}

// All the functions that implement a single command return
//   -1 if the server should exit
//    0 if the command was successful
//    1 if the command was unsuccessful

int do_quit(serverstate *ss) {
    dlog("Executing quit\n");
    // TODO: Implement this function
    ss->state = UPDATE;
    mail_list_destroy(ss->curr_mail_list);
    send_formatted(ss->fd, "+OK Service closing transmission channel\r\n");
    return -1;
}

int do_user(serverstate *ss) {
    dlog("Executing user\n");
    // dlog("number of words: %d", ss->nwords);
    // TODO: Implement this function
    if(ss->nwords == 2) {
        if(is_valid_user(ss->words[1], NULL)) {
            ss->state = AUTHORIZATION;
            // strncpy(ss->curr_user, ss->words[1], NAME_MAX);
            ss->curr_user = strdup(ss->words[1]);
            // user_list_add(ss->userList, ss->words[1]);
            send_formatted(ss->fd, "+OK User is valid, proceed with password\r\n");
        } else {
            ss->state = AUTHORIZATION;
            send_formatted(ss->fd, "-ERR sorry, no mailbox for %s here\r\n", ss->words[1]);
        }
        return 0;
    }
    send_formatted(ss->fd, "-ERR no such message\r\n");
    return 1;
}

int do_pass(serverstate *ss) {
    dlog("Executing pass\n");
    // TODO: Implement this function

    if(checkstate(ss, AUTHORIZATION) != 0) {
        return 1;
    }

    if (ss->nwords == 2){
        char *username = ss->curr_user;
        char *password = ss->words[1];
        if (is_valid_user(username, password)){
            ss->state = TRANSACTION;
            ss->curr_mail_list = load_user_mail(username);
            send_formatted(ss->fd, "+OK Password is valid, mail loaded\r\n");
            return 0;
        } else {
            send_formatted(ss->fd, "-ERR Invalid password\r\n");
            return 1;
        }
    } else {
        return syntax_error(ss);
    }

    return 1;

}

int do_stat(serverstate *ss) {
    dlog("Executing stat\n");
    // TODO: Implement this function
    if (checkstate(ss, TRANSACTION) != 0) {
        return 1;
    }

    unsigned int num_messages = mail_list_length(ss->curr_mail_list, 0); // 0 indicates not to include deleted messages
    unsigned int total_size = mail_list_size(ss->curr_mail_list);

    send_formatted(ss->fd, "+OK %u %u\r\n", num_messages, total_size);

    return 0;
}

int do_list(serverstate *ss) {

    dlog("Executing list\n");
    if(checkstate(ss,TRANSACTION) != 0){
        return 1;
    }

    int total_messages = mail_list_length(ss->curr_mail_list, 1);
    int num_messages = mail_list_length(ss->curr_mail_list, 0); // 0 indicates not to include deleted messages
    // int num_deleted = total_messages - num_messages;
    // size_t total_size = mail_list_size(ss->curr_mail_list);
    if(ss->nwords == 1) {
        send_formatted(ss->fd, "+OK %d messages\r\n", num_messages);
        for(int i = 0; i < total_messages; i++) {
            mail_item_t item = mail_list_retrieve(ss->curr_mail_list, i);
            if (item) {
                send_formatted(ss->fd, "%d %zu\r\n", i + 1, mail_item_size(item));              
            }
        }

        send_formatted(ss->fd, ".\r\n");
        return 0;
        
    } else if(ss->nwords == 2) {
        int message_num = atoi(ss->words[1]);
        if (message_num <= 0 || message_num > total_messages) {
            send_formatted(ss->fd, "-ERR no such message, only %d messages in maildrop\r\n", num_messages);
            return 1;
        } 

        mail_item_t item = mail_list_retrieve(ss->curr_mail_list, message_num - 1);
            // deleted
        if(item == NULL) {
            send_formatted(ss->fd, "-ERR no such message\r\n");
            return 1;
        }
        
        send_formatted(ss->fd, "+OK %d %zu\r\n", message_num, mail_item_size(item));
        return 0;     
        
    } else {
        return syntax_error(ss);
    }

    
    return 1;
    
}


int do_retr(serverstate *ss) {
    dlog("Executing retr\n");

    if(checkstate(ss,TRANSACTION) != 0){
        return 1;
    }

    if(ss->nwords != 2) {
        return syntax_error(ss);
    }
        
    int msg_num = atoi(ss->words[1]);

    if (msg_num <= 0 || msg_num > mail_list_length(ss->curr_mail_list, 1)) {
        send_formatted(ss->fd, "-ERR no such message\r\n");
        return 1;
    }

    mail_item_t mail_item = mail_list_retrieve(ss->curr_mail_list, msg_num - 1);
    
    // deleted
    if(mail_item == NULL) {
        send_formatted(ss->fd, "-ERR no such message\r\n");
        return 1;
    }
    
    FILE *mail_file = mail_item_contents(mail_item);

    send_formatted(ss->fd, "+OK Message follows\r\n");
    char line[1024];
    while (fgets(line, sizeof(line), mail_file) != NULL) {
        send_formatted(ss->fd, "%s", line);

    }
    fclose(mail_file);

    send_formatted(ss->fd, ".\r\n");
    
    return 0;
}

int do_rset(serverstate *ss) {
    dlog("Executing rset\n");

    if(checkstate(ss, TRANSACTION) != 0) {
        return 1;
    }

    int recovered_messages = mail_list_undelete(ss->curr_mail_list);
    send_formatted(ss->fd, "+OK %d message(s) restored\r\n", recovered_messages);

    return 0;
}

int do_noop(serverstate *ss) {
    dlog("Executing noop\n");
    if(checkstate(ss,TRANSACTION) != 0) {
        return 1;
    }
    send_formatted(ss->fd, "+OK (noop)\r\n");
    return 0;
}

int do_dele(serverstate *ss) {
    dlog("Executing dele\n");

    if(checkstate(ss, TRANSACTION) != 0) {
        return 1;
    }

    if (ss->nwords != 2) {
        return syntax_error(ss);
    }

    int pos = atoi(ss->words[1]);
    if (pos <= 0 || pos > mail_list_length(ss->curr_mail_list, 1)) {
        send_formatted(ss->fd, "-ERR no such message\r\n");
        return 1;
    }

    mail_item_t item = mail_list_retrieve(ss->curr_mail_list, pos - 1);
    if (item) {
        mail_item_delete(item);
        send_formatted(ss->fd, "+OK Message deleted\r\n");
        return 0;
    } 
    
    send_formatted(ss->fd, "-ERR Unable to delete message\r\n");
    return 1;
}

void handle_client(int fd) {
    size_t len;
    serverstate mstate, *ss = &mstate;
    ss->fd = fd;
    ss->nb = nb_create(fd, MAX_LINE_LENGTH);
    ss->state = Undefined;
    uname(&ss->my_uname);
    if (send_formatted(fd, "+OK POP3 Server on %s ready\r\n", ss->my_uname.nodename) <= 0) return;

    while ((len = nb_read_line(ss->nb, ss->recvbuf)) >= 0) {
        if (ss->recvbuf[len - 1] != '\n') {
            // command line is too long, stop immediately
            send_formatted(fd, "-ERR Syntax error, command unrecognized\r\n");
            break;
        }
        if (strlen(ss->recvbuf) < len) {
            // received null byte somewhere in the string, stop immediately.
            send_formatted(fd, "-ERR Syntax error, command unrecognized\r\n");
            break;
        }
        // Remove CR, LF and other space characters from end of buffer
        while (isspace(ss->recvbuf[len - 1])) ss->recvbuf[--len] = 0;
        dlog("Command is %s\n", ss->recvbuf);
        if (strlen(ss->recvbuf) == 0) {
            send_formatted(fd, "-ERR Syntax error, blank command unrecognized\r\n");
            break;
        }
        // Split the command into its component "words"
        ss->nwords = split(ss->recvbuf, ss->words);
        char *command = ss->words[0];
        if (!strcasecmp(command, "QUIT")) {
            if (do_quit(ss) == -1) break;
        } else if (!strcasecmp(command, "USER")) {
            if (do_user(ss) == -1) break;
        } else if (!strcasecmp(command, "PASS")) {
            if (do_pass(ss) == -1) break;
        } else if (!strcasecmp(command, "STAT")) {
            if (do_stat(ss) == -1) break;
        } else if (!strcasecmp(command, "LIST")) {
            if (do_list(ss) == -1) break;
        } else if (!strcasecmp(command, "RETR")) {
            if (do_retr(ss) == -1) break;
        } else if (!strcasecmp(command, "RSET")) {
            if (do_rset(ss) == -1) break;
        } else if (!strcasecmp(command, "NOOP")) {
            if (do_noop(ss) == -1) break;
        } else if (!strcasecmp(command, "DELE")) {
            if (do_dele(ss) == -1) break;
        } else if (!strcasecmp(command, "TOP") ||
                   !strcasecmp(command, "UIDL") ||
                   !strcasecmp(command, "APOP")) {
            dlog("Command not implemented %s\n", ss->words[0]);
            if (send_formatted(fd, "-ERR Command not implemented\r\n") <= 0) break;
        } else {
            // invalid command
            if (send_formatted(fd, "-ERR Syntax error, command unrecognized\r\n") <= 0) break;
        }
    }
    nb_destroy(ss->nb);
}
