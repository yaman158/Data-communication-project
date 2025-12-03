#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

#define LISTEN_PORT 8889
#define BUFFER_SIZE 1024


unsigned short calculate_crc16(const char* data, int length) {
    unsigned short crc = 0xFFFF;
    for (int i = 0; i < length; i++) {
        crc ^= (unsigned char)data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc = crc >> 1;
        }
    }
    return crc;
}

int main() {
    WSADATA wsa;
    SOCKET s_listen, s_server;
    struct sockaddr_in server, incoming;
    int c, recv_size;
    char buffer[BUFFER_SIZE];
    

    char received_data[BUFFER_SIZE];
    char method[20];
    char sent_crc_str[10];
    unsigned short computed_crc, sent_crc;

    WSAStartup(MAKEWORD(2, 2), &wsa);

    s_listen = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(LISTEN_PORT);

    bind(s_listen, (struct sockaddr*)&server, sizeof(server));
    listen(s_listen, 1);

    printf("Client 2 (Receiver) hazir. Dinliyor (Port: %d)...\n", LISTEN_PORT);
    
    c = sizeof(struct sockaddr_in);
    s_server = accept(s_listen, (struct sockaddr*)&incoming, &c);
    
    recv_size = recv(s_server, buffer, BUFFER_SIZE, 0);
    if (recv_size > 0) {
        buffer[recv_size] = '\0';
        


        char *token1 = strchr(buffer, '|');
        char *token2 = strrchr(buffer, '|');

        if (token1 && token2 && token1 != token2) {
            int data_len = token1 - buffer;
            strncpy(received_data, buffer, data_len);
            received_data[data_len] = '\0';


            int method_len = token2 - (token1 + 1);
            strncpy(method, token1 + 1, method_len);
            method[method_len] = '\0';

            strcpy(sent_crc_str, token2 + 1);
            
            sent_crc = (unsigned short)strtol(sent_crc_str, NULL, 16);


            computed_crc = calculate_crc16(received_data, strlen(received_data));


            printf("\n--- SONUC RAPORU ---\n");
            printf("Received Data       : %s\n", received_data);
            printf("Method              : %s\n", method);
            printf("Sent Check Bits     : %04X\n", sent_crc);
            printf("Computed Check Bits : %04X\n", computed_crc);
            printf("--------------------\n");
            
            if (sent_crc == computed_crc) {
                printf("Status: DATA CORRECT\n");
            } else {
                printf("Status: DATA CORRUPTED\n");
            }

        } else {
            printf("Gecersiz paket formati.\n");
        }
    }

    closesocket(s_server);
    closesocket(s_listen);
    WSACleanup();
    

    printf("\nCikmak icin Enter'a basin...");
    getchar();

    return 0;
}
