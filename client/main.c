#define WIN32
#if defined (WIN32)
    #include <winsock2.h>
    typedef int socklen_t;
#elif defined (linux)
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket(s) close(s)
    typedef int SOCKET;
    typedef struct sockaddr_in SOCKADDR_IN;
    typedef struct sockaddr SOCKADDR;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#define PORT 8990
#define LG_BUF 64

int main(void)
{
    #if defined (WIN32)
        WSADATA WSAData;
        int erreur = WSAStartup(MAKEWORD(2,2), &WSAData);
    #else
        int erreur = 0;
    #endif

    SOCKET sock;
    SOCKADDR_IN sin;
    int loop=1,i=0;
    unsigned short buffer[LG_BUF],repServ=0;
    unsigned long nb=0;
    client infoClient;

    //initialisation du buffer
    for(i=0;i<LG_BUF;i++)
        buffer[i]=0;

    /* Si les sockets Windows fonctionnent */
    if(!erreur)
    {
        printf("Entrer l'adresse IP du serveur de communication :");
        scanf("%s",buffer);
        emptyBuffer();

        /* Création de la socket */
        sock = socket(AF_INET, SOCK_STREAM, 0);

        /* Configuration de la connexion */
        sin.sin_addr.s_addr = inet_addr(buffer);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(PORT);

        /* Si l'on a réussi à se connecter */
        if(connect(sock, (SOCKADDR*)&sin, sizeof(sin)) != SOCKET_ERROR)
        {
            printf("Connection a %s sur le port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));

            infoClient=autoten(sock);

            do{
                printf("%s :",infoClient.login);
                message(buffer);

                nb=strlen(buffer)*2+2;
                nb=htonl(nb);

                if(send(sock, &nb, 4, 0)==SOCKET_ERROR)
                    printf("Envoie de la chaine echouer.\n");

                for(i=0;i<LG_BUF;i++)
                    buffer[i]=htons(buffer[i]);
                nb=ntohl(nb);
                if(send(sock, buffer, nb, 0) == SOCKET_ERROR)
                    printf("Erreur de transmission\n");
                for(i=0;i<LG_BUF;i++)
                    buffer[i]=ntohs(buffer[i]);

                if(!(strcmp(buffer,"exit")) || !(strcmp(buffer,"shutdown")) )
                    loop=0;

                recv(sock,&repServ,2,0);
                repServ=ntohs(repServ);
                if(repServ==1)
                {
                    if(recv(sock, &nb, 4, 0) == SOCKET_ERROR)
                        printf("Erreur de la reception de la taille\n");
                    nb=ntohl(nb);

                    if(recv(sock, buffer, nb, 0) != SOCKET_ERROR){
                        for(i=0;i<LG_BUF;i++)
                            buffer[i]=ntohs(buffer[i]);
                        printf("Reponse serveur: %s\n", buffer);
                    }
                    else
                        printf("Erreur de la reception de la chaine de caractere.\n");
                }
                else if(repServ==2)
                {
                    if(recv(sock, &nb, 4, 0) == SOCKET_ERROR)
                        printf("Erreur de la reception du numero de la commande ADMIN\n");

                    else
                    {
                        nb=ntohl(nb);
                        if(nb==COMMANDE_SEND_ADMIN){
                            repServ=htons(1);
                            if(send(sock, &repServ, 2, 0)==SOCKET_ERROR)
                                printf("Envoie du signal echouer.\n");
                            envoitFichier(sock,buffer);
                        }
                        else if(nb==COMMANDE_LS_ADMIN){
                            repServ=htons(1);
                            if(send(sock, &repServ, 2, 0)==SOCKET_ERROR)
                                printf("Envoie du signal echouer.\n");
                            cmdLS(sock);
                        }
                        else
                        {
                            printf("Le client ne connait pas cette commande ADMIN. Mettez a jour votre logiciel.\n");
                            repServ=htons(0);
                            if(send(sock, &repServ, 2, 0)==SOCKET_ERROR)
                                printf("Envoie du signal echouer.\n");
                        }

                        if(recv(sock, &nb, 4, 0) == SOCKET_ERROR)
                            printf("Erreur de la reception de la taille\n");
                        nb=ntohl(nb);

                        if(recv(sock, buffer, nb, 0) != SOCKET_ERROR){
                            for(i=0;i<LG_BUF;i++)
                                buffer[i]=ntohs(buffer[i]);
                            printf("Reponse serveur: %s\n", buffer);
                        }
                        else
                            printf("Erreur de la reception de la chaine de caractere.\n");
                        }
                }
                repServ=0;
            }while(loop);
        }
        /* sinon, on affiche "Impossible de se connecter" */
        else
        {
            printf("Impossible de se connecter\n");
        }

        /* On ferme la socket */
        closesocket(sock);

        #if defined (WIN32)
            WSACleanup();
        #endif
    }

    /* On attend que l'utilisateur tape sur une touche, puis on ferme */
    getchar();

    return EXIT_SUCCESS;
}
