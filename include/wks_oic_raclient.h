#include "strophe.h"

#define DMC_INTERVAL 10
#define MAX_URI_LEN 256
#define MAX_PROP_LEN 256
#define MAX_PAYLOAD_LEN 256
#define MAX_XMPPUN_LEN 128
#define MAX_XMPPPW_LEN 128
#define MAX_XMPPSV_LEN 128
#define MAX_JID_LEN    255
#define MAX_PIN_LEN 64

#define TAG PCF("raclient")
#define CTX_VAL 0x99

typedef enum {

   RESPONSE_OK, 	    
   RESPONSE_CREATED,
   RESPONSE_NO_CONTENT,
   RESPONSE_BAD_REQ,
   RESPONSE_NULL,
   RESPONSE_INTERNAL_ERR,
   RESPONSE_501
} RESPONSE_CODE; 


typedef struct _response_code_t {

  int code;
  char* resp_str;

} response_code_t ;


typedef struct _xmpp_cbctx_t {

  xmpp_conn_t* conn;
  xmpp_ctx_t*  ctx;
  xmpp_stanza_t *stanza;  

} xmpp_cbctx_t;


typedef enum {
	COMMAND_NONE,
    	COMMAND_DISCOVERY,
    	COMMAND_GET,
    	COMMAND_PUT,
	COMMAND_POST,	
	COMMAND_OBSERVE
} COMMAND_ENUM;

struct cmd {
	COMMAND_ENUM command;
	char coapuri[MAX_URI_LEN];
	char putpayload[MAX_PAYLOAD_LEN];
        xmpp_cbctx_t *cbctx;
	struct cmd *next;
};

typedef enum {
	REG_SUCCEED,
	REG_FAILED,
	REG_UPDATED
} REG_STATE;

struct dmc_result{
	char username[MAX_XMPPUN_LEN];
	char password[MAX_XMPPPW_LEN];
	char server[128];
	REG_STATE state;
};

void oic_set_command( COMMAND_ENUM command, char *coapuri, char *payload, xmpp_cbctx_t*);

char* Insert_Respone_Code(int code, char* const payload);
int cmd_map(char*);

