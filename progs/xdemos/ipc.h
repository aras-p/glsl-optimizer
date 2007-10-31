#ifndef IPC_H
#define IPC_H


extern int MyHostName(char *nameOut, int maxNameLength);
extern int CreatePort(int *port);
extern int AcceptConnection(int socket);
extern int Connect(const char *hostname, int port);
extern void CloseSocket(int socket);
extern int SendData(int socket, const void *data, int bytes);
extern int ReceiveData(int socket, void *data, int bytes);
extern int SendString(int socket, const char *str);
extern int ReceiveString(int socket, char *str, int maxLen);


#endif /* IPC_H */
