#include "Utils/dh.hpp"
#include "Utils/rc4.hpp"
#include "Utils/prompt.hpp"
#include "Sockets/comms.hpp"
#include "Sockets/connector.hpp"
using namespace std;

mutex mtx;
queue<int> waiting_client_sockets;
unordered_map<string, int> usernames;
unordered_map<string, string> partner;
unordered_map<int, string> commonFile;
void closeSession(string username);

// Send Generator and Modulus
string sendPrimitives()
{
    return to_string(G) + " " + to_string(P) + " ";
}

// Send Status
void sendStatus(string username)
{
    vector<string> names;
    string statusMsg;
    statusMsg += RED;
    statusMsg += "\t[server]:\n";

    for (auto it : usernames)
        names.push_back(it.first);
    for (int i = 0; i < names.size(); i++)
    {
        string status = partner.find(names[i]) == partner.end() ? string(GRN) + "FREE" + string(NRM) : string(RED) + "BUSY" + string(NRM);
        statusMsg += "\t" + string(BLU) + names[i] + string(NRM) + " " + status;
        if (i != names.size() - 1)
            statusMsg += "\n";
    }
    statusMsg += NRM;
    comms comm(usernames[username]);
    comm.sendMessage(statusMsg);
    return;
}

// Connect Client
bool connectToClient(string username, vector<string> &parsedMsg, string &logFile)
{
    comms comm(usernames[username]);
    string partnerName = parsedMsg[1];
    cout << "Session request from " + username + " to " + partnerName << endl;
    if (partner.find(username) != partner.end())
    {
        string errMessage = RED;
        errMessage += "\t[server]: You are already in a chat. Please close the current chat to connect to another client.";
        errMessage += NRM;
        comm.sendMessage(errMessage);
        cout << CYN << username << RED << " connection rejected due to already existing connection." << NRM << endl;
        return false;
    }
    if (username == partnerName)
    {
        string errMessage = RED;
        errMessage += "\t[server]: To connect name can't be same as parent connection name";
        errMessage += NRM;
        comm.sendMessage(errMessage);
        cout << CYN << username << RED << " connection rejected as an user can't connect with itself" << NRM << endl;
        cout << RED << "Destination name can't be same as source Name" << NRM << endl;
        return false;
    }
    if (usernames.find(partnerName) == usernames.end())
    {
        string errMessage = RED;
        errMessage += "\t[server]: Username doesn't exist.";
        errMessage += NRM;
        comm.sendMessage(errMessage);
        cout << CYN << username << RED << " connection rejected as " << CYN << partnerName << RED << " is not a valid username." << NRM << endl;
        return false;
    }
    if (partner.find(partnerName) != partner.end())
    {
        string errMessage = RED;
        errMessage += "\t[server]: " + string(CYN) + partnerName + string(NRM) + " is BUSY. Please try after someone, or chat with someone else.";
        errMessage += NRM;
        comm.sendMessage(errMessage);
        cout << CYN << username << RED << " connection rejected as " << CYN << partnerName << RED << " is busy." << NRM << endl;
        return false;
    }
    partner[username] = partnerName;
    partner[partnerName] = username;

    commonFile[usernames[username]] = commonFile[usernames[partnerName]] = logFile;

    cout << GRN << "Connected: " + string(BLU) + username + string(NRM) + " and " << BLU << partnerName << NRM << endl;

    // Notify the clients about their successful connection:
    string msg = GRN;
    msg += "\t[server]: You are now connected to ";
    comm.sendMessage(msg + partnerName + NRM);
    comms comm1(usernames[partnerName]);
    comm1.sendMessage(msg + username + NRM);

    // Exchange keys
    string handshake = "handshake";
    comm.sendMessage(handshake);
    comm1.sendMessage(handshake);

    return true;
}

