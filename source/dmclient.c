#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "dmclient.h"
#include "cJSON.h"
#include "logger.h"

struct resources *rhead=NULL;
struct resources *rcurrent=NULL;
int rcount = 0, err, sum, clen, header_finished=3;
char url[128] = "60.251.161.12", http_port[8] = "8080", buffer[BUFFER_SIZE] = "";


REG_STATE reg_result = REG_FAILED;
char dmc_xmpp_un[MAX_XMPPUN_LEN]="user3@localhost/smack0";
char dmc_xmpp_pw[MAX_XMPPPW_LEN]="1qaz2wsx";
char dmc_xmpp_url[MAX_XMPPSV_LEN]="172.16.100.191";

char* strrep(char *str, char *rep)  {
    char *pos;    
    char* buf[128];
    char* ret;


    pos = strstr(str, rep);
    if (pos == NULL)  {
        return NULL;
    }
    
    *pos = NULL;
    
    strcpy(buf, str);
    strcat( buf,"\\40"); 
    strcat(buf, pos+1);
      
    ret = malloc(128);
    strcpy(ret, buf);
    printf("replace @ ==[%s]==\n", ret);
    return ret;
}


//must free return pointer after using it
char* get_valuefrom( char *start, char *name )
{
	char *temp = NULL, *temp2 = NULL, *result;
	int resultLen;
	temp = strstr( start, name );
	if( temp != NULL )
	{
		temp += strlen(name)+1;
		temp2 = strchr( temp ,'\"' );
		if( temp2 != NULL )
		{
			temp2++;
			temp = strchr( temp2, '\"' );
			if( temp != NULL )
			{
				*temp = '\0';
				resultLen = strlen( temp2 ) + 1;
				result = (char*)malloc(resultLen);
				snprintf( result, resultLen, "%s", temp2 );
				*temp = '\"';
				return result;
			}
		}
	}
	return NULL;
}

const char *get_dmcresult_char( REG_STATE state )
{
	if( state == REG_SUCCEED )
		return "REG_SUCCEED";
	else if( state == REG_FAILED )
		return "REG_FAILED";
	else if( state == REG_UPDATED )
		return "REG_UPDATED";
	return "REG_FAILED";
}

#if 0
char *getPIN()
{
	
        static char PIN[MAX_PIN_LEN]="2345678901";

	return PIN;
}

#endif

int setup_connection( char *server_url )
{
    struct addrinfo hints, *res;
    int rc, fd=0;

    memset( &hints, 0, sizeof( hints ) );
    //hints.ai_family = AF_UNSPEC;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if( ( rc = getaddrinfo( server_url, http_port, &hints, &res ) ) != 0 ) {
        printf( "Get server address information failed: %s!\n", gai_strerror( rc ) );
        return -1;
    }

    do {
        //fd = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
        fd = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
        
        struct timeval timeout;  
        timeout.tv_sec = SOCKET_TIMEOUT;
        timeout.tv_usec = 0;  
        socklen_t len = sizeof(timeout);  
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len); 
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, len); 
          
        if( connect( fd, res->ai_addr, res->ai_addrlen ) == 0 ) {
            break;
        } else {
            printf( "Connect to server(%s) failed\n", server_url );
            close( fd );
            return -1;
        }
    } while( ( res = res->ai_next ) != NULL );
    return fd;
}

