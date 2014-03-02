#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <io.h>
#include <direct.h>

#define PORT 8990
#define LG_BUF 64
#define LG_DIR 50

#define LGT_LOGIN 15
#define LGT_MDP 10
#define MDP_ADMIN "123456"
#define ADMIN 1
#define MEMBRE 0

#define CMD_ADMIN_SEND 1
#define CMD_ADMIN_LS 2

#define CMD_HELLO 1
#define CMD_TIME 2
#define CMD_STATUS 3
#define CMD_INFO 4
#define CMD_SERVEUR 5
#define CMD_COMMANDE 6

typedef struct client client;
struct client{
    unsigned long ID_client;
    unsigned short login[LGT_LOGIN+1];
    unsigned short mdp[LGT_MDP+1];
    unsigned short statut;
};

void commandeServ(SOCKET csock,unsigned short* string,client clientInf);
int numCmd(const char* string);
int numCmdAdm(const unsigned short* string);
char *datetime(void);
void emptyBuffer();
client autoten(SOCKET csock);
int receptionFichier(SOCKET csock,client clientInf,unsigned short *msg,char *string);
int downloadFichier(SOCKET csock,char* nomFichier,struct _finddata_t D);
char* convTime(int sec);
char* rechDir(client clientInf,char* nomFich);
int dir(client infCli,SOCKET csock);
int totalFilDir(char *dir);
void sendClient(SOCKET csock,client autotiClient);
struct _finddata_t recvStructFindData(SOCKET csock);
void  analyseChemin(char* destFichCli,char* destFichServ,const char* msg);
char* rechStr(const char *string,const char *string2);

void emptyBuffer()
{
    int c;
    while((c=getchar())!='\0' && c!='\n');
}

char *datetime(void)
{
  time_t t = time(0);
  static char buf[32];
  strftime(buf, sizeof buf, "%d/%m/%Y %H:%M:%S", localtime(&t));
  return buf;
}

