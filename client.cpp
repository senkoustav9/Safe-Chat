#include "Utils/dh.hpp"
#include "Utils/rc4.hpp"
#include "Utils/prompt.hpp"
#include "Sockets/comms.hpp"
#include "Sockets/connector.hpp"
using namespace std;

int sockfd;
ll privateKey;
ll secretKey;
string username;
bool secure;

// Remove Username
string removeUsername(string msg) {
  string name;
  int i = 0;
  while (i < msg.length() && msg[i] != '$') {
    name.push_back(msg[i]);
    i++;
  }
  i++;
  if (i >= msg.length()) {
    string ex = msg;
    rc4_crypt(ex, to_string(secretKey));
    return ex;
  }
  string newMsg = msg.substr(i);
  cout << name << ": ";
  return newMsg;
}

// Receive Message
void *receiveMsg(void *arg) {
  int sockfd = *((int *)arg);
  while (true) {
    comms comm(sockfd);
    string msg = comm.receive();
    if (secure) {
      msg = removeUsername(msg);
      rc4_crypt(msg, to_string(secretKey));
    }
    if (msg == "close") {
      cout << "Server closed the connection." << endl;
      exit(0);
    } else if (msg == "goodbye") {
      secure = false;
      secretKey = 0;
      continue;
    } else if (msg == "handshake") {
      privateKey = getPrivateKey();
      comm.sendMessage("key " + to_string(createPublicKey(privateKey)));
      continue;
    } else if (msg == "keys") {
      string B = comm.receive();
      secretKey = createSecretKey(stoll(B), privateKey);
      secure = true;
      continue;
    }
    cout << msg << endl;
  }
}

// Send Message
void *sendMsg(void *arg) {
  int sockfd = *((int *)arg);
  while (true) {
    string msg;
    getline(cin, msg);
    string prompt = "     [" + username + "]: ";
    if (msg.size())
      cout << GOUP << left << CYN << prompt << MAG << msg << NRM << endl;
    if (msg == "clear") {
      cout << CLR;
      continue;
    }
    comms comm(sockfd);
    if (secure) {
      if (msg != "goodbye" && msg != "close")
        rc4_crypt(msg, to_string(secretKey));
    }
    comm.sendMessage(msg);
    sleep(0.1);
    if (msg == "close")
      exit(0);
  }
}

// Exit Handler
void exit_handler(int sig) {
  comms comm(sockfd);
  comm.sendMessage("close");
  comm.disconnect();
  exit(0);
}

// Get Public Keys
void getPublicKeys(string s) {
  string strG, strP;
  int i = 0;
  while (i < s.length() && s[i] != ' ') {
    strG.push_back(s[i]);
    i++;
  }
  i++;
  while (i < s.length() && s[i] != ' ') {
    strP.push_back(s[i]);
    i++;
  }
  setPrimitiveKeys(stoll(strG), stoll(strP));
}

// Main Function
int main(int argc, char **argv) {
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
  comm.sendMessage(username);
  cout << readme(username);
  pthread_create(&receive_t, NULL, receiveMsg, &sockfd);
  pthread_create(&send_t, NULL, sendMsg, &sockfd);
  pthread_join(receive_t, NULL);
  pthread_join(send_t, NULL);
  comm.disconnect();
  return 0;
}