# Safe Chat

Current Implementation:
It is a multi-threaded LAN Chat Server in Cpp. TCP Sockets have been used for communication between clients. 
The application runs on a command line interface (CLI) and supports many functionalities like
1. Displaying the current status of the active users whether they are busy or free.
2. The server creates a separate chat room for a pair of clients allowing them to chat.
3. If a user logs out or his connection is dead, the thread corresponding to that user is released and other user is made free.

To Do:
1. Implementing encryption using Cpp library to establish tamper-proof communication between clients.
