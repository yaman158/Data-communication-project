#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <stdlib.h>
#include <time.h> 

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BUFFER_SIZE 1024

// ---------------------------------------------------------
// 1. CRC-16 (Standart)
// ---------------------------------------------------------
unsigned short calculate_crc16(const char* data, int length) {
    unsigned short crc = 0xFFFF;
    for (int i = 0; i < length; i++) {
        crc ^= (unsigned char)data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else crc = crc >> 1;
        }
    }
    return crc;
}

// ---------------------------------------------------------
// 2. Parity Bit (Tek/Çift Kontrolü)
// ---------------------------------------------------------
unsigned short calculate_parity_bit(const char* data, int length) {
    int ones_count = 0;
    for (int i = 0; i < length; i++) {
        unsigned char c = (unsigned char)data[i];
        for (int bit = 0; bit < 8; bit++) {
            if ((c >> bit) & 1) ones_count++;
        }
    }
    return (ones_count % 2); 
}

// ---------------------------------------------------------
// 3. Internet Checksum (TCP/IP)
// ---------------------------------------------------------
unsigned short calculate_internet_checksum(const char* data, int length) {
    unsigned long sum = 0;
    const unsigned short *ptr = (const unsigned short *)data;
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }
    if (length > 0) sum += *(unsigned char *)ptr;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (unsigned short)(~sum);
}

// ---------------------------------------------------------
// 4. 2D Parity (LRC + VRC)
// ---------------------------------------------------------
unsigned short calculate_2d_parity(const char* data, int length) {
    unsigned char row_parity = 0; 
    for (int i = 0; i < length; i++) {
        row_parity ^= data[i]; 
    }
    return (unsigned short)((row_parity << 8) | (row_parity ^ 0xFF)); 
}

// ---------------------------------------------------------
// 5. Hamming Code Signature (Bit Pozisyon XORlama)
// ---------------------------------------------------------
// Verideki "1" olan bitlerin pozisyonlarini XORlayarak imza üretir.
unsigned short calculate_hamming_signature(const char* data, int length) {
    unsigned short signature = 0;
    int global_bit_pos = 1; // Hamming saymaya 1'den baslar

    for (int i = 0; i < length; i++) {
        unsigned char c = (unsigned char)data[i];
        for (int j = 0; j < 8; j++) {
            // Eger bit 1 ise, o bitin pozisyonunu imzaya ekle (XOR)
            if ((c >> j) & 1) { 
                signature ^= global_bit_pos; 
            }
            global_bit_pos++;
        }
    }
    // Not: Bu yöntem 8192 byte'a kadar veri için güvenlidir (unsigned short tasmaz)
    return signature;
}

int main() {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in server;
    char message[BUFFER_SIZE], packet[BUFFER_SIZE];
    unsigned short check_value = 0;
    char method_name[20];

    srand(time(NULL)); // Rastgelelik tohumu

    printf("\nWinsock baslatiliyor...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) return 1;

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Baglanti hatasi.\n");
        return 1;
    }

    printf("Gonderilecek mesaji girin: ");
    fgets(message, BUFFER_SIZE, stdin);
    message[strcspn(message, "\n")] = 0;

    // --- RASTGELE YÖNTEM SEÇIMI (0-4 Arasi) ---
    int random_choice = rand() % 5; 

    switch (random_choice) {
        case 0:
            strcpy(method_name, "CRC16");
            check_value = calculate_crc16(message, strlen(message));
            break;
        case 1:
            strcpy(method_name, "PARITY_BIT");
            check_value = calculate_parity_bit(message, strlen(message));
            break;
        case 2:
            strcpy(method_name, "INET_CSUM");
            check_value = calculate_internet_checksum(message, strlen(message));
            break;
        case 3:
            strcpy(method_name, "2D_PARITY");
            check_value = calculate_2d_parity(message, strlen(message));
            break;
        case 4:
            strcpy(method_name, "HAMMING");
            check_value = calculate_hamming_signature(message, strlen(message));
            break;
    }

    // Paketi Hazirla: DATA|METHOD|CHECK_VALUE
    sprintf(packet, "%s|%s|%04X", message, method_name, check_value);
    
    printf("\n--- GONDERIM RAPORU ---\n");
    printf("Mesaj          : %s\n", message);
    printf("Secilen Yontem : %s\n", method_name);
    printf("Hesaplanan Kod : %04X\n", check_value);
    printf("-----------------------\n");

    if (send(s, packet, strlen(packet), 0) < 0) {
        printf("Gonderim hatasi.\n");
        return 1;
    }

    closesocket(s);
    WSACleanup();
    return 0;
}
