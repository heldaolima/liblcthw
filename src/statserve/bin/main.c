#include <stdio.h>
#include <lcthw/dbg.h>
#include <statserve/statserve.h>

int main(int argc, char* argv[])
{
    check(argc == 3, "USAGE: statserve host port");
    
    const char *host = argv[1];
    const char *port = argv[2];

    check(server_echo(host, port), "Failed to connect to server.");

    return 0;

error:

    return 1;
}