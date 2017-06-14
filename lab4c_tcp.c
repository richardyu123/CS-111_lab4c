#define _GNU_SOURCE

#include "mraa/aio.h"
#include "mraa/gpio.h"

#include <signal.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <getopt.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int running = 1;
int period = 1;
int use_celsius = 0;

struct pollfd p_fds[1];
int timeout = 0;
mraa_aio_context context;
uint16_t value;
float R, temperature;
time_t start, p_time, curr_time;
//Guarantees that the program will start by logging a temperature
double elapsed = INT_MAX;
struct tm * t;
char sample[20];  

mraa_gpio_context buttonContext;

int log_called = 0;
int port_num;
FILE * log_file;
int socket_fd;
int id;
char * host_name;

void send_error(char* error_string, int return_code) {
    fprintf(stderr, "%s\n", error_string);
    exit(return_code);
}

void terminate() {
    //print SHUTDOWN to stdout and logfile
    curr_time = time(NULL);
    t = localtime(&curr_time);
    mraa_aio_close(context);
    mraa_gpio_close(buttonContext);
    if (log_called) {
        fprintf(log_file, "%02d:%02d:%02d SHUTDOWN\n", t->tm_hour, t->tm_min, t->tm_sec); 
        if (fclose(log_file)) {
            send_error(strerror(errno), 2);
        }
    }
    close(socket_fd);
    exit(0);   
}

void generateReports() {
    time(&p_time);
    if (running && elapsed >= (double)period) {
        time(&start);
        curr_time = time(NULL);
        t = localtime(&curr_time);

	//Temperature algorithm from wiki.seeed.cc/Grove-Temperature_Sensor_V1.2/
        value = mraa_aio_read(context);
        R = 1023.0 / ((float)value) - 1.0;
        R *= 100000.0;
        temperature = 1.0 / (log(R / 100000.0) / 4275 + (1 / 298.15)) - 273.15;

        if (!use_celsius) {
            temperature = temperature * 1.8 + 32;
        }
        sprintf(sample, "%02d:%02d:%02d %04.1f\n", t->tm_hour, t->tm_min, t->tm_sec, temperature);
        send(socket_fd, sample, strlen(sample), 0);
        if (log_called) {
            fprintf(log_file, sample);
        }
    }
    elapsed = difftime(p_time, start);
}

int main(int argc, char ** argv) {
    if (argc == 1) {
        send_error("Error: there must be an argument for the port number", 1);
    }
    static struct option long_options[] = {
        {"id", required_argument, 0, 'i'},
        {"host", required_argument, 0, 'h'},
        {"log", required_argument, 0, 'l'}
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
            case 'l':
                log_file = fopen(optarg, "w");
                if (log_file == NULL) {
                    send_error(strerror(errno), 2);
                }
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
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        send_error(strerror(errno), 2);
    }
    struct hostent * server = gethostbyname(host_name);
    if (server == NULL) {
        server = gethostbyaddr(host_name, strlen(host_name), AF_INET);
        if (server == NULL) {
            send_error("Error: cannot find host", 1);
        }
    }
    if (server == NULL) {
        send_error("Error: unable to connect to host", 2);
    }
    addr.sin_port = htons(port_num);
    addr.sin_family = AF_INET;
    memcpy((char*) &addr.sin_addr.s_addr, (char*)server->h_addr,
            server->h_length);

    if (connect(socket_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))
            == -1) {
        send_error(strerror(errno), 2);
    }

    signal(SIGINT, terminate);

    mraa_init();
    context = mraa_aio_init(0);
    buttonContext = mraa_gpio_init(3);
    mraa_gpio_dir(buttonContext, MRAA_GPIO_IN);

    time(&start);
    
    listen(socket_fd, 5);
    p_fds[0].fd = socket_fd;
    char message[15];
    sprintf(message, "ID=%9d\n", id);
    send(socket_fd, message, strlen(message), 0);
    p_fds[0].events = POLLIN | POLLERR;

    char buffer[64];
    int periodArg;
    int valid_command;
    while (1) {
        // Checks the temperature and records it to stdout and the log
        generateReports();

        // Checks if the button has been pressed
        if (mraa_gpio_read(buttonContext) != 0) {
            terminate();
        }

        // Polls stdin to check if anything has been input
        int poll_ret = poll(p_fds, 1, timeout);
        valid_command = 1;
        if (poll_ret > 0) {
            if (p_fds[0].revents & POLLIN) {
                int i = read(socket_fd, buffer, 64);
                buffer[i - 1] = '\0';
                if (strcmp(buffer, "OFF") == 0) {
                    if (log_called) {
                        fprintf(log_file, "%s\n", buffer);
                    }
                    terminate();
                } else if (strcmp(buffer, "STOP") == 0) {
                    running = 0;
                } else if (strcmp(buffer, "START") == 0) {
                    running = 1;
                } else if (strcmp(buffer, "SCALE=F") == 0) {
                    use_celsius = 0;
                } else if (strcmp(buffer, "SCALE=C") == 0) {
                    use_celsius = 1;
                } else if (sscanf(buffer, "PERIOD=%d", &periodArg) == 1) {
                    period = periodArg;
                } else {
                    valid_command = 0;
                }
                if (log_called && valid_command) {
                    fprintf(log_file, "%s\n", buffer);
                }
            } else if (p_fds[0].revents & POLLERR) {
                fprintf(stderr, "Error: Error reading from stdin");
                exit(1);
            }
        }
    }
}
