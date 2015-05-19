#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <signal.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "logger.h"
#include "cJSON.h"
#include "dmclient.h"
#include "ocstack.h"

#include <strophe.h>
#include "common.h"

#include "xmppclient.h"



#define TAG PCF("raclient")
#define CTX_VAL 0x99
OCDoHandle gObserveDoHandle;
OCQualityOfService qos_default = OC_LOW_QOS;
char xmpp_un[MAX_XMPPUN_LEN]="";
char xmpp_pw[MAX_XMPPPW_LEN]="";
char xmpp_server[MAX_XMPPSV_LEN]="";
char PIN[MAX_PIN_LEN]="0000";

pthread_attr_t attr1;
pthread_t pid1;

time_t last_discovery_time;
int gQuitFlag = 0;

char *getPIN();
int setPIN(char * szPin);

/* SIGINT handler: set gQuitFlag to 1 for graceful termination */
void handleSigInt(int signum) {

	if (signum == SIGINT) {		
		gQuitFlag = 1;
	}
}

struct cmd *cmd_head=NULL;
struct cmd *cmd_current=NULL;

//set command from TR thread to OIC thread
void oic_set_command( COMMAND_ENUM command, char *coapuri, char *putpayload, xmpp_cbctx_t* cbctx )
{
	struct cmd *cmd_new;
	cmd_new = (struct cmd*)malloc(sizeof(struct cmd));
	cmd_new->command = command;
	snprintf( cmd_new->coapuri, sizeof(cmd_new->coapuri), "%s", coapuri );
	snprintf( cmd_new->putpayload, sizeof(cmd_new->putpayload), "%s", putpayload );
	cmd_new->next = NULL;
        cmd_new->cbctx = cbctx;

	if( !cmd_head )
	{
		cmd_head = cmd_new;
		cmd_current = cmd_new;
	}
	else
	{
		cmd_current->next = cmd_new;
	}


}
const char *getResult(OCStackResult result) 
{
    switch (result) {
    case OC_STACK_OK:
        return "OC_STACK_OK";
    case OC_STACK_RESOURCE_CREATED:
        return "OC_STACK_RESOURCE_CREATED";
    case OC_STACK_RESOURCE_DELETED:
        return "OC_STACK_RESOURCE_DELETED";
    case OC_STACK_INVALID_URI:
        return "OC_STACK_INVALID_URI";
    case OC_STACK_INVALID_QUERY:
        return "OC_STACK_INVALID_QUERY";
    case OC_STACK_INVALID_IP:
        return "OC_STACK_INVALID_IP";
    case OC_STACK_INVALID_PORT:
        return "OC_STACK_INVALID_PORT";
    case OC_STACK_INVALID_CALLBACK:
        return "OC_STACK_INVALID_CALLBACK";
    case OC_STACK_INVALID_METHOD:
        return "OC_STACK_INVALID_METHOD";
    case OC_STACK_NO_MEMORY:
        return "OC_STACK_NO_MEMORY";
    case OC_STACK_COMM_ERROR:
        return "OC_STACK_COMM_ERROR";
    case OC_STACK_INVALID_PARAM:
        return "OC_STACK_INVALID_PARAM";
    case OC_STACK_NOTIMPL:
        return "OC_STACK_NOTIMPL";
    case OC_STACK_NO_RESOURCE:
        return "OC_STACK_NO_RESOURCE";
    case OC_STACK_RESOURCE_ERROR:
        return "OC_STACK_RESOURCE_ERROR";
    case OC_STACK_SLOW_RESOURCE:
        return "OC_STACK_SLOW_RESOURCE";
    case OC_STACK_NO_OBSERVERS:
        return "OC_STACK_NO_OBSERVERS";
    #ifdef WITH_PRESENCE
    case OC_STACK_PRESENCE_DO_NOT_HANDLE:
        return "OC_STACK_PRESENCE_DO_NOT_HANDLE";
    case OC_STACK_PRESENCE_STOPPED:
        return "OC_STACK_PRESENCE_STOPPED";
    #endif
    case OC_STACK_ERROR:
        return "OC_STACK_ERROR";
    default:
        return "UNKNOWN";
    }
}

