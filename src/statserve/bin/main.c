#include <stdio.h>
#include <lcthw/dbg.h>
#include <statserve/statserve.h>

int main(int argc, char* argv[])
{ 
    if (argc == 1) {
        check(server_echo("127.0.0.1", "7899"), "Failed to connect to server.");
    }
    else {
        check(argc == 3, "USAGE: statserve host port");
        
        const char *host = argv[1];
        const char *port = argv[2];

        check(server_echo(host, port), "Failed to connect to server.");
    }

    return 0;

error:
    return 1;
}