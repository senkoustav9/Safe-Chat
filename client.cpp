#define MAX_SIZE 2048

#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <vector>

using namespace std;

vector<string> parser(string &s, char delimiter)
{
    vector<string> res;
    string curr;
    for (auto x : s)
    {
        if (x == delimiter)
        {
            res.push_back(curr);
            curr = "";
        }
        else
            curr += x;
    }
    res.push_back(curr);
    return res;
}

void execSC(string msgRead)
{
    vector<string> parsedCommand = parser(msgRead, ' ');
    if (parsedCommand.size() > 0 && (parsedCommand[0].compare("close") == 0 || parsedCommand[0].compare("\x1b[31mClose:") == 0))
        exit(0);
}

class Client
{
    int socketfd;
    struct sockaddr_in saddr;

public:
    bool constructNow(string server_address, int server_port)
    {
        socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketfd < 0)
        {
            cout << "" << endl;
            exit(EXIT_FAILURE);
        }

        memset(&saddr, '\0', sizeof(sockaddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(server_port);

        if (inet_pton(AF_INET, &server_address[0], &saddr.sin_addr) <= 0)
        {
            cout << "Address Invalid" << endl;
            exit(EXIT_FAILURE);
        }
        return true;
    }

    void _connect()
    {
        if (connect(socketfd, (sockaddr *)&saddr, sizeof(sockaddr)) < 0)
        {
            cout << "Connection Failed" << endl;
            exit(EXIT_FAILURE);
        }
    }

    string _read()
    {
        string msg = "";
        char last_character = '-';
        do
        {
            char buffer[MAX_SIZE] = {0};
            read(socketfd, buffer, 2048);
            string s(buffer);
            msg += s;
            last_character = s[s.size() - 1];
        } while (last_character != '%');

        return msg.substr(0, msg.size() - 1);
    }

    bool _send(string msg)
    {
        msg += "%";
        char buffer[msg.size() + 1];
        strcpy(buffer, &msg[0]);

        int bytes_sent_total = 0;
        int bytes_sent_now = 0;
        int message_len = msg.length();
        while (bytes_sent_total < message_len)
        {
            bytes_sent_now = send(socketfd, &buffer[bytes_sent_total], message_len - bytes_sent_total, 0);
            if (bytes_sent_now == -1)
            {
                cout << "Error in sending Message Protocol" << endl;
                return false;
            }
            bytes_sent_total += bytes_sent_now;
        }
        return true;
    }

    void _close()
    {
        close(socketfd);
    }
};

Client myclient = Client();
pthread_t recv_t, send_t;

void *receiveMsg(void *arg)
{
    while (true)
    {
        string msgRead = myclient._read();
        cout << msgRead << endl;
        execSC(msgRead);
    }
}

void *sendMsg(void *arg)
{
    while (true)
    {
        string msg;
        getline(cin, msg);
        myclient._send(msg);
        execSC(msg);
    }
}

void exit_handler(int sig)
{
    myclient._send("close");
    myclient._close();
    exit(0);
}

int main(int argc, char **argv)
{

    try
    {
        string ip_address(argv[1]);
        myclient.constructNow(ip_address, stoi(argv[2]));
    }
    catch (exception e)
    {
        cout <<"Enter in this format: ./client server_ip_address server_port\n";
        exit(0);
    }

    signal(SIGINT, exit_handler);

    myclient._connect();
    cout << "Connection Established" << endl;
    cout << "Enter Username: ";

    string user;
    getline(cin, user);
    myclient._send(user);

    pthread_create(&recv_t, NULL, receiveMsg, NULL);
    pthread_create(&send_t, NULL, sendMsg, NULL);
    pthread_join(recv_t, NULL);
    pthread_join(send_t, NULL);
    myclient._close();
    return 0;
}