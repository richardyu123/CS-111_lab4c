#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int log_called = 0;
int port_num;
int log_fd;
int id;
char * host_name;

void send_error(char* error_string, int return_code) {
    fprintf(stderr, "%s\n", error_string);
    exit(return_code);
}

int main(int argc, char ** argv) {
    static struct option long_options[] = {
        {"id", required_argument, 0, 'i'},
        {"host", required_argument, 0, 'h'},
        {"log", required_argument, 0, 'f'}
    };
    int curr_opt = getopt_long(argc - 1, argv, "i:h:f", long_options, NULL);
    while (curr_opt != -1) {
        switch (curr_opt) {
            case 'i':
                if (strlen(optarg) != 9) {
                    send_error("Error: ID must be of length 9", 1);
                } else {
                    id = atoi(optarg);
                }
                break;
            case 'h':
                host_name = optarg;
                break;
            case 'f':
                log_called = 1;
                break;
            default:
                send_error("Usage: ./lab4c_tcp --id=[id-num] --host=[host-name] --log=[file-name] portnum", 1);
                break;

        }
        
        curr_opt = getopt_long(argc - 1, argv, "i:h:f", long_options, NULL);
    }
    if ((port_num = atoi(argv[optind])) == 0) {
        send_error("Error: port number is invalid or missing", 1);
    }
    struct sockaddr_in addr;
}