OCStackApplicationResult obsReqCB(void* ctx, OCDoHandle handle, OCClientResponse * clientResponse) {
    if(ctx == (void*)CTX_VAL)
    {
        OC_LOG_V(INFO, TAG, "Callback Context for OBS query recvd successfully");
    }

    if(clientResponse)
    {
        OC_LOG_V(INFO, TAG, "StackResult: %s",  getResult(clientResponse->result));
        OC_LOG_V(INFO, TAG, "SEQUENCE NUMBER: %d", clientResponse->sequenceNumber);
        OC_LOG_V(INFO, TAG, "Callback Context for OBSERVE notification recvd successfully");
        if((clientResponse->resJSONPayload != NULL))
            OC_LOG_V(INFO, TAG, "JSON = %s =============> Obs Response", clientResponse->resJSONPayload);

        if(clientResponse->sequenceNumber == OC_OBSERVE_REGISTER){
            OC_LOG(INFO, TAG, "This also serves as a registration confirmation");
        }else if(clientResponse->sequenceNumber == OC_OBSERVE_DEREGISTER){
            OC_LOG(INFO, TAG, "This also serves as a deregistration confirmation");
            return OC_STACK_DELETE_TRANSACTION;
        }else if(clientResponse->sequenceNumber == OC_OBSERVE_NO_OPTION){
            OC_LOG(INFO, TAG, "This also tells you that registration/deregistration failed");
            return OC_STACK_DELETE_TRANSACTION;
        }
    }
    return OC_STACK_KEEP_TRANSACTION;
}

//post callback, do nothing
OCStackApplicationResult postReqCB(void* ctx, OCDoHandle handle, OCClientResponse * clientResponse) {

    xmpp_cbctx_t* cbctx = (xmpp_cbctx_t*)ctx;

    if(ctx == (void*)CTX_VAL)
    {
        OC_LOG_V(INFO, TAG, "Callback Context for PUT recvd successfully");
    }

    if(clientResponse)
    {
        OC_LOG_V(INFO, TAG, "StackResult: %s",  getResult(clientResponse->result));
	 if( clientResponse->resJSONPayload != NULL)
        {

            OC_LOG_V(INFO, TAG, "JSON = %s =============> Put Response", clientResponse->resJSONPayload);
            XMPP_IBB_SendPayload(cbctx->conn, cbctx->stanza, cbctx->ctx, clientResponse);
	}
	else
	    OC_LOG_V(INFO, TAG, " response NULL");

    }
    return OC_STACK_DELETE_TRANSACTION;
}
              



//put callback, do nothing
OCStackApplicationResult putReqCB(void* ctx, OCDoHandle handle, OCClientResponse * clientResponse) {

    xmpp_cbctx_t* cbctx = (xmpp_cbctx_t*)ctx;

    if(ctx == (void*)CTX_VAL)
    {
        OC_LOG_V(INFO, TAG, "Callback Context for PUT recvd successfully");
    }

    if(clientResponse)
    {
        OC_LOG_V(INFO, TAG, "StackResult: %s",  getResult(clientResponse->result));
	if( clientResponse->resJSONPayload != NULL)
	{
            OC_LOG_V(INFO, TAG, "JSON = %s =============> Put Response", clientResponse->resJSONPayload);
   
            XMPP_IBB_SendPayload(cbctx->conn, cbctx->stanza, cbctx->ctx, clientResponse);
	}
	else
	    OC_LOG_V(INFO, TAG, " response NULL");
		

    }
    return OC_STACK_DELETE_TRANSACTION;
}

//get callback, write representation to TR parameter
OCStackApplicationResult getReqCB(void* ctx, OCDoHandle handle, OCClientResponse * clientResponse) {


    xmpp_cbctx_t* cbctx = (xmpp_cbctx_t*)ctx;


    if(clientResponse)
    {
        OC_LOG_V(INFO, TAG, "StackResult: %s",  getResult(clientResponse->result));
        OC_LOG_V(INFO, TAG, "SEQUENCE NUMBER: %d", clientResponse->sequenceNumber);
	if( clientResponse->resJSONPayload != NULL)
	{
        	OC_LOG_V(INFO, TAG, "JSON = %s =============> Get Response", clientResponse->resJSONPayload);
	
		XMPP_IBB_SendPayload(cbctx->conn, cbctx->stanza, cbctx->ctx, clientResponse);
	}
       else
		OC_LOG_V(INFO, TAG, "response NULL");		         		
    }

    free(ctx);
    
    return OC_STACK_DELETE_TRANSACTION;
}

