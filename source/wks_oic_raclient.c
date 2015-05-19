#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wks_oic_raclient.h"


response_code_t resp_code_list[]= 
{
    {RESPONSE_OK, "200 OK"},
    {RESPONSE_CREATED, "201 Created"},
    {RESPONSE_NO_CONTENT, "204 No Content"},
    {RESPONSE_BAD_REQ, "400 Bad Request"},
    {RESPONSE_NULL, NULL },
    {RESPONSE_INTERNAL_ERR, "500 Internal Server Error"},
    {RESPONSE_501, NULL},
    {0, NULL}

};


char*  Insert_Respone_Code(int code, char* const payload)
{
    char* buf;
        
    printf("res = %s\n", resp_code_list[code].resp_str);    
    
    if(payload !=NULL)
    	buf = malloc(strlen(resp_code_list[code].resp_str) \
       	+6+strlen(payload) +1);
    else
	buf = malloc(strlen(resp_code_list[code].resp_str) \
        +6+1);	
    
    strcpy(buf, resp_code_list[code].resp_str);
    printf("buf1=%s\n", buf);
    strcat(buf, "\r\n\r\n");  
    printf("buf2=%s\n", buf);  

    if(payload !=NULL)
    {
        strcat(buf, payload);    	
	printf("buf3=%s\n", buf);
    }
    strcat(buf, "\r\n");
        
    return buf;

}

int cmd_map(char * szCMD)
{

   if(strcmp(szCMD, "GET")==0)
        return COMMAND_GET;

   if(strcmp(szCMD, "PUT")==0)
        return COMMAND_PUT;

   if(strcmp(szCMD, "POST")==0)
        return COMMAND_POST;

   if(strcmp(szCMD, "OBSERVE")==0)
        return COMMAND_OBSERVE;

   return 0;
}
 