void http_handler( char *pbuffer, int blen )
{
    char *next, *temp=NULL;
	
	temp = get_valuefrom( buffer, "result_code" );
	if( temp != NULL )
	{
		if( strncmp( temp, "0000", 4 ) == 0 )
		{
			char *username_tmp=NULL, *username=NULL, *password=NULL, *url=NULL;

			//get xmpp info
			username_tmp = get_valuefrom( buffer, "xmpp_account" );
			username = strrep(username_tmp,"@");
			password = get_valuefrom( buffer, "xmpp_password" );
			url = get_valuefrom( buffer, "xmpp_server" );

 			printf("username[%s][%s][%s]", username, password, url);

			if( username != NULL && password != NULL && url != NULL )
			{
				snprintf( dmc_xmpp_un, sizeof(dmc_xmpp_un), "%s", username );
                                snprintf( dmc_xmpp_pw, sizeof(dmc_xmpp_pw), "%s", password );
                                snprintf( dmc_xmpp_url, sizeof(dmc_xmpp_url), "%s", url );

				free( username );
				free( password );
				free( url );							 
				reg_result = REG_SUCCEED;				
			}
		}
		else if( strncmp( temp, "0001", 4 ) == 0 )
		{
			//registration failed
			reg_result = REG_FAILED;
			return;	
		}
		else if( strncmp( temp, "1000", 4 ) == 0 )
		{
			//device info updated
			reg_result = REG_UPDATED;
			return;	
		}
		else
			return ;
	}
	else
		return;

    //if first buffer and header isn't finished
    if( sum == -1 && header_finished == 0 )
    {
        if( (temp = strstr( buffer, "\r\n\r\n" )) != NULL )
		{
			header_finished = 1;
            sum = (int)((buffer+blen-temp)/sizeof(char))-4;
		}
    }
    else
    {
		//add sum
		if( sum == -1 )
			sum = sum + blen + 1;
		else
			sum += blen;
    }
   
    temp = buffer;
    while( (next = strstr( temp, "\r\n" )) != NULL )
    {
        *next='\0';
		//determine the header configuration
        if( strncmp( temp, "Content-Length: ", 16 ) == 0 )
        {
            temp+=16;
            clen=atoi(temp);
        }
        
        temp = next+2;
    }
}
void recv_handler( int fd )
{
    int blen = 0;
    clen = -1;
    sum = -1;
	header_finished = 0;

    blen = recv( fd, buffer, sizeof( buffer ), 0 );
    while( blen > 0 && ( clen < 0 || sum < clen ) )
    {
		//printf("==================\n%s\n================\n", buffer);
        http_handler( buffer, blen );
        memset( buffer, 0, sizeof( buffer ) );
        if( clen == 0 ||(clen > 0 && sum == clen ))
            break;
        blen = recv( fd, buffer, sizeof( buffer ), 0 );
    }
}

int send2DMS( char *dmc_message, int reg_act )
{
    int fd;
    fd = setup_connection( url );
    /* Connect ok, break here */
    int len;
    int res = 0;
    char *from;


    if(reg_act == DMC_REGISTER)
    { 
	printf("==== DMC REGISTER==========\n");	
        len = snprintf( buffer, sizeof( buffer ),
                        "POST /rae?action=register HTTP/1.1\r\n"
                        "Host: %s\r\n" 
                        "Content-Length: %d\r\n"
                        "\r\n"
						"%s"
						"\r\n\r\n",
                        url,
						strlen(dmc_message),
                        dmc_message );
   
    }
    else
    {

	printf("DMC Update\n");
        len = snprintf( buffer, sizeof( buffer ),
                        "POST /rae?action=update HTTP/1.1\r\n"
                        "Host: %s\r\n"
                        "Content-Length: %d\r\n"
                        "\r\n"
                                                "%s"
                                                "\r\n\r\n",
                        url,
                                                strlen(dmc_message),
                        dmc_message );


    }
    from = buffer;
    do {
        len -= res;
        from += res;
        res = send( fd, from, len, 0 );
    } while( res > 0 );

        if( res == 0 ) {
    }

    memset( buffer, 0, sizeof( buffer ) );
    recv_handler( fd );

    close( fd );
    return 0;
}

