#include "ocstack.h"
#include "wks_oic_raclient.h"

#define BUFFER_SIZE     2048
#define SOCKET_TIMEOUT  5
#define RETURN_NULL(x) if ((x)==NULL) return;
#define RETURN_ERR(err,s) if ((err)==-1) { perror(s); return; }
#define RETURN_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); return; }

enum {
	DMC_REGISTER =0,
 	DMC_UPDATE
};


void dmc_resource( char *addr, OCClientResponse *clientResponse );
struct dmc_result *dmc_discovery_finish();
const char *get_dmcresult_char(REG_STATE state);

struct resources
{
	char uri[MAX_URI_LEN];
	char prop[MAX_PROP_LEN];
	struct resources *next;
};
