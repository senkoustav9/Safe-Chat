#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cassert>
#include <pthread.h>
#include <signal.h>
#include <vector>
#include "dh.hpp"
#include "rc4.hpp"

using namespace std;

#define PORT 8083
#define MAX_SIZE 20480
#define NRM "\x1B[0m"
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define GOUP "\033[A\r"
#define CLR "\e[2J\e[H"

// Global Variables:
int sockfd;
ll privateKey;
ll secretKey;
string username;
bool secure;

void err(string s)
{
    perror(s.c_str());
    exit(1);
}
class connector
{
    int sockfd;
    struct sockaddr_in saddr;

public:
    connector(bool s_c, uint16_t port, string addr = "") // s_c is 1 for Server and 0 for Client
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (!~sockfd)
            err("Can't create socket.");
        memset(&saddr, 0, sizeof(sockaddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(port);
        if (s_c)
        {
            saddr.sin_addr.s_addr = htonl(INADDR_ANY);
            int reuse = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
                perror("setsockopt(SO_REUSEADDR) failed");
#ifdef SO_REUSEPORT
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse)) < 0)
                perror("setsockopt(SO_REUSEPORT) failed");
#endif
            if (!~bind(sockfd, (sockaddr *)&saddr, sizeof saddr))
                err("Can't bind the socket.");
        }
        else if (!~inet_pton(AF_INET, addr.c_str(), &saddr.sin_addr))
            err("Wrong address format.");
    }
    // Client side functions:
    int connectToServer()
    {
        if (!~connect(sockfd, (sockaddr *)&saddr, sizeof saddr))
            err("Cannot connect.");
        return sockfd;
    }
    // Server side functions:
    void listenForClients()
    {
        if (!~listen(sockfd, 10))
            err("Cannot listen.");
        cout << "Waiting for Connections on port: " << PORT << endl;
    }
    int acceptNow()
    {
        int new_fd;
        socklen_t a = sizeof(saddr);
        if (!~(new_fd = accept(sockfd, (sockaddr *)&saddr, (socklen_t *)&a)))
            err("Cannot accept.");
        return new_fd;
    }
    void closeServer()
    {
        close(sockfd);
    }
};
class comms
{
    int sockfd;

public:
    comms(int fd)
    {
        sockfd = fd;
    }
    string receive()
    {
        string msg = "";
        char last_character = '%';
        do
        {
            char buffer[2048] = {0};
            read(sockfd, buffer, 2048);
            string s(buffer);
            msg += s;
            last_character = s[s.size() - 1];
        } while (last_character != '|');
        return msg.substr(0, msg.size() - 1);
    }
    bool sendMsg(string s)
    {
        s += '|';
        char buffer[s.size()];
        strcpy(buffer, &s[0]);
        int bytes_total = 0;
        int bytes_left = s.length();
        int bytes_now = 0;
        while (bytes_left > 0)
        {
            bytes_now = send(sockfd, &buffer[bytes_total], bytes_left, 0);
            if (bytes_now == -1)
            {
                err("Error sending message");
                return false;
            }
            assert(bytes_now <= bytes_left);
            bytes_left -= bytes_now;
            bytes_total += bytes_now;
        }

        return true;
    }
    void disconnect()
    {
        close(sockfd);
    }
};

string removeUsername(string msg)
{
    string name;
    int i = 0;
    while (i < msg.length() && msg[i] != '$')
    {
        name.push_back(msg[i]);
        i++;
    }
    i++;
    if (i >= msg.length())
    {
        string ex = msg;
        rc4_crypt(ex, to_string(secretKey));
        return ex;
    }
    string newMsg = msg.substr(i);
    cout << name << ": ";
    return newMsg;
}

void *receiveMsg(void *arg)
{
    int sockfd = *((int *)arg);
    while (true)
    {
        comms comm(sockfd);
        string msg = comm.receive();
        if (secure)
        {
            msg = removeUsername(msg);
            rc4_crypt(msg, to_string(secretKey));
        }

        if (msg == "close")
        {
            cout << "Server closed the connection." << endl;
            exit(0);
        }
        else if (msg == "goodbye")
        {
            secure = false;
            secretKey = 0;
            continue;
        }
        else if (msg == "handshake")
        {
            privateKey = getPrivateKey();
            comm.sendMsg("key " + to_string(createA(privateKey)));
            continue;
        }
        else if (msg == "keys")
        {
            string B = comm.receive();
            // cout << B << endl;
            secretKey = createSecretKey(stoll(B), privateKey);
            // cout << secretKey << endl;
            secure = true;
            continue;
        }
        cout << msg << endl;
    }
}
void *sendMsg(void *arg)
{
    int sockfd = *((int *)arg);
    while (true)
    {
        string msg;
        getline(cin, msg);
        string prompt = "        [" + username + "]: ";
        if (msg.size())
            cout << GOUP << left << CYN << prompt << MAG << msg << NRM << endl;
        if (msg == "clear")
        {
            cout << CLR;
            continue;
        }
        comms comm(sockfd);
        if (secure)
        {
            if (msg != "goodbye" && msg != "close")
                rc4_crypt(msg, to_string(secretKey));
        }
        comm.sendMsg(msg);
        sleep(0.1);
        if (msg == "close")
            exit(0);
    }
}

void exit_handler(int sig)
{
    comms comm(sockfd);
    comm.sendMsg("close");
    comm.disconnect();
    exit(0);
}

void getPublicKeys(string s)
{
    string strG, strP;
    int i = 0;
    while (i < s.length() && s[i] != ' ')
    {
        strG.push_back(s[i]);
        i++;
    }
    i++;
    while (i < s.length() && s[i] != ' ')
    {
        strP.push_back(s[i]);
        i++;
    }
    setPublicKeys(stoll(strG), stoll(strP));
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    pthread_t receive_t, send_t;
    string ip_address = "local";

    connector client(0, PORT, ip_address);

    // Singal Handling:
    signal(SIGINT, exit_handler);

    // Client side:
    sockfd = client.connectToServer();
    cout << "Connected to server." << endl;
    comms comm(sockfd);
    getPublicKeys(comm.receive());
    cout << "Enter your username: ";
    cin >> username;
    comm.sendMsg(username);
    pthread_create(&receive_t, NULL, receiveMsg, &sockfd);
    pthread_create(&send_t, NULL, sendMsg, &sockfd);
    pthread_join(receive_t, NULL);
    pthread_join(send_t, NULL);

    comm.disconnect();
    return 0;
}