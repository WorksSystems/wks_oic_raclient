#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ocstack.h"
#include "dmclient.h"
#include "xmppclient.h"

#include <strophe.h>
#include "common.h"



int iq_ibb_open_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
    char*  szBlock_size;
    char*  szSid;


    if( xmpp_stanza_get_child_by_name(stanza, "open") != NULL)
    {

        printf(" =====XEP0047 Open Handle\n");

        szBlock_size =  \
            xmpp_stanza_get_attribute(  xmpp_stanza_get_child_by_name(stanza, "open"), "block-size");
        szSid =  \
            xmpp_stanza_get_attribute(xmpp_stanza_get_child_by_name(stanza, "open"), "sid");
    printf("XEP0047 IQ blocksize=%s sid=%s \n",szBlock_size, szSid );
        XMPP_IBB_Ack_Send(conn, stanza, userdata);


    }
    else  if( xmpp_stanza_get_child_by_name(stanza, "data") != NULL)
    {
        XMPP_IBB_Ack_Send(conn, stanza, userdata);
        printf("========XEP0047 Data process\n");

        XMPP_IBB_data_process(conn, stanza  , userdata);

    }
    else if( xmpp_stanza_get_child_by_name(stanza, "close")  )
    {
        XMPP_IBB_Ack_Send(conn, stanza, userdata);
    }
    return 1;
}


int XMPP_IBB_data_process(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{

    char* intext;
    unsigned char *result;
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;

    char szMethod[MAX_METHOD_LEN];
    char szUri[MAX_COAP_URI_LEN];
    char szPayload[MAX_JSON_PAYLOAD_LEN];
    char szHeader[MAX_HEAD_LEN];


    printf("\nxep0047 data handler\n");

    intext = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stanza, "data"));

    printf("[Data=%s]\n", intext);

    result = base64_decode(ctx, intext, strlen(intext));
    printf("Decode result=%s\n", result);

    sscanf(result, "%s%s%[^{]\n%s", szMethod, szUri, szHeader, szPayload);

    if((strcmp(szMethod, "PUT")!=0) &&  \
         (strcmp(szMethod, "POST")!=0))
         szPayload[0] = NULL;

    printf("=====[%s][%s][%s][%s]====", szMethod,szUri, szHeader,szPayload);

    xmpp_cbctx_t* xmpp_cbctx = malloc(sizeof(xmpp_cbctx_t));
    xmpp_cbctx->conn = conn;
    xmpp_cbctx->ctx = ctx;
    xmpp_cbctx->stanza = xmpp_stanza_copy(stanza);

    oic_set_command( cmd_map(szMethod), szUri, szPayload, xmpp_cbctx );

    xmpp_free(ctx, intext);
    xmpp_free(ctx, result);
    return 1;
}

void XMPP_IBB_SendPayload(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata,OCClientResponse* ocresp )
{

    static int seq = 0;
    int data_seq = 0;
//    char* buf;
    char Data_Seq_Buf[32];
    char ID_Buf[32];
    char *resp;
    char* encoded_data;
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stanza_t *iq, *data,  *text;

    iq  = xmpp_stanza_new(ctx);
    data = xmpp_stanza_new(ctx);
    text = xmpp_stanza_new(ctx);

    xmpp_stanza_set_name(iq, "iq");
    xmpp_stanza_set_type(iq, "set");

    sprintf(ID_Buf, "ID-seq-%d", seq);
    seq++;
    xmpp_stanza_set_id(iq, ID_Buf);
    xmpp_stanza_set_attribute(iq, "to", xmpp_stanza_get_attribute(stanza, "from"));
    xmpp_stanza_set_attribute(iq, "from", xmpp_stanza_get_attribute(stanza, "to"));

    xmpp_stanza_set_name(data, "data");

    xmpp_stanza_set_ns(data, "http://jabber.org/protocol/ibb");

    xmpp_stanza_set_attribute(data, "sid",   \
    xmpp_stanza_get_attribute(xmpp_stanza_get_child_by_name(stanza, "data"), "sid"));
  
    sprintf(Data_Seq_Buf , "%d", data_seq);
    xmpp_stanza_set_attribute(data, "seq", Data_Seq_Buf);

    
    if(ocresp->resJSONPayload == NULL)
    {
        resp = Insert_Respone_Code(RESPONSE_NO_CONTENT,NULL);
    }
    else
    {      
        resp = Insert_Respone_Code(RESPONSE_OK,ocresp->resJSONPayload);

    }


    printf("\n[Response =%s]\n", resp);
    encoded_data = base64_encode(ctx, (unsigned char*)resp, strlen(resp));

    xmpp_stanza_set_text_with_size(text, encoded_data, strlen(encoded_data));
    xmpp_stanza_add_child(data, text);
    xmpp_stanza_add_child(iq, data);
    xmpp_send(conn, iq);
    seq++;

    free(resp);
    xmpp_free(ctx, encoded_data);

    xmpp_stanza_release(iq);
    xmpp_stanza_release(stanza);  //copied by IBB IQ receiver handler

}

void XMPP_IBB_Ack_Send(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stanza_t *iq;

    iq  = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, "iq");
    xmpp_stanza_set_type(iq, "result");

    xmpp_stanza_set_id(iq, xmpp_stanza_get_attribute(stanza, "id"));
    xmpp_stanza_set_attribute(iq, "to", xmpp_stanza_get_attribute(stanza, "from"));
    xmpp_stanza_set_attribute(iq, "from", xmpp_stanza_get_attribute(stanza, "to"));
    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);
}

/* XMPP_IBB_Close_Send() has not been verified. */
void XMPP_IBB_Close_Send(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
   
#if 0
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stanza_t *iq;

    iq  = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iq, "iq");
    xmpp_stanza_set_type(iq, "set");

    xmpp_stanza_set_id(iq, xmpp_stanza_get_attribute(stanza, "id"));
    xmpp_stanza_set_attribute(iq, "to", xmpp_stanza_get_attribute(stanza, "from"));
    xmpp_stanza_set_attribute(iq, "from", xmpp_stanza_get_attribute(stanza, "to"));

    xmpp_send(conn, iq);
    xmpp_stanza_release(iq);

#endif

}

