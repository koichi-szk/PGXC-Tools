#include "parse.h"
#include <getopt.h>


typedef struct command_t {
	int		background;
	pid_t	pid;
	int		exit_code;
	char	*inf;
	char	*outf;
} command_t;


/* For general input */
inputData_t *inputData = NULL;

/* Log File */
FILE *logf = NULL;

static void do_init();
#define cmdname(c) wvalue(elementVal(arrayElement_((c), 0)))

/*
 * Synopsis
 *
 * pgxc_ctl []options ....] [file ....]
 *
 * Options
 *
 * -f file: input from file, not fom stdin.
 * -c config file: configuration file.   Default is $HOME/pgxc/pgxcConf
 * -v, --verbose : Verbose mode.  Default
 * -S, --silent : Silent mode.
 * -V, --version : Prints version, then exit.
 * -h, --help : Prints help, then exit.
 * --with-log: Write log.   Default.
 * --without-log: Do not write log.
 *
 * If no file nor -f option is specified, then command will be read from the terminal or stdin.
 */
main(int argc, char *argv[])
{
	int c;

	char *verbose_opt = NULL;
	char *file_opt = NULL;
	char *log_opt = NULL;
	char *conf_file = NULL;
	char buf[1024];

	static struct option long_options[] = {
		{"silent",	no_argument, 0, 's'},
		{"verbose", no_argument, 0, 'v'},
		{"version", no_argument, 0, 'V'},
		{"help", no_argument, 0, 'h'},
		{"with-log", no_argument, 0, 1},
		{"without-log", no_argument, 0, 2}
	};

	do_minimum_init();
	while (1) {
		int option_index;

		c = getopt_long(argc, argv, "f:c:vSVh",
						long_options, &option_index);
		switch(c) {
			case 'f':
				if (file_opt)
					free(file_opt);
				file_opt = strdup(optarg);
				break;
			case 'c':
				if (conf_file)
					free(conf_file);
				conf_file = strdup(optarg);
				break;
			case 's':
				if (verbose_opt)
					free(verbose_opt);
				verbose_opt = strdup("off");
				break;
			case 'v':
				if (verbose_opt)
					free(verbose_opt);
				verbose_opt = strdup("on");
				break;
			case 'V':
				print_version();
				exit(0);
			case 'h':
				print_help();
				exit(0);
			case 1:
				if (log_opt)
					free(log_opt);
				log_opt = strdup("on");
				break;
			case 2:
				if (log_opt)
					free(log_opt);
				log_opt = strdup("off");
				break;
			default:
				fprintf(stderr, "?? getopt returned character code 0x%x ??\n", c);
		}
	}
	if (conf_file)
		read_config(conf_file);
	else
	{
		sprintf(buf, "%s/pgxc/pgxcConf", getenv("HOME"));
		read_config(buf);
	}
	
}

void do_minimum_init(void)
{
	char buf[1024];
	variable_t *variable;
	/*
	 * Initialize set of variables
	 */
	/* HOME */
	add_variable("HOME", makeWordValue(getenv("HOME")));
	add_variable("USER", makeWordValue(getenv("USER")));
}

static int cmd_end(value_t *token);

void read_config(char *conff)
{
	inputData_t configInput = NULL;
	array_t *cmd = NULL;
	value_t *token = NULL;
	
	confInput = pushInputFileName(configInput, conff);
	while (1)
	{
		if (cmd)
			free_array(cmd);
		for (token = getValue(configInput); ;token = getValue(configInput))
		{
			cmd = append_to_array(cmd, token);
			if (cmd_end(token))
			{
				makeArrayIndex(cmd);
				do_command(cmd, false);
				if (configInput->inf->eof_flag)
					return;
				else
					continue;
			}
		}
	}
}

static int cmd_end(value_t *token)
{
	if (value_type(token) == V_EOL || value_type(token) == V_EOF || value_type(token) == V_COMMENT)
		return true;
	if (value_type(token) == V_SPECIAL && spvalue(token) == ';')
		return true;
	return false;
}

/*
 * background -> specified command should run as backgroud, then
 * sync termination with syn_command.  Otherwise, waits until the
 * command is done, set exit code and then returns.
 *
 * If interactive is false, it does not echo any prompt to the terminal.
 * No output will be written to the terminal.
 * Mainly done at initialization/configuration time.
 */
