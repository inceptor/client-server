#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <io.h>
#include <direct.h>
#include <time.h>

#define LGT_LOGIN 15
#define LGT_MDP 10
#define COMMANDE_SEND_ADMIN 1
#define COMMANDE_LS_ADMIN 2
#define LG_BUF 64
#define LG_DIR 50

typedef struct client client;
struct client{
    unsigned long ID_client;
    unsigned short login[LGT_LOGIN+1];
    unsigned short mdp[LGT_MDP+1];
    unsigned short statut;
};

void emptyBuffer();
void message(char string[]);
client autoten(SOCKET sock);
void envoitFichier(SOCKET sock,unsigned short* buffer);
int uploadFichier(SOCKET sock,char* nomFichier,struct _finddata_t D);
char* convTime(int sec);
client recvClient(SOCKET sock,client autotiClient);
void sendStructFindData(SOCKET sock,struct _finddata_t D);
int cmdLS(SOCKET sock);

void emptyBuffer()
{
    int c;
    while((c=getchar())!='\0' && c!='\n');
}

void message(char string[])
{
    scanf("%[^\n]",string);
    emptyBuffer();
}

client autoten(SOCKET sock)
{
    client autotiClient;
    int type=0,i=0;
    unsigned long lgMsg=0;
    unsigned short msgServ[LG_BUF];

    if(recv(sock,&lgMsg,4,0)==SOCKET_ERROR)
        printf("Erreur de reception de taille du message.\n");
    lgMsg=ntohl(lgMsg);

    if(recv(sock,msgServ,lgMsg,0)==SOCKET_ERROR)
        printf("Erreur de reception de message.\n");
    for(i=0;i<((lgMsg-2)/2);i++)
            msgServ[i]=ntohs(msgServ[i]);

    printf("%s",msgServ);
    scanf("%15s",autotiClient.login);
    emptyBuffer();

    lgMsg=htonl(strlen(autotiClient.login)*2+2);

    if(send(sock,&lgMsg,4,0)==SOCKET_ERROR)
        printf("Erreur d'envois de taille du message.\n");
    lgMsg=ntohl(lgMsg);
    for(i=0;i<((lgMsg-2)/2);i++)
            autotiClient.login[i]=htons(autotiClient.login[i]);

    if(send(sock,autotiClient.login,lgMsg,0)==SOCKET_ERROR)
        printf("Erreur d'envois de message.\n");

    if(recv(sock,&type,1,0)==SOCKET_ERROR)
        printf("Erreur de reception du type.\n");
    for(i=0;i<((lgMsg-2)/2);i++)
            autotiClient.login[i]=ntohs(autotiClient.login[i]);

    if(type)
    {
        if(recv(sock,&lgMsg,4,0)==SOCKET_ERROR)
            printf("Erreur de reception de taille du message.\n");
        lgMsg=ntohl(lgMsg);

        if(recv(sock,msgServ,lgMsg,0)==SOCKET_ERROR)
            printf("Erreur de reception de message.\n");
        for(i=0;i<LG_BUF || i<((lgMsg-2)/2);i++)
                msgServ[i]=ntohs(msgServ[i]);

        printf("%s",msgServ);
        scanf("%10s",autotiClient.mdp);
        emptyBuffer();

        lgMsg=htonl(strlen(autotiClient.mdp)*2+2);

        if(send(sock,&lgMsg,4,0)==SOCKET_ERROR)
            printf("Erreur d'envois de taille du message.\n");
        lgMsg=ntohl(lgMsg);

        for(i=0;i<LG_BUF || i<((lgMsg-2)/2);i++)
                autotiClient.mdp[i]=htons(autotiClient.mdp[i]);
        if(send(sock,autotiClient.mdp,lgMsg,0)==SOCKET_ERROR)
            printf("Erreur d'envois de message.\n");

        //reception message de confirmation statut
        if(recv(sock,&lgMsg,4,0)==SOCKET_ERROR)
            printf("Erreur de reception de taille du message.\n");
        lgMsg=ntohl(lgMsg);

        if(recv(sock,msgServ,lgMsg,0)==SOCKET_ERROR)
            printf("Erreur de reception de message.\n");
        for(i=0;i<LG_BUF || i<((lgMsg-2)/2);i++)
                msgServ[i]=ntohs(msgServ[i]);

        printf("%s",msgServ);

        //reception structure
        autotiClient=recvClient(sock,autotiClient);
    }
    else
        autotiClient=recvClient(sock,autotiClient);

    return autotiClient;
}