void commandeServ(SOCKET csock,unsigned short* string,client clientInf)
{
    int i=0;
    unsigned long tailleMsg=0,cmd=0;
    unsigned short reponse=0,msg[LG_BUF];

    //initialisation de msg
    for(i=0;i<LG_BUF;i++)
        msg[i]=0;

    if(numCmd(string))
    {
        reponse=1;
        reponse=htons(reponse);
        send(csock,&reponse,2,0);
        printf("Commande declencher.\n");

        if(numCmd(string)==CMD_HELLO)
        {
            strcpy(msg,"Bonjour client.");
            tailleMsg=htonl(strlen(msg)*2+2);
        }
        else if(numCmd(string)==CMD_TIME)
        {
            strcpy(msg,datetime());
            tailleMsg=htonl(strlen(msg)*2+2);
        }
        else if(numCmd(string)==CMD_INFO)
        {
            strcpy(msg,"Serveur de transfert de fichier + tchat. Made by Mehdi.");
            tailleMsg=htonl(strlen(msg)*2+2);
        }
        else if(numCmd(string)==CMD_SERVEUR)
        {
            strcpy(msg,"Serveur V0.1.");
            tailleMsg=htonl(strlen(msg)*2+2);
        }
        else if(numCmd(string)==CMD_COMMANDE)
        {
            strcpy(msg,"hello\ttime\tinfo\tserveur\tstatus");
            tailleMsg=htonl(strlen(msg)*2+2);
        }
        else if(numCmd(string)==CMD_STATUS)
        {
            sprintf(msg,"Tu es %s.",clientInf.statut ? "Admin":"Membre");
            tailleMsg=htonl(strlen(msg)*2+2);
        }
        else
        {
            strcpy(msg,"Commande encore non definie.");
            tailleMsg=htonl(strlen(msg)*2+2);
        }

        if(SOCKET_ERROR!=send(csock,&tailleMsg,4,0))
            printf("Taille de la chaine envoyer : %d\n",(ntohl(tailleMsg)-2)/2);
        else
            printf("Erreur envoit de la taille de la chaine.");

        printf("Chaine envoyer : %s\n",msg);
        for(i=0;i<LG_BUF;i++)
            msg[i]=htons(msg[i]);
        tailleMsg=ntohl(tailleMsg);

        if(SOCKET_ERROR==send(csock,msg,tailleMsg,0))
                printf("ERREUR ENVOIT CHAINE\n");
    }
    else if( numCmdAdm(string) && clientInf.statut==ADMIN)
    {
        reponse=2;
        reponse=htons(reponse);
        if(send(csock,&reponse,2,0)==SOCKET_ERROR)
            printf("Envoie de la réponse echouer.\n");

        printf("Commande ADMIN declencher.\n");

        //envoit du numero de la commande admin
        cmd=htonl(numCmdAdm(string));
        if(SOCKET_ERROR!=send(csock,&cmd,4,0))
            printf("Numero de commande ADMIN declencher : %d\n",ntohl(cmd));
        else
            printf("Erreur d'envoit numero de commande ADMIN.");

        //reception du signal. Si le client ne connait pas la commande, la valeur sera de 0
        if(recv(csock, &reponse, 2, 0)==SOCKET_ERROR)
            printf("Envoie du signal echouer.\n");
        reponse=ntohs(reponse);

        if(numCmdAdm(string)==CMD_ADMIN_SEND && reponse!=0)
        {
            if(!(receptionFichier(csock,clientInf,msg,string)))
            {
                strcpy(msg,"Le fichier a ete sauvegarder.");
                tailleMsg=htonl(strlen(msg)*2+2);
            }
            else
            {
                strcpy(msg,"Erreur survenue, fichier non enregistrer.");
                tailleMsg=htonl(strlen(msg)*2+2);
            }
        }
        else if(numCmdAdm(string)==CMD_ADMIN_LS && reponse!=0)
        {
            (dir(clientInf,csock)>0)? sprintf(msg,"Total des fichier: %d",totalFilDir(rechDir(clientInf,"*.*"))) : strcpy(msg,"Aucun fichier dans le dossier.");
            tailleMsg=htonl(strlen(msg)*2+2);
        }
        else
        {
            if(reponse){
                strcpy(msg,"Commande Admin encore non definie.");
                tailleMsg=htonl(strlen(msg)*2+2);
            }
            else{
                printf("Le client ne connait pas la commande.\n");
                strcpy(msg,"commande interrompu.");
                tailleMsg=htonl(strlen(msg)*2+2);
            }
        }

        if(SOCKET_ERROR!=send(csock,&tailleMsg,4,0))
            printf("Taille de la chaine envoyer : %d\n",(ntohl(tailleMsg)-2)/2);
        else
            printf("Erreur envoit de la taille de la chaine.");

        printf("Chaine envoyer : %s\n",msg);
        for(i=0;i<LG_BUF;i++)
            msg[i]=htons(msg[i]);
        tailleMsg=ntohl(tailleMsg);

        if(SOCKET_ERROR==send(csock,msg,tailleMsg,0))
                printf("ERREUR ENVOIT CHAINE\n");
    }
    else{
        reponse=htons(reponse);
        send(csock,&reponse,2,0);
    }
}

int numCmd(const char* string)
{
    int numCommande=0;

    if(!(strcmp(string,"hello")))
        numCommande=CMD_HELLO;
    else if(!(strcmp(string,"time")))
        numCommande=CMD_TIME;
    else if(!(strcmp(string,"status")))
        numCommande=CMD_STATUS;
    else if(!(strcmp(string,"info")))
        numCommande=CMD_INFO;
    else if(!(strcmp(string,"serveur")))
        numCommande=CMD_SERVEUR;
    else if(!(strcmp(string,"commande")))
        numCommande=CMD_COMMANDE;

    return numCommande;
}

int numCmdAdm(const unsigned short* string)
{
    int numCommande=0;

    if(strstr(string,"send"))
        numCommande=CMD_ADMIN_SEND;
    else if(!(strcmp(string,"ls")))
        numCommande=CMD_ADMIN_LS;

    return numCommande;
}