void dmc_resource( char *addr, OCClientResponse *clientResponse )
{
	char *uri=NULL, *prop=NULL, *temp=NULL, *temp2=NULL;
	char *rescpy;
	int respLen;
	
	if(clientResponse->resJSONPayload !=NULL)	
	    respLen = strlen((char*)(clientResponse->resJSONPayload));
	else 
	    respLen = 0;

	rescpy = (char*)malloc(respLen+1);
	memset(rescpy, 0, respLen+1);

	if(clientResponse->resJSONPayload !=NULL)
	{
	    snprintf( rescpy, respLen , "%s", clientResponse->resJSONPayload);
	}
		    		    	     	
	//get uri
	temp = strstr( rescpy, "\"href\":\"" );
	while( temp != NULL )
	{
		temp = temp + 8;
		temp2 = strchr( temp, '\"');
		if( temp2 != NULL )
		{
			*temp2 = '\0';
			temp2++;
			uri = temp; 
			//get prop
			temp = strstr( temp2, "\"prop\":");
			if( temp != NULL )
			{
				temp = temp + 7;
				temp2 = strchr( temp, '}');
				if( temp2 != NULL )
				{
					struct resources *rtemp;
					temp2++;
					*temp2='\0';
					temp2++;
					prop = temp;
					rtemp = (struct resources*)malloc(sizeof(struct resources));
					snprintf( rtemp->uri, sizeof(rtemp->uri), "%s%s", addr, uri );
					snprintf( rtemp->prop, sizeof(rtemp->prop), "%s", prop );
					rtemp->next = NULL;
					//add into resources list
					if( rhead == NULL )
					{
						rhead = rtemp;
						rcurrent = rtemp;
					}
					else
					{
						rcurrent->next = rtemp;
						rcurrent = rtemp;
					}
					rcount++;
				}
			}
		}
		temp = strstr( temp2, "\"href\":\"" );
	}
	free( rescpy );
}

struct dmc_result *dmc_discovery_finish(int reg_act)
{
	struct dmc_result *res=NULL;
	struct resources *rtemp = rhead, *rtemp2= rhead;
	char *dmc_message=NULL;
	int dmc_message_len = 40+MAX_PIN_LEN+rcount*(MAX_URI_LEN+MAX_PROP_LEN);
	
	//initiate result
	memset( dmc_xmpp_un, 0, sizeof( dmc_xmpp_un ) );
	memset( dmc_xmpp_pw, 0, sizeof( dmc_xmpp_pw ) );
	memset( dmc_xmpp_url, 0, sizeof( dmc_xmpp_url ) );
	reg_result = REG_FAILED;
	
	//generate dmc message
	dmc_message = (char*)malloc(dmc_message_len);
		snprintf( dmc_message, dmc_message_len, "{\"PIN\":\"%s\",\"Res\":[ ", getPIN() );
	while( rtemp != NULL )
	{
		char obj[19+MAX_URI_LEN+MAX_PROP_LEN];
		snprintf( obj, sizeof(obj), "{\"uri\":\"%s\",\"prop\":%s},", rtemp->uri, rtemp->prop );
		strcat( dmc_message, obj );
		rtemp = rtemp->next;
		free(rtemp2);
		rcount--;
		rtemp2 = rtemp;
	}
	rhead = NULL;
	rcurrent = NULL;
	*(dmc_message+strlen(dmc_message)-1)='\0';
	strcat( dmc_message, "]}" );
	
	//send to DMS
	OC_LOG_V( INFO, TAG, "Send dmc_message to DM server %s:%s: %s", url, http_port, send2DMS( dmc_message, (reg_act==DMC_REGISTER) ? DMC_REGISTER : DMC_UPDATE ) == 0 ? "Succeed":"Failed" );
	
	//return to RACLIENT
	res = (struct dmc_result*)malloc( sizeof( struct dmc_result ) );
	memset( res, 0, sizeof( struct dmc_result ) );
	res->state = reg_result;
	if( res->state == REG_SUCCEED )
	{
		snprintf( res->server, sizeof(res->server), "%s", dmc_xmpp_url );
		snprintf( res->username, sizeof(res->username), "%s", dmc_xmpp_un );
		snprintf( res->password, sizeof(res->password), "%s", dmc_xmpp_pw );
	}
	
	free(dmc_message);   
	return res;
}
          