command_t *do_command(array_t *cmd, int bg_opt)
{
	int ii;
	command_t *rv;

	cmd = trimLastIfNeeded(cmd);
	log_command(cmd);
	ii = arraySize(cmd);
	if (ii == 0) return NULL;
	if (!isCmdName(arrayElement_(cmd, 0)))
	{
		/* Error */ 
		error();
	}
	if (ii == 3 && value_type(elementVal(arrayElement_(cmd, 1))) == V_SPECIAL && spvalue(elementVal(arrayElement_(cmd, 1))) == '=')
	{
		/* Assign command is always done in foreground */
		rv = do_assign_cmd(cmd);
	}
	else if (strcmp(cmdname(cmd), "cd") == 0)
	{
		/* cd is also done in foreground */
		rv = do_cd_cmd(cmd);
	}
	else if (strcmp(cmdname(cmd), "echo") == 0)
	{
		/* Echo is done in foreground */
		rv = do_echo_cmd(cmd);
	}
	else
	{
		/* General command */
		rv = do_command(cmd, bg_opt);
	}
	free_array(cmd);
	return rv;
}


command_t *do_assign_cmd(array_t *cmd)
{
	command_t *rv = Malloc(sizeif(command_t));

	assignVariableByName(wvalue(elementVal(arrayElement_(cmd, 0))), elementVal(arrayElement_(cmd, 2)));
	memcpy(rv, 0, sizeof(command_t));
	return(rv);
}

command_t *do_cd_cmd(array_t *cmd)
{
	int ii = arraySize(cmd);
	int rc;
	arrayElement_t *e;
	command_t *rv = Malloc(sizeof(command_t));

	if (ii != 2)
		/* size error */
		error();
	switch (value_type(elementVal(arrayElement_(cmd, 1)))) {
		case V_UNDEF:
		case V_NUMERIC:
		case V_SPECIAL:
		case V_ARRAY:
			/* Error */
			error();
			break;
		default:
			convertElementToWord(arrayElement_(cmd, 1));
			rc = chdir(wvalue(arrayVal(arrayElement_(cmd, 1))));
			if (rc < 0)
			{
				/* Error */
				error();
			}
	}
	rv->background = false;
	rv->pid = 0;
	rv->exit_code = rc ? 1 : 0;
	rv->inf = rv->outf = NULL;
	return(rv);
}

/* 
 * Echo will also be written to the log.  No -n option supported.
 */
command_t *do_echo_cmd(array_t *cmd)
{
	arrayElement_t *e;
	for (e = arrayElement(cmd, 1); e; e = e->next)
	{
		echo_value(elementVal(e));
		echo_string(" ");
	}
	echo_string("\n");
}

void echo_value(value_t *v)
{
	char buf[128];

	if (v == NULL)
		return;
	switch(value_type(v)) {
		case V_UNDEF:
		case V_COMMENT:
			return;
		case V_NUMERIC:
			sprintf(buf, "%d", ivalue(v));
			echo_string(buf);
			return;
		case V_WORD:
			echo_string(wvalue(v));
			return;
		case V_STRING:
			echo_string("\"");
			echo_string(svalue(v));
			echo_string("\"");
			return;
		case V_RAWSTRING:
			echo_string("'");
			echo_string(rsvalue(v));
			echo_string("'");
			return;
		case V_VARIABLE:
			echo_value(findVariable(vvalue(v))->value);
			return;
		case V_ARRAY:
			echo_array(avalue(v));
			return;
		case V_SPECIAL:
			buf[0] = spvalue(v);
			buf[1] = 0;
			echo_string(buf);
			return;
		default:
			/* Internal error */
			return;
	}
}

void echo_array(array_t *a)
{
	arrayElement_t *e;
	echo_string("(");
	for (e = a->head; e; e = e->next)
	{
		echo_value(e->val);
	}
	echo_string(")");
	return;
}

void log_command(array_t *cmd)
{
	arrayElement_t *e;
	for (e = a->head; e; e = e->next)
	{
		log_value(e->val);
	}
	return;
}

echo_string(char *s)
{
	fputs(stdout, s);
	if (logf)
		fputf(logf, s);
}


void sync_command(command_t *cmd)
{
	if (cmd->background == false)
		return;
	
}

int isCmdName(arrayElement_t *e)
{
	switch (value_type(elementVal(e))) {
		case V_UNDEF:
		case V_SPECIAL:
		case V_ARRAY:
		case V_NUMERIC:
			return false;
		default:
			convertElementToWord(e);
			return true;
	}
}

void trimLastIfNeeded(array_t *cmd)
{
	int sz = arrayAize(cmd);
	arrayElement_t *e;

	e = arrayElement_(cmd, sz - 1);
	switch (value_type(elementVal(e))) {
		case V_UNDEF:
		case V_EOL:
		case V_EOF:
		case V_COMMENT:
			free_remove_element(e);
			return;
		case V_SPECIAL:
			if (sp_value(elementVal(e)) == ';')
			{
				free_remove_element(e);
				return;
			}
		default:
			return;
	}
}
