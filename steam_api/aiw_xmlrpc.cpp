/* A simple synchronous XML-RPC client written in C. */

#include <stdlib.h>
#include <stdio.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>
#include <xmlrpc-c/winconfig.h>  /* information about this build environment */
#include <xmlrpc-c/base_int.h> 

#include "stdafx.h"

#pragma comment(lib, "../xmlrpc/lib/xmlrpc.lib")
#pragma comment(lib, "../xmlrpc/lib/xmlparse.lib")
#pragma comment(lib, "../xmlrpc/lib/xmltok.lib")


#define NAME "XML-RPC AIW client"
#define VERSION "1.0"
int	RPCINIT=0;

/* Start up our XML-RPC client library. */
void RPCinit()
{
    xmlrpc_client_init(XMLRPC_CLIENT_NO_FLAGS, NAME, VERSION);
	RPCINIT=1;
}

/* Shutdown our XML-RPC client library. */
void RPCexit()
{
    xmlrpc_client_cleanup();
}

int RPCexec_getUSstate(char *url, char *func, int state)
{
    const char	  	 *stateName;
    xmlrpc_value *resultP;
	xmlrpc_env   RPCenv;

	if (RPCINIT==0)  RPCinit();
    /* Initialize our error-handling environment. */
    xmlrpc_env_init(&RPCenv);

	/* Call the famous server at UserLand. */
    resultP = xmlrpc_client_call(&RPCenv, url, func, "(i)", (xmlrpc_int32) state);
	if (RPCenv.fault_occurred)  {
	    xmlrpc_env_clean(&RPCenv);		// deallocate memory for errorstring
		return 0;
	}
    /* Get our state name and print it out. */
    xmlrpc_read_string(&RPCenv, resultP, &stateName);
	if (RPCenv.fault_occurred)  {
	    xmlrpc_env_clean(&RPCenv);		// deallocate memory for errorstring
		return 0;
	}
    Logger(lINFO,"AIW_XMLRPC","USstate=%s", stateName);
    free((char*)stateName);
    /* Dispose of our result value. */
    xmlrpc_DECREF(resultP);
	/* Clean up our error-handling environment. */
    xmlrpc_env_clean(&RPCenv);
	return 1;
}

/****** main test
int main(int  argc, char **const argv ATTR_UNUSED) 
{
    RPCinit();
	RPCexec_getUSstate("http://betty.userland.com/RPC2","examples.getStateName",40);
    RPCexit();
    return 0;
}
****************/