void envoitFichier(SOCKET sock,unsigned short* buffer)
{
    struct _finddata_t D;
    unsigned long tailleMsg=0;
    int hd=0,i=0;
    unsigned short signal=0;

    if(recv(sock, &tailleMsg, 4, 0) == SOCKET_ERROR)
        printf("Erreur de la reception de la taille\n");
    tailleMsg=ntohl(tailleMsg);

    if(recv(sock, buffer, tailleMsg, 0) == SOCKET_ERROR)
        printf("Erreur de la reception de la chaine de caractere.\n");
    for(i=0;i<LG_BUF || i<((tailleMsg-2)/2);i++)
                buffer[i]=ntohs(buffer[i]);

    //recherche du fichier
    hd = _findfirst(buffer,&D);
    if(hd!=-1)
    {
        //envoit du signal OK
        signal=htons(1);
        if(send(sock, &signal, 2, 0)==SOCKET_ERROR)
            printf("Envoie de la reponse serveur echouer.\n");
        signal=0;

        //envoit de la structure du fichier
        sendStructFindData(sock,D);

        uploadFichier(sock,buffer,D);
    }
    else
    {
        if(send(sock, &signal, 2, 0)==SOCKET_ERROR)
            printf("Envoie de la reponse serveur echouer.\n");
        printf("Erreur de saisie dans le nom du fichier.\n");
    }
}

int uploadFichier(SOCKET sock,char* nomFichier,struct _finddata_t D)
{
    short octet=0;
    char temps[64]="";
    unsigned int i=0,octetTotal=0;
    unsigned long t_start = 0;
    unsigned long t_end = 0;
    FILE *fichier;

    fichier=fopen(nomFichier,"rb");
    if(fichier!=NULL)
    {
        printf("Copie du fichier %s.\n",D.name);

        t_start = clock();
        for(i=0;i<D.size;i++)
        {
            fread( &octet , 1 , 1 , fichier );
            octet=htons(octet);
            if(send(sock, &octet, 1, 0)==SOCKET_ERROR)
                printf("Envoie de l'octet %d echouer.\n",i);
            else if( (t_end+1000)<t_start )
            {
                strcpy(temps,(i) ? convTime((D.size-i+1)/(i-octetTotal)) : "");
                printf("Recu:%d/%d octects | Debit : %d octet/s | temps restant : %s\r",i+1,D.size,i-octetTotal,temps);
                octetTotal=i;
                t_end=t_start;
            }
            t_start=clock();
        }

        printf("\nFermeture du fichier.\n");
        fclose(fichier);

        return 0;
    }
    else
    {
        printf("Erreur d'ouverture du fichier.\n");
        return 1;
    }
}

char* convTime(int sec)
{
    int minute=0,heure=0,jour=0;
    char buffer[50]="";

    if(sec>=60)
        minute=sec/60;

    if(minute>=60)
        heure=minute/60;

    if(heure>=60)
        jour=heure/24;

    sec%=60,minute%=60,heure%=24;

    if(jour) sprintf(buffer,"%d j ",jour);
    if(heure) sprintf(buffer,"%d h ",heure);
    if(minute) sprintf(buffer,"%d m ",minute);
    if(sec) sprintf(buffer,"%d s",sec);

    return buffer;
}

