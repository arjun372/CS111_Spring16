/*
 *	Demonstration TLS client
 *
 *       Compile with
 *
 *       gcc -Wall -o tls_client tls_client.c -L/usr/lib -lssl -lcrypto
 *
 *       Execute with
 *
 *       ./tls_client <server_INET_address> <port> <client message string>
 *
 *       Generate certificate with
 *
 *       openssl req -x509 -nodes -days 365 -newkey rsa:1024 -keyout tls_demonstration_cert.pem -out tls_demonstration_cert.pem
 *
 *	 Developed for Intel Edison IoT Curriculum by UCLA WHI
 */
#define  TRUE 1
#define  LOGFILE_NAME "log_file"
#include "pthread.h"
#include "tls_header.h"

void *do_read(void * val); // Function prototype for pthread
int LOGFILE_FD;
int rate;

int main(int args, char *argv[])
{
        int port, range, rate;
        int server;
        int receive_length, line_length;
        char ip_addr[BUFSIZE];
        char *my_ip_addr;
        char *line = NULL;
        char buf[BUFSIZE];
        double heart_rate;
        FILE *fp = NULL;
        SSL *ssl;
        SSL_CTX *ctx;

        my_ip_addr = get_ip_addr();
        printf("My ip addr is: %s\n", my_ip_addr);

        /* READ INPUT FILE */
        fp = fopen("config_file", "r");
        if(fp == NULL) {
                fprintf(stderr, "Error opening config file with name 'config_file'. Exiting.\n");
                exit(1);
        }

        /* OPEN LOG FILE */
        LOGFILE_FD = open(LOGFILE_NAME, O_WRONLY|O_TRUNC|O_CREAT, 0666);
        if(LOGFILE_FD < 0) {
                fprintf(stderr, "Error opening log file with name '%s'. Exiting.\n", LOGFILE_NAME);
                exit(1);
        }

        printf("Reading input file...\n");
        while(getline(&line, &line_length, fp) > 0) {
                if(strstr(line, "host_ip") != NULL) {
                        sscanf(line, "host_ip: %s\n", ip_addr);
                }
                else if(strstr(line, "port") != NULL) {
                        sscanf(line, "port: %d\n", &port);
                }
                else if(strstr(line, "range") != NULL) {
                        sscanf(line, "range: %d\n", &range);
                }
                else if(strstr(line, "rate") != NULL) {
                        sscanf(line, "rate: %d\n", &rate);
                }
                else{
                        fprintf(stderr, "Unrecognized line found: %s. Ignoring.\n", line);
                }
        }
        fclose(fp);
        /* FINISH READING INPUT FILE */

        printf("Connecting to: %s:%d\n", ip_addr, port);
        dprintf(LOGFILE_FD, "Connecting to: %s:%d\n", ip_addr, port);

        /* SET UP TLS COMMUNICATION */
        SSL_library_init();
        ctx = initialize_client_CTX();
        server = open_port(ip_addr, port);
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, server);
        /* FINISH SETUP OF TLS COMMUNICATION */

        /* SEND HEART RATE TO SERVER */
        if (SSL_connect(ssl) == -1) { //make sure connection is valid
                fprintf(stderr, "Error. TLS connection failure. Aborting.\n");
                dprintf(LOGFILE_FD, "Error. TLS connection failure. Aborting.\n");
                ERR_print_errors_fp(stderr);
                close(LOGFILE_FD);
                exit(1);
        }
        else {
                printf("Client-Server connection complete with %s encryption\n", SSL_get_cipher(ssl));
                dprintf(LOGFILE_FD, "Client-Server connection complete with %s encryption\n", SSL_get_cipher(ssl));
                display_server_certificate(ssl);
        }

        pthread_t readThread;
        pthread_create(&readThread, NULL, do_read, (void*) ssl);

        while(TRUE) {
                printf("Current settings: rate: %d, range: %d\n", rate, range);
                dprint(LOGFILE_FD, "Current settings: rate: %d, range: %d\n", rate, range);

                heart_rate = generate_random_number(AVERAGE_HEART_RATE-(double)range, AVERAGE_HEART_RATE+(double)range);

                //populate the buffer with information about the ip address of the client and the heart rate
                sprintf(buf, "Heart rate of patient %s is %4.2f", my_ip_addr, heart_rate);
                printf("Sending message '%s' to server...\n", buf);
                dprintf(LOGFILE_FD, "Sending message '%s' to server...\n", buf);
                SSL_write(ssl, buf, strlen(buf));
                memset(buf,0,sizeof(buf)); //clear out the buffer
                sleep(rate);
        }
        /* FINISH HEART RATE TO SERVER */

        //clean up operations
        SSL_free(ssl);
        close(server);
        close(LOGFILE_FD);
        SSL_CTX_free(ctx);
        return 0;
}

void *do_read(void *ssl_info) {

        char buf[BUFSIZE];
        int receive_length;
        SSL *ssl = (SSL *)ssl_info;

        while(TRUE) {
                memset(buf,0,sizeof(buf));
                receive_length = SSL_read(ssl, buf, sizeof(buf));
                if(strstr(buf, "new_rate: ") != NULL)
                {
                        sscanf(buf, "Heart rate of patient %*s is %*f new_rate: %d", &rate);
                        rate = rate;
                        printf("New rate %d received from server.\n", rate);
                        dprintf(LOGFILE_FD, "New rate %d received from server.\n", rate);
                }
                printf("Received message '%s' from server.\n\n", buf);
                dprintf(LOGFILE_FD, "Received message '%s' from server.\n\n", buf);
        }
}