client autoten(SOCKET csock)
{
    client autotiClient;
    int i=0;
    unsigned long lgMsg=0;
    unsigned short msg[LG_BUF];
    char type=0;

    autotiClient.ID_client=csock;

    sprintf(msg,"Votre nom d'utilisateur [%d caractere max] :",LGT_LOGIN);

    //transforme les données pour les envois network
    lgMsg=strlen(msg);
    for(i=0;i<LG_BUF || i<lgMsg;i++)
            msg[i]=htons(msg[i]);
        lgMsg=htonl(lgMsg*2+2);


    if(send(csock,&lgMsg,4,0)==SOCKET_ERROR)
        printf("Erreur envoit de la taille du message.\n");
    lgMsg=ntohl(lgMsg);

    if(send(csock,msg,lgMsg,0)==SOCKET_ERROR)
        printf("Erreur envoit de message.\n");

    if(recv(csock,&lgMsg,4,0)==SOCKET_ERROR)
        printf("Erreur reception de la taille du message.\n");
    lgMsg=ntohl(lgMsg);
    if(recv(csock,autotiClient.login,lgMsg,0)==SOCKET_ERROR)
        printf("Erreur reception du message.\n");
    for(i=0;i<LG_BUF || i<((lgMsg-2)/2);i++)
            autotiClient.login[i]=ntohs(autotiClient.login[i]);


    if(!(strcmp(autotiClient.login,"admin")))
    {
        //on confirme qu'il sagit d'un admin
        type=1;
        if(send(csock,&type,1,0)==SOCKET_ERROR)
            printf("Erreur d'envoit de type.\n");

        strcpy(msg,"Mot de passe d'admin :");
        lgMsg=htonl(strlen(msg)*2+2);

        if(send(csock,&lgMsg,4,0)==SOCKET_ERROR)
            printf("Erreur envoit de la taille du message.\n");
        lgMsg=ntohl(lgMsg);

        for(i=0;i<LG_BUF || i<((lgMsg-2)/2);i++)
            msg[i]=ntohs(msg[i]);
        if(send(csock,msg,lgMsg,0)==SOCKET_ERROR)
            printf("Erreur envoit de message.\n");

        if(recv(csock,&lgMsg,4,0)==SOCKET_ERROR)
            printf("Erreur reception de la taille du mot de passe.\n");
        lgMsg=ntohl(lgMsg);

        if(recv(csock,autotiClient.mdp,lgMsg,0)==SOCKET_ERROR)
            printf("Erreur reception du mot de passe.\n");
        for(i=0;i<LG_BUF || i<((lgMsg-2)/2);i++)
            autotiClient.mdp[i]=ntohs(autotiClient.mdp[i]);

        if(!(strcmp(autotiClient.mdp,MDP_ADMIN)))
        {
            strcpy(msg,"Bonjour admin.\n");
            lgMsg=htonl(strlen(msg)*2+2);

            if(send(csock,&lgMsg,4,0)==SOCKET_ERROR)
                printf("Erreur envoit de la taille du message.\n");
            lgMsg=ntohl(lgMsg);

            for(i=0;i<LG_BUF || i<((lgMsg-2)/2);i++)
                msg[i]=htons(msg[i]);
            if(send(csock,msg,lgMsg,0)==SOCKET_ERROR)
                printf("Erreur envoit de message.\n");

            autotiClient.statut=ADMIN;
            sendClient(csock,autotiClient);
        }
        else
        {
            strcpy(msg,"Mot de passe erroner. Atribution du login : Escro.\n");
            lgMsg=htonl(strlen(msg)*2+2);

            if(send(csock,&lgMsg,4,0)==SOCKET_ERROR)
                printf("Erreur envoit de la taille du message.\n");
            lgMsg=ntohl(lgMsg);

            for(i=0;i<LG_BUF || i<((lgMsg-2)/2);i++)
                msg[i]=htons(msg[i]);
            if(send(csock,msg,lgMsg,0)==SOCKET_ERROR)
                printf("Erreur envoit de message.\n");


            strcpy(autotiClient.login,"Escro");
            autotiClient.statut=MEMBRE;
            sendClient(csock,autotiClient);
        }

    }
    else
    {
        if(send(csock,&type,1,0)==SOCKET_ERROR)
            printf("Erreur d'envoit de type.\n");

        autotiClient.statut=MEMBRE;
        sendClient(csock,autotiClient);
    }

    printf("---Authentification réussi---\nID:%d \nPseudo:%s \nStatut:%s \n----Fin information---\n",autotiClient.ID_client,autotiClient.login,(autotiClient.statut)?"Admin":"Membre");

    return autotiClient;
}

