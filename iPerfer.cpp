//
// Created by 徐涛 on 2023/1/31.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

int client(char* hostname, int port, int time){
    int client_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // connect
    struct hostent *server = gethostbyname(hostname);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = * (unsigned long *) server->h_addr_list[0];
    addr.sin_port = htons(port);
    if(connect(client_socket_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1){
        printf("Failed to connect to the server.\n");
        exit(-1);
    }
    printf("Connect to the server\n");
    unsigned long byte_num = 0;
    // keep sending 1000 bytes
    char msg[1000];
    memset(msg, '0', 1000);
    clock_t start_time,finish_time;
    start_time = clock();
    while(true){
        finish_time = clock();
        if(((double)(finish_time - start_time) / CLOCKS_PER_SEC) >= double(time)) {
            break;
        }
        int send_len = send(client_socket_fd, &msg, strlen(msg), MSG_NOSIGNAL);

        byte_num += send_len;
    }

    printf("Finished sending data\n");

    // send Fin message
    char *fin_message = "Finished";
    send(client_socket_fd, fin_message, strlen(fin_message), MSG_NOSIGNAL);
    printf("Finish sending Fin message\n");

    // wait for acknowledgement
    char acknowledgement[20] = "";
    recv(client_socket_fd, &acknowledgement, 16, MSG_NOSIGNAL);

    // close socket
    byte_num /= 1000;
    double speed = byte_num / ((double)(finish_time - start_time) / CLOCKS_PER_SEC) / 1000 * 8;
    printf("Sent=%lu KB, Rate=%.3f Mbps\n", byte_num, speed);
//    close(client_socket_fd);
    return 0;


}


int server(int listen_port){
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; // Address family
    addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    addr.sin_port = htons(listen_port); // Host to network long
    bind(server_socket_fd, (struct sockaddr *) &addr, sizeof(addr));

    // listen
    listen(server_socket_fd, 10);
    printf("listening\n");

    // accept
    socklen_t addr_len = sizeof(addr);
    int conn = accept(server_socket_fd, (struct sockaddr *) &addr, &addr_len);
    printf("Accepted the client.\n");
    // receive data
    unsigned long count = 0;
    unsigned long start_time, end_time;
    int flag = 0;
    while(true){
        char buffer[1001] = "";
        int byte_recved = recv(conn, &buffer, 1000, MSG_NOSIGNAL);
        if(flag == 0){
            start_time = clock();
            flag = 1;
        }
        if(strcmp(buffer, "Finished") == 0){
            end_time = clock();
            break;
        }
        count += byte_recved;
    }

    // send acknowledgement message
    char* acknow_message = "Acknowledgement";
    send(server_socket_fd, acknow_message, strlen(acknow_message), MSG_NOSIGNAL);
    count /= 1000;
    double speed = count / ((double)(end_time - start_time) / CLOCKS_PER_SEC) / 1000 * 8;
    printf("Received=%lu KB, Rate=%.3f Mbps\n", count, speed);
//    close(server_socket_fd);
    return 0;
}


int main(int argc, char* argv[]){
    // Server mode
    if(strcmp(argv[1], "-s") == 0 and argc == 4){
        int listen_port = atoi(argv[3]);
        if(listen_port < 1024 or listen_port > 65535){
            printf("Error: port number must be in the range of [1024, 65535]\n");
            return 1;
        }
        server(listen_port);
    }
    // Customer mode
    else if(strcmp(argv[1], "-c") == 0 and argc == 8){
        char* hostname = argv[3];
        int port = atoi(argv[5]);
        if(port < 1024 or port > 65535){
            printf("Error: port number must be in the range of [1024, 65535]\n");
            return 1;
        }
        int time = atoi(argv[7]);
        if(time <= 0){
            printf("Error: time argument must be greater than 0\n");
            return 1;
        }
        client(hostname, port, time);
    }
    // return error
    else{
        printf("Error: missing or extra arguments\n");
        return 1;
    }
    return 0;
}
