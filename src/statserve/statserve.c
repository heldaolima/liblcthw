#undef NDEBUG
#include <stdlib.h>
#include <sys/select.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <statserve/net.h>
#include <lcthw/dbg.h>
#include <lcthw/hashmap.h>
#include <lcthw/stats.h>

#define BUFFER_SIZE 1024
const int RB_SIZE = 1024 * 10;

#define equal_strs(s1, s2) (strcmp((s1), (s2)) == 0)

struct tagbstring LINE_SPLIT = bsStatic(" ");
struct tagbstring CREATE = bsStatic("create");
struct tagbstring STDDEV = bsStatic("stddev");
struct tagbstring MEAN = bsStatic("mean");
struct tagbstring SAMPLE = bsStatic("sample");
struct tagbstring DUMP = bsStatic("dump");
struct tagbstring DELETE = bsStatic("delete");
struct tagbstring OK = bsStatic("OK\n");
struct tagbstring ERR = bsStatic("ERR\n");
struct tagbstring DNE = bsStatic("DNE\n");
struct tagbstring EXISTS = bsStatic("EXISTS\n");
const char LINE_ENDING = '\n';

Hashmap *DATA = NULL;

struct Command;

typedef int (*handler_cb)(struct Command *cmd, RingBuffer *send_rb);

typedef struct Command {
    bstring command;
    bstring name;
    bstring number;
    handler_cb handler;
} Command;

typedef struct Record {
    bstring name;
    Stats *stat;
} Record;

void echo(int client_fd)
{
    char* buffer = calloc(BUFFER_SIZE, sizeof(char));
    check_mem(buffer);

    size_t read_bytes;
    int rc;
    do {
        read_bytes = read(client_fd, buffer, BUFFER_SIZE);
        check(read_bytes > 0, "Failed to read from client.");

        log_info("Read from client %d: %s", client_fd, buffer);

        rc = send(client_fd, buffer, read_bytes, 0);
        check(rc >= 0, "Failed to write to client.");

        bzero(buffer, read_bytes);
    } while (strncmp(buffer, "bye\r", 4) != 0);

    free(buffer);
error: 
    if (buffer) free(buffer);
    return;
}

void send_reply(RingBuffer *send_rb, bstring reply)
{
    RingBuffer_puts(send_rb, reply);
}

int handle_create(Command *cmd, RingBuffer *send_rb)
{
    int rc = 0;

    if (Hashmap_get(DATA, cmd->name)) {
        send_reply(send_rb, &EXISTS);
    } else {
	// allocate a record
        debug("Create: %s %s", bdata(cmd->name), bdata(cmd->number));

        Record *info = calloc(1, sizeof(Record));
        check_mem(info);

        info->stat = Stats_create();
        check_mem(info->stat);

        info->name = bstrcpy(cmd->name);
        check_mem(info->name);

        // first sample
        Stats_sample(info->stat, atof(bdata(cmd->number)));

        rc = Hashmap_set(DATA, info->name, info);
        check(rc == 0, "Failed to add data to map.");

        send_reply(send_rb, &OK);
    }

    return 0;

error:
    return -1;
}

int handle_sample(Command *cmd, RingBuffer *send_rb)
{
    log_info("sample: %s", cmd->name);
    Record *info = Hashmap_get(DATA, cmd->name);

    if (info == NULL) {
        send_reply(send_rb, &DNE);
    } else {
        Stats_sample(info->stat, atof(bdata(cmd->number)));
        bstring reply = bformat("%f\n", Stats_mean(info->stat));
        send_reply(send_rb, reply);
        bdestroy(reply);
    }

    return 0;
}

int handle_delete(Command *cmd, RingBuffer *send_rb)
{
    log_info("delete: %s", bdata(cmd->name));
    Record *info = Hashmap_get(DATA, cmd->name);

    if (info == NULL) {
        send_reply(send_rb, &DNE);
    } else {
        Hashmap_delete(DATA, cmd->name);

        free(info->stat);
        bdestroy(info->name);
        free(info);

        send_reply(send_rb, &OK);
    }

    return 0;
}

int handle_mean(Command *cmd, RingBuffer *send_rb)
{
    log_info("Mean %s", bdata(cmd->name));
    Record *info = Hashmap_get(DATA, cmd->name);

    if (info == NULL) {
        send_reply(send_rb, &DNE);
    } else {
        bstring reply = bformat("%f\n", Stats_mean(info->stat));
        send_reply(send_rb, reply);
        bdestroy(reply);
    }

    return 0;
}

int handle_stddev(Command *cmd, RingBuffer *send_rb)
{
    log_info("stddev: %s", bdata(cmd->name));
    Record *info = Hashmap_get(DATA, cmd->name);

    if (info == NULL) {
        send_reply(send_rb, &DNE);
    } else {
        bstring reply = bformat("%f\n", Stats_stddev(info->stat));
        send_reply(send_rb, reply);
        bdestroy(reply);
    }

    return 0;
}

