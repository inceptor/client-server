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
#include <time.h>

#include "main.h"

int main(void)
{
    #if defined (WIN32)
        WSADATA WSAData;
        int erreur = WSAStartup(MAKEWORD(2,2), &WSAData);
    #else
        int erreur = 0;
    #endif

    SOCKET sock,csock;
    SOCKADDR_IN sin,csin;
    socklen_t recsize = sizeof(csin);
    int sock_err,closeServ=0,loop=1,i=0;
    unsigned long longueurBuffer=0;
    unsigned short buffer[LG_BUF];
    client infoClient;

    //ini du buffer
    for(i=0;i<LG_BUF;i++)
        buffer[i]=0;

    /* Si les sockets Windows fonctionnent */
    if(!erreur)
    {
        do{
            sock = socket(AF_INET, SOCK_STREAM, 0);

            /* Si la socket est valide */
            if(sock != INVALID_SOCKET)
            {
                printf("La socket %d est maintenant ouverte en mode TCP/IP\n", sock);

                /* Configuration */
                sin.sin_addr.s_addr    = htonl(INADDR_ANY);   /* Adresse IP automatique */
                sin.sin_family         = AF_INET;             /* Protocole familial (IP) */
                sin.sin_port           = htons(PORT);         /* Listage du port */
                sock_err = bind(sock, (SOCKADDR*)&sin, sizeof(sin));

                /* Si la socket fonctionne */
                if(sock_err != SOCKET_ERROR)
                {
                    /* Démarrage du listage (mode server) */
                    sock_err = listen(sock, 5);
                    printf("Listage du port %d...\n", PORT);

                    /* Si la socket fonctionne */
                    if(sock_err != SOCKET_ERROR)
                    {
                        /* Attente pendant laquelle le client se connecte */
                        printf("Patientez pendant que le client se connecte sur le port %d...\n", PORT);

                        csock = accept(sock, (SOCKADDR*)&csin, &recsize);
                        printf("Un client se connecte avec la socket %d de %s:%d\n", csock, inet_ntoa(csin.sin_addr), htons(csin.sin_port));

                        infoClient=autoten(csock);

                        do{
                            if(recv(csock, &longueurBuffer, 4, 0) != SOCKET_ERROR){
                                longueurBuffer = ntohl(longueurBuffer);
                                printf("Longueur de la chaine: %d\n", longueurBuffer/2);
                            }
                            else
                                printf("Erreur reception de la longueur chaine.\n");

                            if(recv(csock, buffer, longueurBuffer, 0) != SOCKET_ERROR)
                            {
                                for(i=0;i<LG_BUF;i++)
                                    buffer[i]=ntohs(buffer[i]);
                                printf("Recu de %s (%d) : %s\n", infoClient.login, infoClient.ID_client, buffer);

                                if(!(strcmp(buffer,"exit")) || !(strcmp(buffer,"shutdown")))
                                    loop=0;
                                if(!(strcmp(buffer,"shutdown")) && infoClient.statut==ADMIN)
                                    closeServ=1;

                                commandeServ(csock,buffer,infoClient);

                            }
                            else
                                printf("Erreur reception de la chaine.\n");

                            buffer[0]='\0';

                        }while(loop);
                        //initialise loop
                        loop=1;
                        /* Il ne faut pas oublier de fermer la connexion (fermée dans les deux sens) */
                        shutdown(csock, 2);
                    }
                }

                /* Fermeture de la socket */
                printf("Fermeture de la socket n %d...\n",sock);
                closesocket(sock);
            }
            else
            {
                printf("Socket invalide.");
                closeServ=1;
            }
        }while( !(closeServ) );

    #if defined (WIN32)
        WSACleanup();
    #endif
    }

    printf("Fermeture du serveur terminee\n");
    /* On attend que l'utilisateur tape sur une touche, puis on ferme */
    getchar();

    return EXIT_SUCCESS;
}
