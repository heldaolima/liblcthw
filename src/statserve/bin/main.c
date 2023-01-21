#include <stdio.h>
#include <lcthw/dbg.h>
#include <statserve/statserve.h>

int main(int argc, char* argv[])
{
    check(argc == 3, "USAGE: statserve host port");
    
    check(server_echo(argv[1], argv[2]), "Failed to connect to server.");

    return 0;

error:

    return 1;
}