//discovery callback
OCStackApplicationResult discoveryReqCB(void* ctx, OCDoHandle handle,
        OCClientResponse * clientResponse) {
    uint8_t remoteIpAddr[4];
    uint16_t remotePortNu;

    if (ctx == (void*) CTX_VAL)
    {
        OC_LOG_V(INFO, TAG, "Callback Context for DISCOVER query recvd successfully");
	}
    if(clientResponse)
    {
        OC_LOG_V(INFO, TAG, "StackResult: %s", getResult(clientResponse->result));

        OCDevAddrToIPv4Addr((OCDevAddr *) clientResponse->addr, remoteIpAddr,
                remoteIpAddr + 1, remoteIpAddr + 2, remoteIpAddr + 3);
        OCDevAddrToPort((OCDevAddr *) clientResponse->addr, &remotePortNu);

	if(clientResponse->resJSONPayload !=NULL)
	{
        
	    OC_LOG_V(INFO, TAG,
            "Device =============> Discovered %s @ %d.%d.%d.%d:%d",
            clientResponse->resJSONPayload, remoteIpAddr[0], remoteIpAddr[1],
            remoteIpAddr[2], remoteIpAddr[3], remotePortNu);
				
	    //DM client
	    char addr[MAX_URI_LEN]="";
	    snprintf(addr, sizeof(addr), "coap://%d.%d.%d.%d:%d", remoteIpAddr[0], remoteIpAddr[1], remoteIpAddr[2], remoteIpAddr[3], remotePortNu);
	    dmc_resource( addr, clientResponse );
	}
	else
	{
		OC_LOG_V(INFO, TAG, " response NULL");
	}

    }
    return OC_STACK_KEEP_TRANSACTION ;
}

OCStackResult InvokeOCDoResource(char *query,
                                 OCMethod method, OCQualityOfService qos,
                                 OCClientResponseHandler cb, OCHeaderOption * options, uint8_t numOptions, struct cmd *command)
{
    OCStackResult ret;
    OCCallbackData cbData;
    OCDoHandle handle;

    cbData.cb = cb;
//    cbData.context = (void*)CTX_VAL;
    cbData.context = (void *)(command->cbctx);
    cbData.cd = NULL;
	
        printf("query=%s\n", query);

	ret = OCDoResource(&handle, method, query, 0,
                (method == OC_REST_PUT) ? command->putpayload : NULL,
                qos, &cbData, options, numOptions);

	if (ret != OC_STACK_OK)
	{
		OC_LOG_V(ERROR, TAG, "OCDoResource returns error %d with method %d", ret, method);
	}
	else if (method == OC_REST_OBSERVE || method == OC_REST_OBSERVE_ALL)
	{
		gObserveDoHandle = handle;
	}

    return ret;
}

int InitPostRequest( struct cmd *command )
{
    int queryLen= strlen( command->coapuri)+1;
    char *query = malloc( queryLen );
    int returnVal;

    OC_LOG_V(INFO, TAG, "\n\nExecuting %s", __func__);
        snprintf( query, queryLen, "%s", command->coapuri );
    returnVal = InvokeOCDoResource( query, OC_REST_PUT, OC_LOW_QOS, putReqCB, NULL, 0, command);
    free(query);
    return returnVal;
}


int InitPutRequest( struct cmd *command )
{
    int queryLen= strlen( command->coapuri)+1;
    char *query = malloc( queryLen );
    int returnVal;
	
    OC_LOG_V(INFO, TAG, "\n\nExecuting %s", __func__);
	snprintf( query, queryLen, "%s", command->coapuri );
    returnVal = InvokeOCDoResource( query, OC_REST_PUT, OC_LOW_QOS, putReqCB, NULL, 0, command);
    free(query);
    return returnVal;
}

int InitGetRequest( struct cmd *command )
{
    int queryLen = strlen( command->coapuri )+1;
   
    char *query = malloc( queryLen );
    OCStackResult result;

    printf("query len=%d\n", queryLen);

    OC_LOG_V(INFO, TAG, "\n\nExecuting %s", __func__);
	snprintf( query, queryLen+1, "%s", command->coapuri );

	OC_LOG_V(INFO, TAG, "query in InitGetRequest: %s", command->coapuri);

    result = InvokeOCDoResource(query, OC_REST_GET, OC_HIGH_QOS, getReqCB, NULL, 0, command);
    free(query);

    return result;
}

int InitDiscovery()
{
    OCStackResult ret;
    OCCallbackData cbData;
    OCDoHandle handle;

	/* Start a discovery query*/
    char szQueryUri[64] = { 0 };
    strcpy(szQueryUri, OC_WELL_KNOWN_QUERY);

    cbData.cb = discoveryReqCB;
    cbData.context = (void*)CTX_VAL;
    cbData.cd = NULL;
    ret = OCDoResource(&handle, OC_REST_GET, szQueryUri, 0, 0, OC_LOW_QOS, &cbData, NULL, 0);
    if (ret != OC_STACK_OK)
    {
        OC_LOG_V(ERROR, TAG, "OCStack resource error");
    }
	
    return ret;
}