client recvClient(SOCKET sock,client autotiClient)
{
    unsigned long taille;
    int i=0;

    //reception ID_Client
    if(recv(sock,&autotiClient.ID_client,4,0)==SOCKET_ERROR)
            printf("Erreur de reception de l'ID client.\n");
    autotiClient.ID_client=ntohl(autotiClient.ID_client);


    //reception login
    if(recv(sock,&taille,4,0)==SOCKET_ERROR)
            printf("Erreur de reception de la taille du login client.\n");
    taille=ntohl(taille);

    if(recv(sock,autotiClient.login,taille,0)==SOCKET_ERROR)
            printf("Erreur de reception du login client.\n");
    for(i=0;i<LG_BUF || i<((taille-2)/2);i++)
            autotiClient.login[i]=ntohs(autotiClient.login[i]);

    //reception mdp
    if(recv(sock,&taille,4,0)==SOCKET_ERROR)
            printf("Erreur de reception de la taille du mot de passe client.\n");
    taille=ntohl(taille);

    if(SOCKET_ERROR==recv(sock,autotiClient.mdp,taille,0))
            printf("Erreur de reception du mot de passe client.\n");
    for(i=0;i<LG_BUF || i<((taille-2)/2);i++)
            autotiClient.mdp[i]=ntohs(autotiClient.mdp[i]);

    //envoit statut
    if(SOCKET_ERROR==recv(sock,&autotiClient.statut,1,0))
            printf("Erreur de reception du statut client.\n");
    autotiClient.statut=ntohs(autotiClient.statut);

    return autotiClient;
}

void sendStructFindData(SOCKET sock,struct _finddata_t D)
{
    int i=0;

    D.attrib=htons(D.attrib);
    if(send(sock, &D.attrib ,sizeof(D.attrib) , 0) == SOCKET_ERROR)
        printf("Erreur de l'envois de l'attribu du fichier.\n");

    D.time_create=htonl(D.time_create);
    if(send(sock, &D.time_create ,4 , 0) == SOCKET_ERROR)
        printf("Erreur de l'envois du temps de création du fichier.\n");

    D.time_access=htonl(D.time_access);
    if(send(sock, &D.time_access ,4 , 0) == SOCKET_ERROR)
        printf("Erreur de l'envois du temps d'acces du fichier.\n");

    D.time_write=htonl(D.time_write);
    if(send(sock, &D.time_write ,4 , 0) == SOCKET_ERROR)
        printf("Erreur de l'envois du temps d'écriture du fichier.\n");

    D.size=htonl(D.size);
    if(send(sock, &D.size,4 , 0) == SOCKET_ERROR)
        printf("Erreur de l'envois de la taille du fichier.\n");

    for(i=0;i<260;i++)
        D.name[i]=htons(D.name[i]);
    if(send(sock, D.name ,260 , 0) == SOCKET_ERROR)
        printf("Erreur de l'envois du nom du fichier.\n");

    return D;
}

int cmdLS(SOCKET sock)
{
    int i=0;
    unsigned long loop=0,nb=0,totalFich=0;
    unsigned short nameFich[LG_DIR];

    //reception du nombre de texte a afficher
    if(recv(sock,&loop,4,0)==SOCKET_ERROR)
        printf("Erreur de reception du nombre de fichier.\n");
    totalFich=ntohl(loop);
    if(totalFich>0){
        for(loop=ntohl(loop);loop>0;loop--)
        {
            if(recv(sock,&nb,4,0)==SOCKET_ERROR)
                printf("Erreur de reception de la taille du message.\n");

            if(recv(sock,nameFich,ntohl(nb),0)==SOCKET_ERROR)
                printf("Erreur de reception du message.\n");
            for(i=0;i<LG_DIR && i<((ntohl(nb)-2)/2);i++)
                    nameFich[i]=htons(nameFich[i]);
            printf("%s \n",nameFich);
        }
    }

    return totalFich;
}

#endif // MAIN_H_INCLUDED
