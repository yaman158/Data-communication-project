#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

#define LISTEN_PORT 8889
#define BUFFER_SIZE 1024

// =============================================================
//  DOGRULAMA ALGORITMALARI (Client 1 ile Birebir Ayni Olmali)
// =============================================================

// 1. CRC-16
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

// 2. Parity Bit
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

// 3. Internet Checksum
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

// 4. 2D Parity
unsigned short calculate_2d_parity(const char* data, int length) {
    unsigned char row_parity = 0;
    for (int i = 0; i < length; i++) {
        row_parity ^= data[i];
    }
    return (unsigned short)((row_parity << 8) | (row_parity ^ 0xFF));
}

// 5. Hamming Code Signature
unsigned short calculate_hamming_signature(const char* data, int length) {
    unsigned short signature = 0;
    int global_bit_pos = 1;
    for (int i = 0; i < length; i++) {
        unsigned char c = (unsigned char)data[i];
        for (int j = 0; j < 8; j++) {
            if ((c >> j) & 1) {
                signature ^= global_bit_pos;
            }
            global_bit_pos++;
        }
    }
    return signature;
}

// =============================================================
//  MAIN PROGRAM
// =============================================================

int main() {
    WSADATA wsa;
    SOCKET s_listen, s_server;
    struct sockaddr_in server, incoming;
    int c, recv_size;
    char buffer[BUFFER_SIZE];
    
    // Ayristirma Degiskenleri
    char received_data[BUFFER_SIZE];
    char method[20];
    char sent_crc_str[10];
    unsigned short computed_val = 0, sent_val = 0;

    WSAStartup(MAKEWORD(2, 2), &wsa);

    s_listen = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(LISTEN_PORT);

    bind(s_listen, (struct sockaddr*)&server, sizeof(server));
    listen(s_listen, 1);

    printf("Client 2 (Alici) hazir. Port: %d dinleniyor...\n", LISTEN_PORT);
    
    c = sizeof(struct sockaddr_in);
    s_server = accept(s_listen, (struct sockaddr*)&incoming, &c);
    
    printf("Baglanti kabul edildi. Veri bekleniyor...\n");

    recv_size = recv(s_server, buffer, BUFFER_SIZE, 0);
    if (recv_size > 0) {
        buffer[recv_size] = '\0';
        
        // PAKET FORMATI: DATA | METHOD | CHECKSUM
        char *token1 = strchr(buffer, '|');
        char *token2 = strrchr(buffer, '|');

        if (token1 && token2 && token1 != token2) {
            // 1. Veriyi Al
            int data_len = token1 - buffer;
            strncpy(received_data, buffer, data_len);
            received_data[data_len] = '\0';

            // 2. Metodu Al
            int method_len = token2 - (token1 + 1);
            strncpy(method, token1 + 1, method_len);
            method[method_len] = '\0';

            // 3. Gönderilen Checksum'i Al
            strcpy(sent_crc_str, token2 + 1);
            sent_val = (unsigned short)strtol(sent_crc_str, NULL, 16);

            // 4. METODA GÖRE HESAPLAMA YAP
            printf("\n--- ANALIZ BASLIYOR ---\n");
            printf("Algilanan Yontem: %s\n", method);

            if (strcmp(method, "CRC16") == 0) {
                computed_val = calculate_crc16(received_data, strlen(received_data));
            } 
            else if (strcmp(method, "PARITY_BIT") == 0) {
                computed_val = calculate_parity_bit(received_data, strlen(received_data));
            }
            else if (strcmp(method, "INET_CSUM") == 0) {
                computed_val = calculate_internet_checksum(received_data, strlen(received_data));
            }
            else if (strcmp(method, "2D_PARITY") == 0) {
                computed_val = calculate_2d_parity(received_data, strlen(received_data));
            }
            else if (strcmp(method, "HAMMING") == 0) {
                computed_val = calculate_hamming_signature(received_data, strlen(received_data));
            }
            else {
                printf("HATA: Bilinmeyen Metod (%s)!\n", method);
                computed_val = 0xFFFF; // Hatayi garantilemek için rastgele bir deger
            }

            // 5. SONUÇ RAPORU
            printf("\n--- SONUC RAPORU ---\n");
            printf("Gelen Veri        : %s\n", received_data);
            printf("Kullanilan Metod  : %s\n", method);
            printf("Gonderilen Kod    : %04X\n", sent_val);
            printf("Hesaplanan Kod    : %04X\n", computed_val);
            printf("--------------------\n");
            
            if (sent_val == computed_val) {
                printf("DURUM: VERI SAGLAM (DATA CORRECT)\n");
            } else {
                printf("DURUM: VERI BOZUK (DATA CORRUPTED)\n");
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
