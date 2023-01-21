#undef NDEBUG
#include <stdlib.h>
#include <sys/select.h>
#include <stdio.h>
#include <lcthw/dbg.h>
#include <lcthw/bstrlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#define PORT 7899
#define BUFFER_SIZE 1024

#define equal_strs(s1, s2) (strcmp((s1), (s2)) == 0)

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


int main(int argc, char* argv[])
{
    // bstring local = bsStatic("127.0.0.1");
    int server_fd, client_fd; 
    struct sockaddr_in server_addr, client_addr;
    int rc;

    server_fd = socket(AF_INET, SOCK_STREAM, 0); 
    check(server_fd != -1, "Failed to create server socket.");


    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    socklen_t server_len = sizeof(server_addr);
    socklen_t client_len = sizeof(client_addr);

    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    rc = bind(server_fd, (struct sockaddr *) &server_addr, server_len);
    check(rc >= 0, "Could not bind socket.");

    rc = listen(server_fd, 128);
    check(rc >= 0, "Fail on listen.");

    log_info("Server is listening at %d", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len);
        check(client_fd > 0, "Failed to establish connection with client.");
        log_info("Connected: %s:%d, file descriptor: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);

        echo(client_fd);

        log_info("Connection closed: %d\n", client_fd);
        close(client_fd);
    }

    return 0;

error:
    return -1;
}