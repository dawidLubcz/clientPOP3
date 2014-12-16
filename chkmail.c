//author Dawid Lubczynski
//simple pop3 client
//client connects to server and shows message list

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define MAXBUF 2048
#define RESPBUF 2048

//error codes
enum ERROR_CODE
{
    EC_NO_ERROR=0,
    EC_ERROR=1
};

//verification establishing a connection
void err(char *where)
{
	fprintf(stderr, "error in %s: %d\n", where, errno);
	exit(1);
}

//veryfication server response
int respVer(char *resp, char *where)
{
    int ret = EC_ERROR;

    if('+' == resp[0])
    {
        printf(where); printf(" OK\n");
        ret = EC_NO_ERROR;
    }
    else if ('-' == resp[0])
    {
        printf("Error with "); printf(where); printf("\n");
        ret = EC_ERROR;
    }
    else
    {
        printf("Unknown response "); printf(where); printf("\t"); printf(resp); printf("\n");
    }

    return ret;
}

//main
int main(int argc, char *argv[])
{
    //variables
    int error = EC_NO_ERROR;    
    char *remote =  argv[2];    //server address
    char *user   =  argv[1];    //user name 
    char *pass   =  argv[3];    //password
    struct servent  *_psSent;
    struct protoent *_psPent;
    int  _iPort = 110;
    int  _iSock;
    int  _iResult;
	in_addr_t ipadr;
    struct sockaddr_in _sAddr;
    struct hostent     *_psHent;
    char _tcResponse[RESPBUF];  
    char _tcUser[MAXBUF];
    char _tcPass[MAXBUF];
    char _tcBuf[MAXBUF];

    //check input parameters
    if(argc < 4)
    {
        printf("Too few parameters!\n");
        printf("chkmail user_name server_addr password\n\n");
        error = EC_ERROR;
        return 1;
    }
    
    //clear buffers
    memset(_tcUser,'\0',sizeof(_tcUser));
    memset(_tcPass,'\0',sizeof(_tcPass));

    //create client requests
    strcpy(_tcUser, "USER ");
    strcat(_tcUser, user);
    strcat(_tcUser, "\r\n");

    strcpy(_tcPass, "PASS ");
    strcat(_tcPass, pass);
    strcat(_tcPass, "\r\n");

    //init structs
    _psSent = getservbyname("pop3", "tcp");
    if(_psSent == NULL)    
		err("getservbyname");

    //fill structure
    _iPort = _psSent->s_port;
    _psPent = getprotobyname("tcp");
    if(_psPent == NULL)
        err("getprotobyname");


    _psHent = gethostbyname(remote);

    _sAddr.sin_family   = AF_INET;
    _sAddr.sin_port     = _iPort;
    _sAddr.sin_addr     = *((struct in_addr *)_psHent->h_addr);
    memset(_sAddr.sin_zero, '\0', 8);

    //create socket
    _iSock = socket(AF_INET, SOCK_STREAM, _psPent->p_proto);
    if(_iSock < 0)   
		err("socket");

    //try connect to server
        _iResult = connect(_iSock, (struct sockaddr *)&_sAddr, sizeof(struct sockaddr));
    if(_iResult < 0)    
		err("connect");     
    else    
        printf("\nConnected!\n");

    //check if connected
    if(error == EC_NO_ERROR)
    {
        _iResult = recv(_iSock, _tcResponse, RESPBUF-1, 0);
        error = respVer(_tcResponse,"connecting");

        send(_iSock, _tcUser, strlen(_tcUser),0);
    }

    //check if password and user name were correct
    if(error == EC_NO_ERROR)
    {
        _iResult = recv(_iSock, _tcResponse, RESPBUF-1, 0);
        send(_iSock, _tcPass, strlen(_tcPass), 0);

        _iResult = recv(_iSock, _tcResponse, RESPBUF-1, 0);
        error = respVer(_tcResponse,"authentication");
    }

    //request a list of messages from the server
    strcpy(_tcBuf,"LIST\r\n");

    if(error == EC_NO_ERROR)
    {
        write(_iSock, _tcBuf, strlen(_tcBuf));

        _iResult = recv(_iSock, _tcResponse, RESPBUF-1, 0);
        error = respVer(_tcResponse,"send LIST");
    }

    memset(_tcBuf,'\0',sizeof(_tcBuf));

    //print messages
    if(error == EC_NO_ERROR)
        printf("\nList of messages below.\n");

    _iResult = MAXBUF;

    if(error == EC_NO_ERROR)
        while(1)
        {

            _iResult= recv(_iSock, _tcBuf, MAXBUF-1,0);
            _tcBuf[_iResult] = '\0';
            puts(_tcBuf);
            if(_iResult < MAXBUF -1)break;
        }

    //end session
    strcpy(_tcBuf,"QUIT\r\n");
    if(error == EC_NO_ERROR)
        write(_iSock, _tcBuf, strlen(_tcBuf));

    if(error == EC_NO_ERROR)
    {
        _iResult = recv(_iSock, _tcResponse, RESPBUF-1, 0);
        error = respVer(_tcResponse,"exit from server");
    }

    close(_iSock);
	return 0;
}
