#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

#define SIZE 1024
#define ANSWER_TIME 60
int chat_socket = -1;

struct ad_data
{
    int port;
    char name[SIZE];
    char seller_name[SIZE];
    char state[25];
};

void time_handler()
{
    char message[10];
    send(chat_socket, message, strlen(message), 0);
    close(chat_socket);
    chat_socket = -1;
    write(1, "ERROR! You only have 1 minute to answer.\n", 41);
}

void checkMessages(char *buff, int size)
{
    char *message = strtok(buff, "/");
    char *message2 = strtok(NULL, "/");
    if (strcmp(message, "accept") == 0 || strcmp(message, "reject") == 0)
    {
        write(1, message2, strlen(message2));
        chat_socket = -1;
        return;
    }
    write(1, "Server said : ", 14);
    write(1, message2, strlen(message2));
}

struct ad_data seperate_data(char *all_data)
{
    struct ad_data data;
    char *words = strtok(all_data, "\n");
    int port = atoi(words);
    data.port = port;
    words = strtok(NULL, "\n");
    strcpy(data.name, words);
    words = strtok(NULL, "\n");
    strcpy(data.seller_name, words);
    words = strtok(NULL, "\n");
    strcpy(data.state, words);
    return data;
}

int connectServer(int port)
{
    int fd;
    struct sockaddr_in server_address;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    { // checking for errors
        printf("Error in connecting to server\n");
    }

    return fd;
}

int main(int argc, char *argv[])
{
    int max_sd;
    char buff[SIZE] = {0};
    char buyer_name[SIZE];
    fd_set master_set, working_set;
    signal(SIGALRM, time_handler);
    siginterrupt(SIGALRM, 1);
    if (argc != 2)
    {
        sprintf(buff, "ERROR, You haven't entered your port!\n");
        write(1, buff, strlen(buff));
        exit(1);
    }

    sprintf(buff, "You successfully entered!\nPlease Enter your name: ");
    write(1, buff, strlen(buff));
    read(0, buyer_name, SIZE);

    int broadcast_socket, broadcast = 1, opt = 1;
    struct sockaddr_in bc_address;

    broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(broadcast_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(broadcast_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(atoi(argv[1]));
    bc_address.sin_addr.s_addr = inet_addr("192.168.1.255");

    bind(broadcast_socket, (struct sockaddr *)&bc_address, sizeof(bc_address));

    FD_ZERO(&master_set);
    max_sd = broadcast_socket;
    FD_SET(broadcast_socket, &master_set);
    FD_SET(0, &master_set); // for stdin

    struct ad_data data[200];
    int size = 0;
    bool isChatting = false;

    write(1, "\tshow_list\n\texit\n\tselect ads_name\n\tchat message\n\toffer price\n\texit chat\n", 72);
    while (1)
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);
        for (int i = 0; i <= max_sd; i++)
        {
            if (FD_ISSET(i, &working_set))
            {
                if (i == 0)
                {
                    memset(buff, 0, SIZE);
                    char names[SIZE + 10];
                    char seller_name[SIZE + 16];
                    char states[SIZE + 10];
                    char ports[SIZE + 10];
                    read(0, buff, SIZE);
                    char *order = strtok(buff, " ");
                    if (strcmp(order, "show_list\n") == 0) // show list
                    {
                        if (size == 0)
                        {
                            write(1, "The menu is empty!\n", 19);
                        }
                        for (int j = 0; j < size; j++)
                        {
                            if (strcmp(data[j].state, "Expired") != 0)
                            {
                                sprintf(names, "id: %d\n", j );
                                write(1, names, strlen(names));

                                sprintf(names, "name: %s\n", data[j].name);
                                write(1, names, strlen(names));

                                sprintf(seller_name, "seller name: %s\n", data[j].seller_name);
                                write(1, seller_name, strlen(seller_name));

                                sprintf(ports, "ports: %d\n", data[j].port);
                                write(1, ports, strlen(ports));

                                sprintf(states, "state: %s\n", data[j].state);
                                write(1, states, strlen(states));
                                write(1, "********************\n", 22);
                            }
                        }
                    }
                    else if (strcmp(order, "exit\n") == 0)
                    {
                        exit(1);
                    }
                    else if (strcmp(order, "chat") == 0)
                    {
                        if (chat_socket == -1)
                        {
                            write(1, "No advertisement selected!\n", 27);
                            write(1, "\tshow_list\n\texit\n\tselect ads_name\n\tchat message\n\toffer price\n\texit chat\n", 72);
                            continue;
                        }
                        char *msg = strtok(NULL, " ");
                        char message[SIZE];
                        strcpy(message, "chat/");
                        strcpy(message + 5, msg);
                        send(chat_socket, message, strlen(message), 0);
                        alarm(ANSWER_TIME);
                    }
                    else if (strcmp(order, "select") == 0)
                    {
                        char *id = strtok(NULL, " ");
                        int idNum = atoi(id);
                        bool checkifExist = false;
                        if (strcmp(data[idNum].state, "Waiting") == 0)
                        {
                            chat_socket = connectServer(data[idNum].port);
                            FD_SET(chat_socket, &master_set);
                            strcpy(data[idNum].state, "Discuss");
                            checkifExist = true;
                            if (chat_socket > max_sd)
                                max_sd = chat_socket;
                            alarm(ANSWER_TIME);
                        }
                        else if (strcmp(data[idNum].state, "Discuss") == 0)
                        {
                            write(1, "This advertisement is in discuss state.\n", 40);
                        }
                        else if(checkifExist == false)
                            write(1, "This id doesn't exist.\n", 23);
                    }
                    else if (strcmp(order, "offer") == 0)
                    {
                        if (chat_socket == -1)
                        {
                            write(1, "No advertisement selected!\n", 27);
                            write(1, "\tshow_list\n\texit\n\tselect ads_name\n\tchat message\n\toffer price\n\texit chat\n", 72);
                            continue;
                        }
                        char *msg = strtok(NULL, " ");
                        char message[SIZE];
                        strcpy(message, "offer/");
                        strcpy(message + 6, msg);
                        send(chat_socket, message, strlen(message), 0);
                        alarm(ANSWER_TIME);
                    }
                    else
                    {
                        write(1, "Your command is invalid!\n", 25);
                    }
                    write(1, "\tshow_list\n\texit\n\tselect ads_name\n\tchat message\n\toffer price\n\texit chat\n", 72);
                }
                else if (i == chat_socket)
                {
                    int bytes_received;
                    memset(buff, 0, SIZE);
                    bytes_received = recv(i, buff, SIZE, 0);

                    if (bytes_received == 0)
                    { // EOF
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    checkMessages(buff, strlen(buff));
                    alarm(ANSWER_TIME);
                }
                else if (i == broadcast_socket)
                {
                    struct ad_data new_data;
                    bool isNew = true;
                    char buffer[SIZE];
                    recv(i, buffer, SIZE, 0);
                    write(1, "List updated! enter show_list to see the updated menu.\n", 55);
                    new_data = seperate_data(buffer);
                    for (int j = 0; j < size; j++)
                    {
                        if (strcmp(new_data.name, data[j].name) == 0)
                        {
                            data[j] = new_data;
                            isNew = false;
                            break;
                        }
                    }
                    if (isNew)
                    {
                        data[size] = new_data;
                        size++;
                    }
                }
            }
        }
    }
}