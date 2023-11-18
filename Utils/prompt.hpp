#include "../Headers/builtin.hpp"
#include "../Headers/colors.hpp"
using namespace std;

string readme(string username) {
  string s = CYN;
  s += "\n+------------------------------------------------------------+\n";
  s += "|                                                            |\n";
  s += "|   [server]: Welcome to the chatroom, " + username +
       string(22 - username.length(), ' ') + "|\n";
  s += "|                                                            |\n";
  s += "|   [server]: Here are the commands you can use:             |\n";
  s += "|                                                            |\n";
  s += "|   1. status             : Lists the status of all users    |\n";
  s += "|   2. connect [username] : Connect to username to start     |\n";
  s += "|                           chatting                         |\n";
  s += "|   3. goodbye            : Ends current chatting session    |\n";
  s += "|   4. close              : Disconnects the user from the    |\n";
  s += "|                           server                           |\n";
  s += "|   5. clear              : Clears the chat from the window  |\n";
  s += "|   6. Ctrl + C (client)  : Disconnects the client and       |\n";
  s += "|                           terminates its chat session      |\n";
  s += "|                           if present                       |\n";
  s += "|   7. Ctrl + C (server)  : Terminates the server and all the|\n";
  s += "|                           clients connected to the server  |\n";
  s += "+------------------------------------------------------------+\n";
  s += NRM;
  return s;
}