int handle_dump(Command *cmd, RingBuffer *send_rb)
{
    log_info("dump: %s", bdata(cmd->name));
    Record *info = Hashmap_get(DATA, cmd->name);

    if (info == NULL) {
        send_reply(send_rb, &DNE);
    } else {
        bstring reply = bformat("%f %f %f %f %ld %f %f\n",
                Stats_mean(info->stat),
                Stats_stddev(info->stat),
                info->stat->sum,
                info->stat->sumsq,
                info->stat->n,
                info->stat->min,
                info->stat->max);

        send_reply(send_rb, reply);
        bdestroy(reply);
    }

    return 0;
}

int parse_command(struct bstrList *splits, Command *cmd)
{
    cmd->command = splits->entry[0];

    if (biseq(cmd->command, &CREATE)) {
        check(splits->qty == 3, "Failed to parse create: %d", splits->qty);
        cmd->name = splits->entry[1];
        cmd->number = splits->entry[2];
        cmd->handler = handle_create;
    } else if (biseq(cmd->command, &MEAN)) {
        check(splits->qty == 2, "Failed to parse mean: %d", splits->qty);
        cmd->name = splits->entry[1];
        cmd->handler = handle_mean;
    } else if (biseq(cmd->command, &SAMPLE)) {
        check(splits->qty == 3, "Failed to parse sample: %d", splits->qty);
        cmd->name = splits->entry[1];
        cmd->number = splits->entry[2];
        cmd->handler = handle_sample;
    } else if (biseq(cmd->command, &DUMP)) {
        check(splits->qty == 2, "Failed to parse dump: %d", splits->qty);
        cmd->name = splits->entry[1];
        cmd->handler = handle_dump;
    } else if (biseq(cmd->command, &DELETE)) {
        check(splits->qty == 2, "Failed to parse delete: %d", splits->qty);
        cmd->name = splits->entry[1];
        cmd->handler = handle_delete;
    } else if (biseq(cmd->command, &STDDEV)) {
        check(splits->qty == 2, "Failed to parse stddev: %d", splits->qty);
        cmd->name = splits->entry[1];
        cmd->handler = handle_stddev;
    } else {
        sentinel("Failed to parse the command: unknown");
    }

    return 0;

error:
    return -1;
}

int parse_line(bstring data, RingBuffer *send_rb)
{
    int rc = -1;
    Command cmd = {.command = NULL};

    struct bstrList *splits = bsplits(data, &LINE_SPLIT);
    check(splits != NULL, "Bad data.");

    rc = parse_command(splits, &cmd);
    check(rc == 0, "Failed to parse command.");

    rc = cmd.handler(&cmd, send_rb);
    

error: // fallthrough
    if (splits) bstrListDestroy(splits);
    return rc;

}

void client_handler(int client_fd)
{
    int rc = 0;
    RingBuffer *recv_rb = RingBuffer_create(RB_SIZE);
    RingBuffer *send_rb = RingBuffer_create(RB_SIZE);

    check_mem(recv_rb);
    check_mem(send_rb);

    while (read_some(recv_rb, client_fd, 1) != -1) {
        bstring data = read_line(recv_rb, LINE_ENDING);
        check(data != NULL, "Client closed.");

        rc = parse_line(data, send_rb);
        bdestroy(data);
        check(rc == 0, "Failed to parse user. Closing.");

        if (RingBuffer_available_data(send_rb)) {
            write_some(send_rb, client_fd, 1);
        }
    }

    rc = close(client_fd);
    check(rc != -1, "Failed to close the socket.");

error: // fallthrough
    if (recv_rb) RingBuffer_destroy(recv_rb);
    if (send_rb) RingBuffer_destroy(send_rb);
}

int setup_data()
{
    DATA = Hashmap_create(NULL, NULL);
    check_mem(DATA);

    return 0;

error:
    return -1;
}

int server_echo(const char *host, const char *port)
{
    check(host != NULL, "Invalid host.");
    check(port != NULL, "Invalid port.");

    int server_fd, client_fd; 
    struct sockaddr_in server_addr, client_addr;
    int rc;

    rc = setup_data();
    check(rc != -1, "Failed to setup the data store.");
    
    server_fd = server_listen(host, port); 
    check(server_fd >= 0, "Failed to create server socket.");

    socklen_t server_len = sizeof(server_addr);
    socklen_t client_len = sizeof(client_addr);

    log_info("Server is listening at %s", port);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len);
        check(client_fd >= 0, "Failed toaccept connection.");
        
        log_info("Connected: %s:%d, file descriptor: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);

        // echo(client_fd);
        client_handler(client_fd);

        log_info("Connection closed: %d\n", client_fd);
        close(client_fd);
    }

    return 0;

error:
    return -1;
}
