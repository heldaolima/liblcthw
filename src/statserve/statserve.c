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
#include <statserve/net.h>

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



int server_echo(const char* host, const char *port)
{
    check(host != NULL, "Invalid host.");
    check(port != NULL, "Invalid port.");

    int server_fd, client_fd; 
    struct sockaddr_in server_addr, client_addr;
    int rc;

    server_fd = server_listen(host, port); 
    check(server_fd >= 0, "Failed to create server socket.");

    socklen_t server_len = sizeof(server_addr);
    socklen_t client_len = sizeof(client_addr);

    log_info("Server is listening at %s", port);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_len);
        check(client_fd >= 0, "Failed toaccept connection.");
        
        log_info("Connected: %s:%d, file descriptor: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);

        echo(client_fd);

        log_info("Connection closed: %d\n", client_fd);
        close(client_fd);
    }

    return 0;

error:
    return -1;
}