int main( int argc, char* argv[] )
{
    uint8_t addr[20] = {0};
    uint8_t* paddr = NULL;
    uint16_t port = USE_RANDOM_PORT;
    xmpp_ctx_t *ctx = NULL;
    xmpp_conn_t* conn = NULL;    
    int reg_act = DMC_REGISTER;
    int errornum=0;
    void *ptr;

    pthread_attr_init(&attr1);
    pthread_attr_setstacksize(&attr1, PTHREAD_STACK_MIN * 2);
   	
    if(!setPIN("2345678901"))
    { 	
	fprintf(stderr, "PIN read error\n");
	return 1;
    }	
    else
	printf("\t\tRead PIN : %s\n", getPIN());

    if( argc < 2 )
    {
	exit(-1);
    }
    uint8_t *ifname = (uint8_t *)argv[1];
	
    /*Get Ip address on defined interface and initialize coap on it with random port number
     * this port number will be used as a source port in all coap communications*/
    if ( OCGetInterfaceAddress(ifname, sizeof(ifname), AF_INET, addr,
                sizeof(addr)) == ERR_SUCCESS)
    {
        OC_LOG_V(INFO, TAG, "Starting occlient on address %s",addr);
        paddr = addr;
    }

    /* Initialize OCStack*/
    if (OCInit((char *) paddr, port, OC_CLIENT) != OC_STACK_OK) {
        OC_LOG(ERROR, TAG, "OCStack init error");
        return 0;
    }

    time(&last_discovery_time);
    InitDiscovery();
//    conn = XMPP_Init( xmpp_un , xmpp_pw ,  xmpp_server, &ctx);
       
    // Break from loop with Ctrl+C
    OC_LOG(INFO, TAG, "Entering occlient main loop...");

    signal(SIGINT, handleSigInt);

    while (!gQuitFlag) 
	{	
		
		if( cmd_head != NULL )
		{
			switch( cmd_head->command )
			{
				case COMMAND_DISCOVERY:
					InitDiscovery();
					break;
				case COMMAND_GET:
					InitGetRequest(cmd_head);
					break;
				case COMMAND_PUT:
					InitPutRequest(cmd_head);
					break;
				case COMMAND_POST:
                                        InitPutRequest(cmd_head);
                                        break;
				case COMMAND_OBSERVE:
					break;
				default:
					break;
			}
		
			struct cmd *cmd_temp = cmd_head;
			cmd_head = cmd_head->next;
			free(cmd_temp);
		}
		else
		{
			//do discovery every 10 seconds
			time_t now;
			time(&now);
			if( now - last_discovery_time > DMC_INTERVAL )
			{
				struct dmc_result *res = dmc_discovery_finish(reg_act);                                			
				if( res->state == REG_SUCCEED )
				{
					reg_act = DMC_UPDATE;
					snprintf( xmpp_un, sizeof(xmpp_un), "%s", res->username );
					snprintf( xmpp_pw, sizeof(xmpp_pw), "%s", res->password );
					snprintf( xmpp_server, sizeof(xmpp_server), "%s", res->server );
					free( res );
					printf("%s\n%s\n%s\n", xmpp_server, xmpp_un, xmpp_pw);
					if(conn==NULL)
 					{ 

                                            conn = XMPP_Init( \
					    xmpp_un , xmpp_pw,
                                            xmpp_server, &ctx);
         
                                            if(ctx !=NULL)
                                                pthread_create(&pid1, \
				&attr1, (void*)xmpp_run, (void*)ctx );
                                            else 
						printf("ctx null\n");

					}
				}
				else
					free(res);	

				OC_LOG_V( INFO, TAG, "DM client returns state %s", get_dmcresult_char(res->state) );
				InitDiscovery();
				time(&last_discovery_time);
			}
		}

        if (OCProcess() != OC_STACK_OK) 
		{
            OC_LOG(ERROR, TAG, "OCStack process error");
            return 0;
        }

        usleep(200*1000);
    }
    
    printf("Quit\n");	
     
    OC_LOG(INFO, TAG, "Exiting occlient main loop...");
    
    if (OCStop() != OC_STACK_OK) {
        OC_LOG_V(ERROR, TAG, "OCStack stop error");
    }

    xmpp_stop(ctx);
    sleep(1);
    XMPP_Close(ctx, conn);
   

    if ((errornum = pthread_cancel(pid1)) != 0)
      	OC_LOG_V(INFO, TAG, "pthread_cancel: %s", strerror(errornum));
    if ((errornum = pthread_join(pid1, &ptr)) != 0)
	OC_LOG_V(INFO, TAG, "pthread_join: %s", strerror(errornum));


    printf("Done\n");
    return 0;
}

char *getPIN()
{
        return PIN;
}


int setPIN(char * szPin)
{
	if(strlen(szPin)+1 < MAX_PIN_LEN)
            strcpy( PIN, szPin);
	else
	    return 0;
        
}
 