int receptionFichier(SOCKET csock,client clientInf,unsigned short *msg,char *string)
{
    char nomFichier[LG_DIR]="";
    unsigned short reponse=0;
    int i=0;
    unsigned long tailleMsg=0;

    strcpy(msg,strstr(string,"send"));
    strcpy(msg,strchr(msg,' '));

    // Partie fichier
    analyseChemin(msg,nomFichier,msg);

    tailleMsg=htonl(strlen(msg)*2+2);

    if(SOCKET_ERROR!=send(csock,&tailleMsg,4,0))
        printf("Taille de la chaine envoyer : %d\n",(ntohl(tailleMsg)-2)/2);
    else
        {
            printf("Erreur envoit de la taille de la chaine.");
            return 1;
        }

    printf("Chaine envoyer : %s\n",msg);
    //met les bits en big-endian pour les normalisé en réseau
    tailleMsg=ntohl(tailleMsg);
    for(i=0;i<LG_DIR && i<((tailleMsg-2)/2);i++)
            msg[i]=htons(msg[i]);
    if(SOCKET_ERROR==send(csock,msg,tailleMsg,0))
        {
            printf("ERREUR ENVOIT CHAINE\n");
            return 1;
        }
    //------------------------------------------------------

    //reçois si tout c'est bien passer côté client
    reponse=0;
    if(recv(csock, &reponse, 2, 0) == SOCKET_ERROR)
        {
            printf("Erreur de la reception de la reponse client.\n");
            return 1;
        }
    reponse=ntohs(reponse);

    if(reponse)
    {
        struct _finddata_t D;
        D=recvStructFindData(csock);

        strcpy(nomFichier,rechDir(clientInf,nomFichier));

        return downloadFichier(csock,nomFichier,D);
        //Fin partie fichier

    }
    else
    {
        printf("Erreur du cote du client. Procedure stoper.\n");
        return 1;
    }
}

int downloadFichier(SOCKET csock,char* nomFichier,struct _finddata_t D)
{
    short octet=0;
    char temps[50]="";
    unsigned int i=0,octetTotal=0;
    unsigned long t_start = 0;
    unsigned long t_end = 0;
    FILE *fileupload=fopen(nomFichier,"wb");

    if(fileupload!=NULL)
    {
        printf("Creation du fichier : %s\n",nomFichier);

        t_start = clock();
        for(i=0;i<D.size;i++)
        {
            if(recv(csock, &octet, 1, 0)==SOCKET_ERROR)
                printf("echec de la reception de l'octet %d echouer.\n",i);

            else if( (t_end+1000)<t_start )
            {
                strcpy(temps,(i) ? convTime((D.size-i+1)/(i-octetTotal)) : "");
                //Permet d'effacer les débordements afficher sur l'ecran et reviens en début de ligne
                printf("\b%c\b%c\b%c\b%c\r",0,0,0,0);
                printf("Recu:%d/%d octects | Debit : %d octet/s | temps restant : %s",i+1,D.size,i-octetTotal,temps);
                octetTotal=i;
                t_end=t_start;
            }
            octet=ntohs(octet);
            fwrite( &octet , 1 , 1 , fileupload );
            t_start=clock();
        }

        printf("\nCreation du fichier terminer.\n");
        fclose(fileupload);

        return 0;

    }
    else
    {
        printf("Echec de la creation du fichier : %s\n",nomFichier);
        return 1;
    }
}