// Function to Handle a Given Client
void *handle_client(void *arg)
{
    mtx.lock();
    int client_socket = waiting_client_sockets.front();
    waiting_client_sockets.pop();
    mtx.unlock();

    comms comm(client_socket);
    comm.sendMessage(sendPrimitives());
    string username = comm.receive();

    if (usernames.count(username) > 0)
    {
        comm.sendMessage("Username already exists.\n");
        comm.disconnect();
        return NULL;
    }
    usernames[username] = client_socket;

    comm.sendMessage("Username accepted.\n");
    cout << GRN << username << " connected." << NRM << endl;

    string msg = "";
    string logFile;
    while (true)
    {
        msg = comm.receive();
        if (msg.size() == 0)
            continue;
        vector<string> parsedMsg = parseTheString(msg, ' ');
        if (parsedMsg[0] == "status")
            sendStatus(username);
        else if (parsedMsg[0] == "connect")
        {
            connectToClient(username, parsedMsg, logFile);
        }
        else if (parsedMsg[0] == "key")
        {
            string A = parsedMsg[1];
            cout << A << endl;
            comms comm1(usernames[partner[username]]);
            comm1.sendMessage("keys");
            comm1.sendMessage(A);
        }
        else if (parsedMsg[0] == "close")
        {
            closeSession(username);
            break;
        }
        else if (partner.find(username) != partner.end())
        {
            string s;
            s += "\t";
            s += "[" + username + "] $" + msg;
            comms comm1(usernames[partner[username]]);
            cout << s << endl;
            comm1.sendMessage(s);

            if (msg == "goodbye")
            {
                cout << username << " said" << msg << endl;
                closeSession(username);
            }
        }
        else
        {
            string s = RED;
            s += "\t[server]: Invalid Command. Please refer to the readme for commands.";
            s += NRM;
            comm.sendMessage(s);
        }
    }
    usernames.erase(username);
    comm.disconnect();
    cout << RED << username << " disconnected." << NRM << endl;
    return NULL;
}

// Closes the session
void closeSession(string username)
{
    if (partner.find(username) == partner.end())
    {
        comms comm(usernames[username]);
        string errMessage = RED;
        errMessage += "\t[server]: You are not connected to anyone.";
        errMessage += NRM;
        comm.sendMessage(errMessage);
        cout << RED << "Need to connect to someone to close the connection." << NRM << endl;
        return;
    }

    string partnerName = partner[username];
    string msg = GRN;
    msg += "\n\t[server]: " + username + " closed the chat session" + NRM;
    comms comm1(usernames[partnerName]);
    comm1.sendMessage(msg);

    cout << msg;
    if (partner.find(partnerName) != partner.end())
        partner.erase(partnerName);

    if (partner.find(username) != partner.end())
        partner.erase(username);

    msg = "";
    msg += CYN;
    msg += "\t[server]: Session closed";
    msg += NRM;

    comms comm(usernames[username]);
    comm.sendMessage(msg);

    sleep(0.1);
    string message = "goodbye";
    comm.sendMessage(message);
    comm1.sendMessage(message);

    cout << "Sent goodbyes to both\n";
    if (commonFile.find(usernames[username]) != commonFile.end())
        commonFile.erase(usernames[username]);

    if (commonFile.find(usernames[partnerName]) != commonFile.end())
        commonFile.erase(usernames[partnerName]);

    cout << "Disconnected " + string(CYN) + username + string(NRM) + " and " + string(CYN) + partnerName << NRM << endl;
}

// Exit Handler
void exit_handler(int sig)
{
    cout << "\nShutting the server down.. \n and disconnecting the clients..\n";
    for (auto it : usernames)
    {
        comms comm(it.second);
        comm.sendMessage("close");
        comm.disconnect();
    }
    exit(0);
    return;
}

// Main Function
int main(int argc, char **argv)
{
    srand(time(NULL));
    generatePrimitiveKeys();
    if (argc > 1)
    {
        err("Too many arguments.");
    }
    connector server(1, PORT);
    server.listenForClients();

    // Singal Handling:
    signal(SIGINT, exit_handler);
    cout << GRN << "Server is up and running" << NRM << endl;
    cout << GRN << "G: " << to_string(G) << endl;
    cout << GRN << "P: " << to_string(P) << endl;

    // Read the Primitives
    ofstream oF("./Headers/primitives.txt");
    if (!oF)
    {
        cerr << "Failed to open the file!" << endl;
        return 1;
    }
    oF << to_string(G) << endl;
    oF << to_string(P) << endl;
    oF.close();

    while (true)
    {
        int client_socket = server.acceptNow();
        waiting_client_sockets.push(client_socket);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, NULL);
    }
    return 0;
}