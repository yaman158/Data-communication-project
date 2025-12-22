#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define LISTEN_PORT 8888    // Client 1'i dinlediði port
#define CLIENT2_PORT 8889   // Client 2'ye baðlanacaðý port
#define BUFFER_SIZE 1024

// Rastgele Karakter Ekleme Fonksiyonu
void inject_error(char* packet) {
    char data[BUFFER_SIZE];
    char rest[BUFFER_SIZE];
    char corrupted_data[BUFFER_SIZE];
    char random_char;
    int pos;

    // Paketi ayýr: DATA | METHOD...
    char* token = strchr(packet, '|');
    if (token == NULL) return; // Hatalý format

    int data_len = token - packet;
    strncpy(data, packet, data_len);
    data[data_len] = '\0';
    strcpy(rest, token); // Kalan kýsým (|CRC16|XXXX)

    // Rastgele Karakter ve Pozisyon Üret
    srand(time(NULL));
    random_char = 'A' + (rand() % 26); // A-Z arasý rastgele harf
    pos = rand() % (data_len + 1);     // Verinin herhangi bir yerine (baþýna, ortasýna, sonuna)

    // Karakteri Ekle
    strncpy(corrupted_data, data, pos);
    corrupted_data[pos] = random_char;
    corrupted_data[pos + 1] = '\0';
    strcat(corrupted_data, data + pos);

    printf("Orjinal Veri: %s\n", data);
    printf("Hata Enjekte Edildi (Insertion): %s -> %s\n", data, corrupted_data);

    // Paketi Yeniden Birleþtir
    sprintf(packet, "%s%s", corrupted_data, rest);
}

int main() {
    WSADATA wsa;
    SOCKET s_listen, s_client1, s_to_client2;
    struct sockaddr_in server, client1, client2_addr;
    int c;
    char buffer[BUFFER_SIZE];

    WSAStartup(MAKEWORD(2, 2), &wsa);

    // 1. ADIM: Client 1'den veri al (Server Modu)
    s_listen = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(LISTEN_PORT);

    bind(s_listen, (struct sockaddr*)&server, sizeof(server));
    listen(s_listen, 1);

    printf("Server dinliyor (Port: %d). Client 1 bekleniyor...\n", LISTEN_PORT);
    c = sizeof(struct sockaddr_in);
    s_client1 = accept(s_listen, (struct sockaddr*)&client1, &c);
    
    printf("Client 1 baglandi. Veri aliniyor...\n");
    int recv_size = recv(s_client1, buffer, BUFFER_SIZE, 0);
    if (recv_size > 0) {
        buffer[recv_size] = '\0';
        printf("Gelen Paket: %s\n", buffer);

        // Hata Enjeksiyonu Yap
        inject_error(buffer);
        printf("Iletilecek Bozuk Paket: %s\n", buffer);
    }
    closesocket(s_client1);
    closesocket(s_listen); // Dinlemeyi durdur

    // 2. ADIM: Client 2'ye veriyi ilet (Client Modu)
    printf("Client 2'ye baglaniliyor (Port: %d)...\n", CLIENT2_PORT);
    s_to_client2 = socket(AF_INET, SOCK_STREAM, 0);
    
    client2_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    client2_addr.sin_family = AF_INET;
    client2_addr.sin_port = htons(CLIENT2_PORT);

    // Client 2'nin hazýr olmasý için kýsa bir bekleme gerekebilir, ama döngü kullanmýyoruz.
    // Client 2'nin server.c çalýþmadan önce açýk olmasý gerekir.
    if (connect(s_to_client2, (struct sockaddr*)&client2_addr, sizeof(client2_addr)) < 0) {
        printf("Client 2'ye baglanilamadi. Lutfen once Client 2'yi calistirin.\n");
    } else {
        send(s_to_client2, buffer, strlen(buffer), 0);
        printf("Veri Client 2'ye iletildi.\n");
    }

    closesocket(s_to_client2);
    WSACleanup();
    return 0;
}