char* convTime(int sec)
{
    int minute=0,heure=0,jour=0;
    char buffer[64]="",tmp[20]="";

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

char* rechDir(client clientInf,char* nomFich)
{
    struct _finddata_t D;
    char buffer[64]="";
    int done=0,hd,trouver=0;

    hd = _findfirst("*.*",&D);
    if (hd==-1)
        return;
    while (!done)
    {
        if(!(strcmp(clientInf.login,D.name)) && (D.attrib&0x10) )
            trouver=1;
        done = _findnext(hd,&D);
    }

    if( !(trouver) )
    {
        printf("Dossier utilisateur non trouver. Creation d'un dossier.\n");
        mkdir(clientInf.login);
        printf("Dossier creer : %s \n",clientInf.login);
    }

    sprintf(buffer,"%s/%s",clientInf.login,nomFich);

    return buffer;
}

int dir(client infCli,SOCKET csock)
{
    int done=0,hd=0,i=0,TotalFich=0;
    unsigned long nb=0;
    struct _finddata_t D;
    unsigned short name[LG_DIR];
    char dir[LG_DIR]="";

    strcpy(dir,rechDir(infCli,"*.*"));

    hd = _findfirst(dir,&D);
    if (hd==-1){
        nb=htonl(TotalFich);
        if(send(csock,&nb,4,0)==SOCKET_ERROR)
                printf("Erreur envoit du nombre de fichier.\n");
        strcpy(name,"Aucun fichier dans le dossier.");
    }
    else{
        TotalFich=totalFilDir(dir);
        nb=htonl(TotalFich);
        if(send(csock,&nb,4,0)==SOCKET_ERROR)
                printf("Erreur envoit du nombre de fichier.\n");

        while (!done){
            if(!(D.attrib&0x10)){
                strcpy(name,D.name);
                printf("%s\n",name);

                nb=htonl(strlen(name)*2+2);
                if(send(csock,&nb,4,0)==SOCKET_ERROR)
                    printf("Erreur envoit de la taille du message.\n");

                for(i=0;i<LG_DIR && i<((ntohl(nb)-2)/2);i++)
                    name[i]=htons(name[i]);
                if(send(csock,name,ntohl(nb),0)==SOCKET_ERROR)
                    printf("Erreur envoit du message.\n");
            }
            done = _findnext(hd,&D);
        }
    }
    return TotalFich;
}

int totalFilDir(char *dir)
{
    int i=0,hd=0;
    struct _finddata_t D;

    hd = _findfirst(dir,&D);
    if(hd==-1)
        return 0;
    else
    {
        for(i=(D.attrib&0x10)? 0:1; !(_findnext(hd,&D));i++)
        {
            if(D.attrib&0x10)
                i--;
        }
        return i;
    }
}

void sendClient(SOCKET csock,client autotiClient)
{
    unsigned long taille;
    int i=0;

    //envoit ID_Client
    taille=htonl(autotiClient.ID_client);
    if(SOCKET_ERROR==send(csock,&taille,4,0))
            printf("ERREUR ENVOIT ID CLIENT\n");

    //envoit login
    taille=htonl(strlen(autotiClient.login)*2+2);
    if(SOCKET_ERROR==send(csock,&taille,4,0))
            printf("ERREUR ENVOIT TAILLE LOGIN\n");
    taille=ntohl(taille);

    for(i=0;i<LG_BUF && i<((taille-2)/2);i++)
            autotiClient.login[i]=htons(autotiClient.login[i]);
    if(SOCKET_ERROR==send(csock,autotiClient.login,taille,0))
            printf("ERREUR ENVOIT CHAINE LOGIN\n");
    for(i=0;i<LG_BUF && i<((taille-2)/2);i++)
            autotiClient.login[i]=ntohs(autotiClient.login[i]);

    //envoit mdp
    taille=htonl(strlen(autotiClient.mdp)*2+2);
    if(SOCKET_ERROR==send(csock,&taille,4,0))
            printf("ERREUR ENVOIT TAILLE MDP\n");
    taille=ntohl(taille);

    for(i=0;i<LG_BUF && i<((taille-2)/2);i++)
            autotiClient.mdp[i]=htons(autotiClient.mdp[i]);
    if(SOCKET_ERROR==send(csock,autotiClient.mdp,taille,0))
            printf("ERREUR ENVOIT CHAINE MDP\n");
    for(i=0;i<LG_BUF && i<((taille-2)/2);i++)
            autotiClient.mdp[i]=ntohs(autotiClient.mdp[i]);

    //envoit statut
    autotiClient.statut=htons(autotiClient.statut);
    if(SOCKET_ERROR==send(csock,&autotiClient.statut,1,0))
            printf("ERREUR ENVOIT STATUT\n");
    autotiClient.statut=ntohs(autotiClient.statut);
}

void  analyseChemin(char* destFichCli,char* destFichServ,const char* msg)
{
    int i=0,j=0;
    char tmpNomFich[LG_DIR]="",tmpNomFich2[LG_DIR]="",suprEsp[LG_DIR]="";

    strcpy(suprEsp,msg);
    while(suprEsp[0]==' ')
    {
        for(i=0;suprEsp[i]!='\0' && i<LG_DIR;i++)
            suprEsp[i]=suprEsp[i+1];
    }

    for(i=0;suprEsp[i]!=' ' && suprEsp[i]!='\0' && i<LG_DIR;i++)
        tmpNomFich[i]=suprEsp[i];
    tmpNomFich[i]='\0';

    strcpy(destFichServ,tmpNomFich);

    //marque la fin de la première chaine pour rechercher la deuxième
    suprEsp[i]='\0';
    while(suprEsp[i+1]==' ')
    {
        for(j=0;suprEsp[i+j]!='\0' && (i+j)<LG_DIR;j++)
            suprEsp[i+j]=suprEsp[i+j+1];
    }

    for(j=1;suprEsp[j+i]!=' ' && suprEsp[j+i]!='\0' && (i+j)<LG_DIR;j++)
        tmpNomFich2[j-1]=suprEsp[j+i];
    tmpNomFich2[j]='\0';

    strcpy(destFichCli,tmpNomFich2);
}

struct _finddata_t recvStructFindData(SOCKET csock)
{
    int i=0;
    struct _finddata_t D;

    if(recv(csock, &D.attrib ,sizeof(D.attrib) , 0) == SOCKET_ERROR)
        printf("Erreur de la reception de l'attribu du fichier.\n");
    D.attrib=ntohs(D.attrib);

    if(recv(csock, &D.time_create ,4 , 0) == SOCKET_ERROR)
        printf("Erreur de la reception du temps de création du fichier.\n");
    D.time_create=ntohl(D.time_create);

    if(recv(csock, &D.time_access ,4 , 0) == SOCKET_ERROR)
        printf("Erreur de la reception du temps d'acces du fichier.\n");
    D.time_access=ntohl(D.time_access);

    if(recv(csock, &D.time_write ,4 , 0) == SOCKET_ERROR)
        printf("Erreur de la reception du temps d'écriture du fichier.\n");
    D.time_write=ntohl(D.time_write);

    if(recv(csock, &D.size ,4 , 0) == SOCKET_ERROR)
        printf("Erreur de la reception de la taille du fichier.\n");
    D.size=ntohl(D.size);

    if(recv(csock, D.name ,260 , 0) == SOCKET_ERROR)
        printf("Erreur de la reception du nom du fichier.\n");
    for(i=0;i<260;i++)
        D.name[i]=ntohs(D.name[i]);

    return D;
}

char* rechStr(const char *string,const char *string2)
{
    char *string3;
    int i=0,j=0,identique=0;

    string3=0;

    for(i=0;i<strlen(string) && string3==0;i++)
    {
        if(string[i]==string2[0])
        {
            for(j=strlen(string2);j>0;j--)
            {
                if(string[i+j-1]==string2[j-1])
                    identique++;
            }
            if(identique==strlen(string2))
                string3=&string[i];
            else
                identique=0;
        }
    }

    return string3;
}

#endif // MAIN_H_INCLUDED
