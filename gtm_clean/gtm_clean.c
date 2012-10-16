#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "gtm/gtm_c.h"
#include "gtm/gtm_client.h"
#include "gtm/libpq-fe.h"

typedef enum command_t
{
	CMD_INVALID = 0,
	CMD_UNREGISTER
} command_t;

char *progname;
command_t command;

char	*nodename = NULL;
int gtmport = -1;
char *gtmhost = NULL;
char *myname = NULL;
#define DefaultName "pgxc_clean_gtm"
GTM_PGXCNodeType nodetype = 0;	/* Invalid */
int verbose = 0;

static int process_unregister_command(GTM_PGXCNodeType type, char *nodename);

int main(int argc, char *argv[])
{

	int opt;
	int rc;

	progname = strdup(argv[0]);

	while ((opt = getopt(argc, argv, "p:h:n:Z:v")) != -1)
	{
		switch(opt)
		{
			case 'p':
				gtmport = atoi(optarg);
				break;
			case 'h':
				if (gtmhost) free(gtmhost);
				gtmhost = strdup(optarg);
				break;
			case 'n':
				if (myname) free(myname);
				myname = strdup(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'Z':
				if (strcmp(optarg, "gtm") == 0)
				{
					nodetype = GTM_NODE_GTM;
					break;
				}
				else if (strcmp(optarg, "gtm_proxy") == 0)
				{
					nodetype = GTM_NODE_GTM_PROXY;
					break;
				}
				else if (strcmp(optarg, "gtm_proxy_postmaster") == 0)
				{
					nodetype = GTM_NODE_GTM_PROXY_POSTMASTER;
					break;
				}
				else if (strcmp(optarg, "coordinator") == 0)
				{
					nodetype = GTM_NODE_COORDINATOR;
					break;
				}
				else if (strcmp(optarg, "datanode") == 0)
				{
					nodetype = GTM_NODE_DATANODE;
					break;
				}
				else
				{
					fprintf(stderr, "%s: Invalid -Z option value, %s\n", progname, optarg);
					exit(2);
				}
				break;
			default:
				fprintf(stderr, "%s: Invalid option %c\n", progname, opt);
				exit(2);
		}
	}
	if (myname == NULL)
		myname = strdup(DefaultName);
	
	if (optind >= argc)
	{
		fprintf(stderr,"%s: No command specified.\n", progname);
		exit(2);
	}
	if (strcmp(argv[optind], "unregister") == 0)
	{
		command = CMD_UNREGISTER;
		if (optind + 1 >= argc)
		{
			fprintf(stderr, "%s: unregister: node name missing\n", progname);
			exit(2);
		}
		nodename = strdup(argv[optind + 1]);
	}
	else
	{
		fprintf(stderr, "%s: Invalid command %s\n", progname, argv[optind]);
		exit(2);
	}
	if (gtmport == -1)
	{
		fprintf(stderr, "%s: GTM port number not specified.\n", progname);
		exit(2);
	}
	if (gtmhost == NULL)
	{
		fprintf(stderr, "%s: GTM host name not specified.\n", progname);
		exit(2);
	}

	switch(command)
	{
		case CMD_UNREGISTER:
			if (nodetype == 0)
			{
				fprintf(stderr, "%s: unregister: -Z option not specified.\n", progname);
				exit(2);
			}
			rc = process_unregister_command(nodetype, nodename);
			break;
		default:
			fprintf(stderr, "%s: Internal error, invalid command.\n", progname);
			exit(1);
	}
	exit(rc);
}

static GTM_Conn *connectGTM()
{
	char connect_str[256];
	GTM_Conn *conn;

	sprintf(connect_str, "host=%s port=%d node_name=%s remote_type=%d postmaster=0",
			gtmhost, gtmport, myname == NULL ? DefaultName : myname, GTM_NODE_COORDINATOR);
	if ((conn = PQconnectGTM(connect_str)) == NULL)
	{
		fprintf(stderr, "%s: Could not connect to GTM\n", progname);
		return(NULL);
	}
	return(conn);
}

static int process_unregister_command(GTM_PGXCNodeType type, char *nodename)
{
	GTM_Conn *conn;
	int res;
	
	conn = connectGTM();
	if (conn == NULL)
	{
		fprintf(stderr, "%s: failed to connect to GTM\n", progname);
		return 1;
	}
	res = node_unregister(conn, type, nodename);
	if (res == GTM_RESULT_OK){
		if (verbose)
			fprintf(stderr, "%s: unregister %s from GTM.\n", progname, nodename);
		GTMPQfinish(conn);
		return GTM_RESULT_OK;
	}
	else
	{
		fprintf(stderr, "%s: Failed to unregister %s from GTM.\n", progname, nodename);
		GTMPQfinish(conn);
		return res;
	}
}
