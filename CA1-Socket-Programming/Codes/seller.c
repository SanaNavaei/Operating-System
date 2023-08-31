#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define SIZE 1024

struct Advertisement
{
    int port;
    int cost;
    char state[25];
    char name[SIZE];
    int socket;
};
struct Advertisement ads[100];
struct sockaddr_in bc_address;
int sock;
char seller_name[SIZE];

bool checkMessages(char *buff, int size, int index)
{
    bool isOffer = false;
    char *message = strtok(buff, "/");
    char *message2 = strtok(NULL, "/");
    if (strcmp(message, "offer") == 0)
        ads[index].cost = atoi(message2);
    write(1, message2, strlen(message2));
}

int acceptClient(int server_fd)
{
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&address_len);

    return client_fd;
}

int setupServer(int port)
{
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    listen(server_fd, 4);

    return server_fd;
}

int main(int argc, char *argv[])
{
    int max_sd, new_socket;
    char buff[SIZE] = {0};
    char seller_ad[SIZE];
    int servers[SIZE], clients[SIZE];
    fd_set master_set, working_set, waiting_set;

    if (argc != 2)
    {
        sprintf(buff, "ERROR, Server is not running!\n");
        write(1, buff, strlen(buff));
        exit(1);
    }

    sprintf(buff, "Server is running!\nPlease Enter your name: ");
    write(1, buff, strlen(buff));
    read(0, seller_name, SIZE);

    int ads_count = 0;

    int broadcast = 1, opt = 1;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(atoi(argv[1]));
    bc_address.sin_addr.s_addr = inet_addr("192.168.1.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

    FD_ZERO(&master_set);
    FD_ZERO(&waiting_set);
    max_sd = sock;
    FD_SET(0, &master_set); // for stdin
    bool isChatting = false;
    char notif[SIZE];

    write(1, "\tadd_advertisement\n\tshow_list\n\taccept id\n\trejct id\n\tchat id message\n\texit\n", 74);
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
                    read(0, buff, SIZE);
                    char *order = strtok(buff, " ");
                    if (strcmp(order, "add_advertisement\n") == 0)
                    {
                        int inDiscuss = 0;
                        for (int j = 0; j < ads_count; j++)
                        {
                            if (strcmp(ads[j].state, "Discuss") == 0)
                            {
                                write(1, "You are chatting with client so you can not add advertisement.\n", 63);
                                inDiscuss = 1;
                                break;
                            }
                        }
                        if (inDiscuss == 0)
                        {
                            char port[10];
                            strcpy(ads[ads_count].state, "Waiting");

                            sprintf(buff, "Enter your advertisement: ");
                            write(1, buff, strlen(buff));
                            read(0, ads[ads_count].name, SIZE);

                            sprintf(buff, "Enter your port: ");
                            memset(port, 0, SIZE);
                            write(1, buff, strlen(buff));
                            read(0, port, SIZE);
                            ads[ads_count].port = atoi(port);

                            char broadcast_buffer[SIZE * 4 + 6];
                            sprintf(broadcast_buffer, "%d\n%s\n%s\n%s\n", ads[ads_count].port, ads[ads_count].name, seller_name, ads[ads_count].state);
                            int a = sendto(sock, broadcast_buffer, strlen(broadcast_buffer), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));

                            servers[ads_count] = setupServer(ads[ads_count].port);
                            FD_SET(servers[ads_count], &master_set);
                            if (servers[ads_count] > max_sd)
                                max_sd = servers[ads_count];
                            FD_SET(servers[ads_count], &waiting_set);
                            ads[ads_count].socket = servers[ads_count];

                            ads_count++;
                        }
                    }
                    else if (strcmp(order, "show_list\n") == 0)
                    {
                        char names[SIZE + 10];
                        char states[SIZE + 10];
                        char ports[SIZE + 10];
                        if (ads_count == 0)
                            write(1, "The menu is empty!\n", 20);
                        for (int j = 0; j < ads_count; j++)
                        {
                            sprintf(names, "id: %d\n", j);
                            write(1, names, strlen(names));

                            sprintf(names, "name: %s", ads[j].name);
                            write(1, names, strlen(names));

                            sprintf(ports, "ports: %d\n", ads[j].port);
                            write(1, ports, strlen(ports));

                            sprintf(ports, "cost: %d\n", ads[j].cost);
                            write(1, ports, strlen(ports));

                            sprintf(states, "state: %s\n", ads[j].state);
                            write(1, states, strlen(states));
                            write(1, "********************\n", 22);
                        }
                    }
                    else if (strcmp(order, "accept") == 0)
                    {
                        char *id = strtok(NULL, " ");
                        int intNum = atoi(id);
                        int check = 0;
                        if (intNum > ads_count)
                        {
                            write(1, "ERROR! You don't have this id of advertisement.\n", 48);
                            check = 1;
                        }
                        else if (strcmp(ads[intNum].state, "Discuss") == 0)
                        {
                            strcpy(ads[intNum].state, "Expired");
                            char broadcast_buffer[SIZE * 4 + 6], message[SIZE], msg[SIZE + 30];
                            sprintf(broadcast_buffer, "%d\n%s\n%s\n%s\n", ads[intNum].port, ads[intNum].name, seller_name, ads[intNum].state);
                            int a = sendto(sock, broadcast_buffer, strlen(broadcast_buffer), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                            sprintf(buff, "The advertisement is expired now!\n");
                            write(1, buff, strlen(buff));
                            strcpy(message, "accept/");
                            strcpy(message + 7, buff);
                            send(ads[intNum].socket, message, strlen(message), 0);
                            char file_name[SIZE + 10];
                            strcpy(file_name, seller_name);
                            file_name[strlen(file_name) - 1] = '\0';
                            strcat(file_name, ".txt");
                            check = 1;
                            int fd = open(file_name, O_WRONLY | O_APPEND | O_CREAT);
                            ads[intNum].name[strlen(ads[intNum].name) - 1] = '\0';
                            sprintf(buff, "%s : %d", ads[intNum].name, ads[intNum].cost);
                            write(fd, buff, strlen(buff));
                            write(fd, "\n", 1);
                            close(fd);
                        }
                        if (check == 0)
                            write(1, "No one offered on this advertisement\n", 37);
                    }
                    else if (strcmp(order, "reject") == 0)
                    {
                        char *id = strtok(NULL, " ");
                        int intNum = atoi(id);
                        int check = 0;
                        if (intNum > ads_count)
                        {
                            write(1, "ERROR! You don't have this id of advertisement.\n", 48);
                            check = 1;
                        }
                        else if (strcmp(ads[intNum].state, "Discuss") == 0)
                        {
                            strcpy(ads[intNum].state, "Waiting");
                            char broadcast_buffer[SIZE * 4 + 6], message[SIZE], msg[SIZE + 30];
                            sprintf(broadcast_buffer, "%d\n%s\n%s\n%s\n", ads[intNum].port, ads[intNum].name, seller_name, ads[intNum].state);
                            int a = sendto(sock, broadcast_buffer, strlen(broadcast_buffer), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                            sprintf(buff, "The advertisement is waiting now!\n");
                            write(1, buff, strlen(buff));
                            strcpy(message, "reject/");
                            strcpy(message + 7, buff);
                            send(ads[intNum].socket, message, strlen(message), 0);
                            check = 1;
                        }
                        if (check == 0)
                            write(1, "No one offered on this advertisement\n", 37);
                    }
                    else if (strcmp(order, "chat") == 0)
                    {
                        char *id = strtok(NULL, " ");
                        int idNum = atoi(id);
                        if (idNum >= ads_count)
                        {
                            write(1, "This advertisement doesn't exist.\n", 34);
                            write(1, "\tadd_advertisement\n\tshow_list\n\taccept\n\trejct\n\tchat id message\n\texit\n", 68);
                            continue;
                        }
                        else if (strcmp(ads[idNum].state, "Waiting") == 0)
                        {
                            write(1, "This advertisement is not in discuss state.\n", 44);
                            write(1, "\tadd_advertisement\n\tshow_list\n\taccept\n\trejct\n\tchat id message\n\texit\n", 68);
                            continue;
                        }
                        else if (strcmp(ads[idNum].state, "Expired") == 0)
                        {
                            write(1, "This advertisement is expired!\n", 31);
                            write(1, "\tadd_advertisement\n\tshow_list\n\taccept\n\trejct\n\tchat id message\n\texit\n", 68);
                            continue;
                        }
                        char *msg = strtok(NULL, " ");
                        char message[SIZE];
                        strcpy(message, "chat/");
                        strcpy(message + 5, msg);
                        send(ads[idNum].socket, message, strlen(message), 0);
                    }
                    else if (strcmp(buff, "exit\n") == 0)
                    {
                        exit(1);
                    }
                    else
                    {
                        write(1, "Your command is invalid!\n", 25);
                    }
                    write(1, "\tadd_advertisement\n\tshow_list\n\taccept\n\trejct\n\tchat id message\n\texit\n", 68);
                }
                else if (FD_ISSET(i, &waiting_set))
                {
                    for (int j = 0; j < ads_count; j++)
                    {
                        if (i == ads[j].socket)
                        {
                            int sockets;
                            sockets = acceptClient(i);
                            if (strcmp(ads[j].state, "Waiting") == 0)
                            {
                                FD_CLR(i, &waiting_set);
                                FD_CLR(i, &master_set);
                                close(i);
                                FD_SET(sockets, &master_set);
                                if (sockets > max_sd)
                                    max_sd = sockets;
                                ads[j].socket = sockets;
                                strcpy(ads[j].state, "Discuss");

                                char broadcast_buffer[SIZE * 4 + 6];
                                sprintf(broadcast_buffer, "%d\n%s\n%s\n%s\n", ads[j].port, ads[j].name, seller_name, ads[j].state);
                                int a = sendto(sock, broadcast_buffer, strlen(broadcast_buffer), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                            }
                            else if (strcmp(ads[j].state, "Expired") == 0)
                            {
                                char message[SIZE];
                                sprintf(message, "Sorry! This item is expired.\n");
                                send(sockets, message, strlen(message), 0);
                            }
                            else
                            {
                                char message[SIZE];
                                sprintf(message, "Sorry! This item is in discuss state.\n");
                                send(sockets, message, strlen(message), 0);
                            }
                        }
                    }
                }
                else
                { // client sending msg
                    int bytes_received, temp;
                    memset(buff, 0, SIZE);
                    bytes_received = recv(i, buff, SIZE, 0);

                    for (int j = 0; j < ads_count; j++)
                    {
                        if (i == ads[j].socket)
                        {
                            break;
                            temp = j;
                        }
                    }

                    if (bytes_received == 0)
                    { // EOF
                        strcpy(ads[temp].state, "Waiting");
                        write(1, "State is in waiting state now.\n", 31);
                        char broadcast_buffer[SIZE * 4 + 6];
                        sprintf(broadcast_buffer, "%d\n%s\n%s\n%s\n", ads[temp].port, ads[temp].name, seller_name, ads[temp].state);
                        int a = sendto(i, broadcast_buffer, strlen(broadcast_buffer), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    char a[SIZE + 20];
                    sprintf(a, "Client of ad : %s", ads[temp].name);
                    write(1, a, strlen(a));
                    checkMessages(buff, strlen(buff), temp);
                }
            }
        }
    }
}
