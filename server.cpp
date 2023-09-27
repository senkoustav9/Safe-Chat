#define MAX_SIZE 2048

#include <bits/stdc++.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <mutex>

using namespace std;
void *handleClient(void *arg);
bool connectToOtherClient(string my_client_name, vector<string> parsedMsg);
void sendCurrentStatus(string my_client_name);
vector<string> splitWord(string &s, char delimiter);
string getGeneralInstructions(string my_client_name);
void closeTheSession(string my_client_name);

class Server
{
  int socketfd;
  struct sockaddr_in sin;
  int sin_len = sizeof(sin);
  int port = 0;

public:
  bool _construct(int server_port)
  {
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
    {
      cout << "Error creating the socket" << endl;
      exit(EXIT_FAILURE);
    }
    memset(&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(server_port);

    if (bind(socketfd, (sockaddr *)&sin, sizeof(sin)) < 0)
    {
      cout << "Error Binding to the address" << endl;
      exit(EXIT_FAILURE);
    }
    port = server_port;
    return true;
  }

  void _listen()
  {
    if (listen(socketfd, 10) < 0)
    {
      cout << "Error Listening" << endl;
      exit(EXIT_FAILURE);
    }
    cout << "Server Listening on port: " << port << endl;
  }

  int _accept()
  {
    int new_socket;
    if ((new_socket = accept(socketfd, (sockaddr *)&sin, (socklen_t *)&sin_len)) < 0)
    {
      cout << "Error Accepting" << endl;
      exit(EXIT_FAILURE);
    }
    return new_socket;
  }

  string readMsg(int client_socket)
  {
    string msg = "";
    char last_character = '-';
    do
    {
      char buffer[MAX_SIZE] = {0};
      read(client_socket, buffer, MAX_SIZE);
      string s(buffer);
      msg += s;
      last_character = s[s.size() - 1];
    } while (last_character != '%');

    return msg.substr(0, msg.size() - 1);
  }

  bool sendMsg(int client_socket, string msg)
  {
    msg = msg + "%";
    char buffer[msg.size()];
    strcpy(buffer, &msg[0]);

    int bytes_sent_total = 0;
    int bytes_sent_now = 0;
    int message_len = msg.length();
    while (bytes_sent_total < message_len)
    {
      bytes_sent_now = send(client_socket, &buffer[bytes_sent_total], message_len - bytes_sent_total, 0);
      if (bytes_sent_now == -1)
      {
        cout << "Error in Server Protocol" << endl;
        return false;
      }
      bytes_sent_total += bytes_sent_now;
    }
    return true;
  }

  void _close(int client_socket)
  {
    close(client_socket);
  }
};

Server myserver = Server();
queue<int> waitingClient;
unordered_map<string, string> partnerClient;
unordered_map<string, int> sock;
mutex mtx;

void *handleClient(void *arg)
{
  mtx.lock();
  int client_socket = waitingClient.front();
  waitingClient.pop();
  mtx.unlock();
  string client_name = myserver.readMsg(client_socket);

  if (sock.find(client_name) != sock.end())
  {
    string errMsg = "";
    errMsg += "Close: There is already a client with that name...\nDisconnecting...\n";
    errMsg += "";
    myserver.sendMsg(client_socket, errMsg);
    cout << "Entered client_name already exists in the database..." << endl;
    return NULL;
  }

  sock[client_name] = client_socket;

  cout << "Client ID: " << client_name << " service started" << endl;
  myserver.sendMsg(client_socket, getGeneralInstructions(client_name));

  string req = "";
  while (true)
  {
    req = myserver.readMsg(client_socket);
    vector<string> parsedMsg = splitWord(req, ' ');

    if (parsedMsg[0] == "status")
      sendCurrentStatus(client_name);
    else if (parsedMsg[0] == "connect")
      connectToOtherClient(client_name, parsedMsg);
    else if (parsedMsg[0] == "close")
    {
      closeTheSession(client_name);
      break;
    }
    else if (partnerClient.find(client_name) != partnerClient.end())
    {
      string msg = "";
      msg += "(" + client_name + "): " + req;
      myserver.sendMsg(sock[partnerClient[client_name]], msg);
      if (req == "goodbye")
        closeTheSession(client_name);
      continue;
    }
    else
    {
      string errMsg = "";
      errMsg += "(server): Command Not Found";
      errMsg += "";
      myserver.sendMsg(client_socket, errMsg);
    }
  }
  sock.erase(client_name);
  myserver._close(client_socket);
  cout << "Client ID: " << client_name << " disconnected.." << endl;
  pthread_exit(NULL);
}

void sendCurrentStatus(string my_client_name)
{
  vector<string> names;
  string statusMessage;
  statusMessage += "";
  statusMessage += "(server):\n";

  for (auto it : sock)
    names.push_back(it.first);

  for (int i = 0; i < names.size() - 1; i++)
  {
    string hasPartner = partnerClient.find(names[i]) == partnerClient.end() ? "FREE" : "BUSY";
    statusMessage += "" + names[i] + " " + hasPartner + "\n";
  }

  string hasPartner = partnerClient.find(names[names.size() - 1]) == partnerClient.end() ? "FREE" : "BUSY";
  statusMessage += "" + names[names.size() - 1] + " " + hasPartner + "\n";

  statusMessage += "";
  myserver.sendMsg(sock[my_client_name], statusMessage);
  return;
}

bool connectToOtherClient(string my_client_name, vector<string> parsedMsg)
{
  string otherClientName = parsedMsg[1];
  cout << "Session request from " + my_client_name + " to " + otherClientName << endl;

  if (partnerClient.find(my_client_name) != partnerClient.end())
  {
    string errMsg = "";
    errMsg += "(server): You are already connected to someone.";
    errMsg += "";
    myserver.sendMsg(sock[my_client_name], errMsg);
    cout << "Already connected, connect request rejected" << endl;
    return false;
  }

  if (my_client_name.compare(otherClientName) == 0)
  {
    string errMsg = "";
    errMsg += "(server): Destination name can't be same as source Name";
    errMsg += "";
    myserver.sendMsg(sock[my_client_name], errMsg);
    cout << "Destination name can't be same as source Name" << endl;
    return false;
  }

  if (sock.find(otherClientName) == sock.end())
  {
    string errMsg = "";
    errMsg += "(server): No client named: " + otherClientName;
    errMsg += "";
    myserver.sendMsg(sock[my_client_name], errMsg);
    cout << "Couldn't connect: No client named: " << otherClientName << endl;
    return false;
  }

  if (partnerClient.find(otherClientName) != partnerClient.end())
  {
    string errMsg = "";
    errMsg += "(server): " + otherClientName + " is BUSY";
    errMsg += "";
    myserver.sendMsg(sock[my_client_name], "" + errMsg);
    cout << "Couldn't connect: client " + otherClientName + " is busy" << endl;
    return false;
  }

  partnerClient[my_client_name] = otherClientName;
  partnerClient[otherClientName] = my_client_name;

  cout << "Connected: " + my_client_name << " and " << otherClientName << endl;
  string successMsg = "";
  successMsg += "(server): You are now connected to ";
  myserver.sendMsg(sock[otherClientName], successMsg + my_client_name + "");
  myserver.sendMsg(sock[my_client_name], successMsg + otherClientName + "");
  return true;
}

void closeTheSession(string my_client_name)
{
  if (partnerClient.find(my_client_name) == partnerClient.end())
    return;
  string otherClientName = partnerClient[my_client_name];
  string msg = "";
  msg += "\n(server): " + my_client_name + " closed the chat session";
  msg += "";
  myserver.sendMsg(sock[otherClientName], msg);

  if (partnerClient.find(otherClientName) != partnerClient.end())
    partnerClient.erase(otherClientName);

  if (partnerClient.find(my_client_name) != partnerClient.end())
    partnerClient.erase(my_client_name);

  msg = "";
  msg += "";
  msg += "(server): Session closed";
  msg += "";
  myserver.sendMsg(sock[my_client_name], msg);

  cout << "Disconnected " + my_client_name + " and " + otherClientName << endl;
}

string getGeneralInstructions(string my_client_name)
{
  string msg = "";
  msg += "\n(server): Hi, " + my_client_name + "\n";
  msg += "\tThe supported commands are:\n";
  msg += "\t1. status             : List all the users status\n";
  msg += "\t2. connect <username> : Connect to username and start chatting\n";
  msg += "\t3. goodbye            : Ends current chatting session\n";
  msg += "\t4. close              : Disconnects you from the server\n";
  msg += "";
  return msg;
}

vector<string> splitWord(string &s, char delimiter)
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

void exit_handler(int sig)
{
  cout << "\nShutting down the server...\nDisconnecting Clients...\n";
  for (auto it : sock)
  {
    myserver.sendMsg(it.second, "close");
    myserver._close(it.second);
  }
  exit(0);
  return;
}

int main(int argc, char **argv)
{
  try
  {
    myserver._construct(stoi(argv[1]));
  }
  catch (exception e)
  {
    cout << "Please provide command in the following format:\n";
    cout << "./server port\n";
    exit(0);
  }
  signal(SIGINT, exit_handler);
  myserver._listen();
  cout << "Server started successfully..." << endl;
  while (true)
  {
    int client_socket = myserver._accept();
    waitingClient.push(client_socket);
    pthread_t tid;
    pthread_create(&tid, NULL, handleClient, NULL);
  }
  return 0;
}