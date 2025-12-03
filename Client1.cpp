#include <stdio.h>
#include <winsock2.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib") 

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
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
    SOCKET s;
    struct sockaddr_in server;
    char message[BUFFER_SIZE], packet[BUFFER_SIZE];
    unsigned short crc;


    printf("\nWinsock baslatiliyor...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Hata. Kod: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Baslatildi.\n");


    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket olusturulamadi : %d\n", WSAGetLastError());
        return 1;
    }

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);


    if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Baglanti hatasi.\n");
        return 1;
    }
    printf("Server'a baglanildi.\n");


    printf("Gonderilecek mesaji girin: ");
    fgets(message, BUFFER_SIZE, stdin);
    message[strcspn(message, "\n")] = 0;


    crc = calculate_crc16(message, strlen(message));


    sprintf(packet, "%s|CRC16|%04X", message, crc);
    printf("Gonderilen Paket: %s\n", packet);


    if (send(s, packet, strlen(packet), 0) < 0) {
        printf("Gonderim hatasi.\n");
        return 1;
    }

    closesocket(s);
    WSACleanup();
    return 0;
}
