/*
 * Nov.9, Fri, Did most of value, variable and input.   Need to handle EOF more elegantly.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define true 1
#define false 0
/*================ Common string ==========================*/

static void *Malloc(size_t s)
{
	void *rv;
	if ((rv = malloc(s)) == NULL)
	{
		fprintf(stderr, "No more memory.\n");
		exit(1);
	}
	return(rv);
}

static void *Realloc(void *ptr, size_t sz)
{
	void *rv = realloc(ptr, sz);
	if (rv == NULL)
	{
		fprintf(stderr, "No more memory.\n");
		exit(1);
	}
	return(rv);
}

/*========== String ========================*/
typedef struct strig_t {
	int size;
	int	len;
	char *s;
} string_t;

static string_t *alloc_string();
static string_t *assign_string(string_t *str, char *s);
static string_t *cat_string(string_t *str, char *s);
static void free_string(string_t *str);

static string_t *alloc_string()
{
	string_t *rv = Malloc(sizeof(string_t));
	memset(rv, 0, sizeof(string_t));
}

static string_t *assign_string(string_t *str, char *s)
{
	string_t *rv;
	int l = s ? strlen(s) : 0;

	if (str == NULL)
		rv = alloc_string();
	else
		rv = str;

	if (rv->s && (rv->size > l))
	{
		strcpy(rv->s, s);
		rv->len = l;
		return(rv);
	}
	if (rv->s) free(rv->s);
	if (l < 128)
	{
		rv->s = Malloc(128);
		rv->size = 128;
	}
	else if (l < 256)
	{
		rv->s = Malloc(256);
		rv->size=256;
	}
	else if (l < 512)
	{
		rv->s = Malloc(512);
		rv->size=512;
	}
	else if (l < 1024)
	{
		rv->s = Malloc(1024);
		rv->size=1024;
	}
	else if (l < 2048)
	{
		rv->s = Malloc(2048);
		rv->size = 2048;
	}
	else if (l < 4096)
	{
		rv->s = Malloc(4096);
		rv->size = 4096;
	}
	else
	{
		fprintf(stderr, "String too long.\n");
		exit(1);
	}
	rv->len = l;
	if (s)
		strcpy(rv->s, s);
	return(rv);
}

static string_t *cat_string(string_t *str, char *s)
{
	string_t *rv;
	int l = s ? strlen(s) : 0;
	int total;

	if (str == NULL)
		rv = alloc_string();
	else
		rv = str;
	if (rv->s == NULL)
	{
		return assign_string(rv, s);
	}

	total = l + str->len;

	if (rv->size > total)
	{
		strcat(rv->s, s);
		rv->len = total;
		return(rv);
	}
	if (total < 128)
	{
		rv->s = Realloc(rv->s, 128);
		rv->size = 128;
	}
	else if (total < 256)
	{
		rv->s = Realloc(rv->s, 256);
		rv->size=256;
	}
	else if (total < 512)
	{
		rv->s = Realloc(rv->s, 512);
		rv->size=512;
	}
	else if (total < 1024)
	{
		rv->s = Realloc(rv->s, 1024);
		rv->size=1024;
	}
	else if (total < 2048)
	{
		rv->s = Realloc(rv->s, 2048);
		rv->size = 2048;
	}
	else if (total < 4096)
	{
		rv->s = Realloc(rv->s, 4096);
		rv->size = 4096;
	}
	else
	{
		fprintf(stderr, "String too long.\n");
		exit(1);
	}
	rv->len = total;
	if (s)
		strcat(str->s, s);
	return(rv);
}

static void free_string(string_t *str)
{
	free(str->s);
	free(str);
}

/* ============= Value ================*/

typedef enum vtype_t {
	V_UNDEF = 0,
	V_NUMERIC,
	V_WORD,
	V_VARIABLE,
	V_STRING,
	V_RAWSTRING,
	V_ARRAY,
	V_SPECIAL,
	V_COMMENT,
	V_EOF
} vtype_t;

typedef struct value_t {
	vtype_t type;
	union {
		int	i_value;		/* Numeric */
		char *w_value;		/* Word */
		string_t *s_value;	/* String */
		string_t *rs_value;	/* Raw string */
		struct array_t *a_value;	/* Array */
		char sp_value;		/* Special char */
		char *v_value;		/* Variable: only variable name is stored here */
	} val;
} value_t;

typedef struct arrayElement_t {
	struct array_t *array;
	struct arrayElement_t *next;
	struct arrayElement_t *prev;
	struct value_t *val;
} arrayElement_t;

typedef struct array_t {
	arrayElement_t *head;
	arrayElement_t *tail;
} array_t;

static void free_array(array_t *a);
#define value_type(v) ((v)->type)
#define ivalue(v) ((v)->val.i_value)
#define wvalue(v) ((v)->val.w_value)
#define svalue(v) ((v)->val.s_value)
#define rsvalue(v) ((v)->val.rs_value)
#define avalue(v) ((v)->val.a_value)
#define spvalue(v) ((v)->val.sp_value)
#define vvalue(v) ((v)->val.v_value)
#define word_to_variable(v) ((v)->type = V_VARIABLE)
static char *getVarStringValue(value_t *val);
char *arrayToString(array_t *array);

static int word_to_int(value_t *v)
{
	int i;

	if (v->type != V_WORD)
		return -1;
	i = atoi(v->val.w_value);
	free(v->val.w_value);
	v->type = V_NUMERIC;
	v->val.i_value = i;
	return 0;
}

static value_t *alloc_val(void)
{
	value_t *v = Malloc(sizeof(value_t));
	memset(v, 0, sizeof(value_t));
	return v;
}

static void free_val(value_t *v)
{
	if (v == NULL)
		return;
	switch (v->type) {
		case V_UNDEF:
		case V_NUMERIC:
		case V_SPECIAL:
			break;
		case V_VARIABLE:
			free(vvalue(v));
			break;
		case V_STRING:
			free_string(svalue(v));
			break;
		case V_RAWSTRING:
			free_string(rsvalue(v));
			break;
		case V_ARRAY:
			free_array(avalue(v));
			break;
		default:
			fprintf(stderr, "Internal error. %s, %d\n", __FILE__, __LINE__);
			exit(1);
	}
	free(v);
	return;
}




#define isArrayEmpty(a) (((a)->head == NULL) && ((a)->tail == NULL))
static array_t *alloc_array(void)
{
	array_t *rv = Malloc(sizeof(array_t));
	rv->head = rv->tail = NULL;
	return rv;
}
static arrayElement_t *append_to_array(array_t *a, value_t *v)
{
	if (a == NULL)
		a = alloc_array();
	arrayElement_t *e = Malloc(sizeof(arrayElement_t));
	e->array = a;
	e->next = e->prev = NULL;
	e->val = v;
	if (a->head == NULL)
	{
		a->head = a->tail = e;
		return e;
	}
	a->tail->next = e;
	e->prev = a->tail;
	a->tail = e;
	return e;
}
static arrayElement_t *append_element(arrayElement_t *e, value_t *v)
{
	arrayElement_t *new = Malloc(sizeof(arrayElement_t));
	new->val = v;
	if (e->next == NULL)
	{
		new->next = NULL;
		e->next = new;
		new->prev = e;
		e->array->tail = new;
	}
	else
	{
		new->next = e->next;
		new->prev = e;
		e->next->prev = new;
		e->next = new;
	}
	return new;
}
static arrayElement_t *insert_element(arrayElement_t *e, value_t *v)
{
	arrayElement_t *new = Malloc(sizeof(arrayElement_t));
	new->val = v;
	if (e->prev == NULL)
	{
		new->prev = NULL;
		e->prev = new;
		new->next = e;
		e->array->head = new;
	}
	else
	{
		new->prev = e->prev;
		new->next = e;
		e->prev->next = new;
		e->prev = new;
	}
	return new;
}
static arrayElement_t *remove_element(arrayElement_t *e)
{
	if (e == NULL)
		return NULL;
	if (e->prev == NULL)
	{
		e->array->head = e->next;
		e->next->prev = NULL;
	}
	else if (e->next == NULL)
	{
		e->array->tail = e->prev;
		e->prev->next = NULL;
	}
	else
	{
		e->prev->next = e->next;
		e->next->prev = e->prev;
	}
	return e;
}
static void free_element(arrayElement_t *e)
{
	if (e == NULL)
		return;
	free_val(e->val);
	free(e);
}
static void free_remove_elemnt(arrayElement_t *e)
{
	free_element(remove_element(e));
}
static void free_all_elements(arrayElement_t *e)
{
	if (e->next == NULL)
		free_element(e);
	else
		return free_all_elements(e->next);
	return;
}
static void free_array(array_t *a)
{
	free_all_elements(a->head);
	free(a);
}

static char *valToString(value_t *val)
{
	char *rv;

	switch(value_type(val)) {
		case V_NUMERIC:
			rv = Malloc(64);
			sprintf(rv, "%d", ivalue(val));
			return rv;
		case V_WORD:
			return strdup(wvalue(val));
		case V_VARIABLE:
			return strdup(vvalue(val));
		case V_STRING:
			return strdup(svalue(val)->s);
		case V_RAWSTRING:
			return strdup(rsvalue(val)->s);
		case V_ARRAY:
			return arrayToString(avalue(val));
		case V_SPECIAL:
			rv = Malloc(2);
			rv[0] = spvalue(val);
			rv[1] = 0;
			return rv;
		case V_UNDEF:
		case V_COMMENT:
		default:
			return strdup("");
	}
}	

/* ============== VARIABLE ======================*/
	
typedef struct variable_t {
	struct variable_t *next;
	char	*var_name;
	value_t *value;
} variable_t;

static variable_t *var_head;
static variable_t *var_tail;

static variable_t *findVariable(char *name)
{
	variable_t *var;

	for(var = var_head; var; var=var->next)
	{
		if (strcmp(var->var_name, name) == 0)
			return var;
	}
	return NULL;
}

static void add_variable(char *name, value_t *val)
{
	variable_t *new = Malloc(sizeof(variable_t));

	new->next = NULL;
	new->var_name = strdup(name);
	new->value = val;

	if (var_head == NULL)
		var_head = var_tail = new;
	else
	{
		var_tail->next = new;
		var_tail = new;
	}
	return;
}

static char *getVarStringValue(value_t *val)
{
	variable_t *variable = findVariable(vvalue(val));

	return valToString(variable->value);
}


/* ================ INPUT ====================*/

typedef struct inFile_t {
	struct inFile_t *next;
	FILE 	*infile;
	int		tty;	
} inFile_t;

#define MAXLINE 2048

typedef struct inputData_t {
	char 	*line;
	char	*cur;
	int		len;
	int		eof_flag;
	inFile_t *inf;
} inputData_t;

#define Isalpha(c) (((c) & 0x80) || ((c) >= 'A' && (c)<= 'Z') || ((c) >= 'a' && (c) <= 'z') || ((c) == '_'))
#define Isnum(c) ((c) >= '0' && (c) <= '9')
#define cChar(d) (*((d)->cur))
#define cPos(d) ((d)->cur)
#define IsEOF(d) ((d)->eof_flag)


static inputData_t *pushInputFile(inputData_t *inData, FILE *newf)
{
	inputData_t *ind;
	int fd;
	inFile_t *f;

	if ((fd = fileno(newf)) == -1)
		return NULL;
	if (inData == NULL)
	{
		ind = Malloc(sizeof(inputData_t));
		memset(ind, 0, sizeof(inputData_t));
	}
	else
	{
		ind = inData;
		ind->cur = NULL;
		ind->line = Malloc(MAXLINE);
		ind->len = MAXLINE;
	}
	f = Malloc(sizeof(inFile_t));
	f->next = ind->inf;
	f->infile = newf;
	f->tty = isatty(fd);
	ind->inf = f;
}

static inputData_t *pushInputFileName(inputData_t *inData, char *fname)
{
	FILE *f;
	f = fopen(fname, "r");
	if (f == NULL)
	{
		fprintf(stderr, "Cannot open file \"%s\", %s\n", fname, strerror(errno));
		return NULL;
	}
	return pushInputFile(inData, f);
}

static FILE *popInputFile(inputData_t *inData)
{
	inFile_t *oldf;

	if (inData == NULL)
		return NULL;
	if (inData->inf == NULL)
		return NULL;
	oldf = inData->inf;
	fclose(oldf->infile);
	inData->inf = oldf->next;
	free(oldf);
	inData->cur = NULL;
	return(inData->inf->infile);
}

static char *Gets(inputData_t *inData)
{
	char *rv;
	FILE *f;

	rv = fgets(inData->line, inData->len, inData->inf->infile);
	if (rv == NULL)
	{
		f = popInputFile(inData);
		if (f = NULL)
		{
			inData->cur = NULL;
			inData->eof_flag = true;
			return NULL;
		}
		return Gets(inData);
	}
	inData->cur = inData->line;
	return rv;
}

static value_t *getVariable(inputData_t *inData);
static value_t *getComment(inputData_t *inData);
static value_t *getRawString(inputData_t *inData);
static value_t *getString(inputData_t *inData);
static value_t *getWord(inputData_t *inData);
static value_t *getArray(inputData_t *inData);
static value_t *getSpecial(inputData_t *inData);

static value_t *getValue(inputData_t *inData)
{
	for(; cChar(inData) == ' ' || cChar(inData) == '\t'; inData->cur++);
	if (cChar(inData) == '\0' || cChar(inData) == '\n')
		return NULL;
	/* Test the first character */
	if (cChar(inData) == '$')
	{
		cPos(inData)++;
		return(getVariable(inData));
	}
	if (cChar(inData) == '#')
		return(getComment(inData));
	if (cChar(inData) == '\'')
	{
		cPos(inData)++;
		return(getRawString(inData));
	}
	if (cChar(inData) == '"')
	{
		cPos(inData)++;
		return(getString(inData));
	}
	if (Isalpha(cChar(inData)) || Isnum(cChar(inData)))
		return(getWord(inData));
	if (cChar(inData) == '(')
		return(getArray(inData));
	else
		return(getSpecial(inData));
}

static void skipTo(inputData_t *inData, char to, int detect_comment)
{
	char *cur;
	if (detect_comment)
	{
		for (cur = cPos(inData); *cur && *cur != to && *cur != '#'; cur++);
		if ((*cur == '\0') || (*cur == '#'))
		{
			Gets(inData);
			if (!IsEOF(inData))
				skipTo(inData, to, detect_comment);
			return;
		}
		else
		{
			cPos(inData) = cur;
			return;
		}
	}
	else
	{
		for (cur = cPos(inData); *cur && *cur != to; cur++);
		if (*cur == '\0')
		{
			Gets(inData);
			return skipTo(inData, to, detect_comment);
		}
		else
		{
			cPos(inData) = cur;
			return;
		}
	}
}

static value_t *getVariable(inputData_t *inData)
{
	value_t *rv;
	if (cChar(inData) == '(')
		cPos(inData)++;
	rv = getWord(inData);
	if (cChar(inData) != ')')
		skipTo(inData, ')', true);
	word_to_variable(rv);
	return rv;
}

static value_t *getWord(inputData_t *inData)
{
	value_t *rv;
	char bkup;
	char *start = inData->cur;
	char *tail;

	rv = alloc_val();
	value_type(rv) = V_WORD;
	for(tail = start; Isalpha(*tail) || Isnum(*tail); tail++);
	bkup = *tail;
	*tail = '\0';
	wvalue(rv) = strdup(start);
	*tail = bkup;
	inData->cur = tail;
	return(rv);
}

static value_t *getComment(inputData_t *inData)
{
	value_t *rv = alloc_val();

	rv->type = V_COMMENT;
	Gets(inData);
	return rv;
}

static value_t *appendRawString(value_t *rv, inputData_t *inData);

static value_t *getRawString(inputData_t *inData)
{
	value_t *rv = alloc_val();
	string_t *str = alloc_string();
	char bkup;

	char *start = cPos(inData);
	char *tail;

	value_type(rv) = V_RAWSTRING;
	for(tail = start; *tail != '\'' && *tail != '\n' && *tail != '\0'; tail++);
	if (*tail == '\'')
	{
		bkup = *tail;
		*tail = '\0';
		str = assign_string(str, start);
		*tail = bkup;
		rsvalue(rv) = str;
		cPos(inData) = tail + 1;
		return(rv);
	}
	if (*tail == '\n')
		*tail = '\0';
	str = assign_string(str, start);
	Gets(inData);
	return appendRawString(rv, inData);
}

static value_t *appendRawString(value_t *rv, inputData_t *inData)
{
	string_t *str = rsvalue(rv);
	char *start = cPos(inData);
	char *tail;
	char bkup;

	for (tail = start; *tail != '\'' && *tail != '\n'  && *tail != '\0'; tail++);
	bkup = *tail;
	*tail = '\0';
	str = cat_string(str, start);
	*tail = bkup;
	if (*tail == '\'')
	{
		cPos(inData) = tail + 1;
		return(rv);
	}
	if (*tail == '\n')
		*tail = '\0';
	str = cat_string(str, start);
	Gets(inData);
	return appendRawString(rv, inData);
}

static value_t *appendString(value_t *rv, inputData_t *inData);

static value_t *getString(inputData_t *inData)
{
	value_t *rv = alloc_val();
	string_t *str = alloc_string();
	value_t *varInStr;

	char *start = cPos(inData);
	char *tail;
	char bkup;

	value_type(rv) = V_STRING;
	for (tail = start; *tail != '"' && *tail != '\n' && *tail != '\0' && *tail != '$'; tail++);
	if (*tail == '"')
	{
		bkup = *tail;
		*tail = '\0';
		str = assign_string(str, start);
		*tail = bkup;
		rsvalue(rv) = str;
		cPos(inData) = tail + 1;
		return(rv);
	}
	bkup = *tail;
	*tail = '\0';
	str = assign_string(str, start);
	*tail = bkup;
	cPos(inData) = tail;
	if (*tail == '\n' || *tail == '\0')
		Gets(inData);
	return appendString(rv, inData);
}

static value_t *appendString(value_t *rv, inputData_t *inData)
{
	string_t *str = svalue(rv);
	char *start = cPos(inData);
	char *tail;
	char bkup;
	value_t *varVar;
	char *varStringVar;

	for (tail = start; *tail != '"' && *tail != '\n' && *tail != '\0' && *tail != '$'; tail++);
	bkup = *tail;
	*tail = '\0';
	str = cat_string(str, start);
	*tail = bkup;
	if (*tail == '$')
	{
		cPos(inData)++;
		varVar = getVariable(inData);
		varStringVar = getVarStringValue(varVar);
		str = cat_string(str, varStringVar);
		free(varStringVar);
		return appendString(rv, inData);
	}
	if (*tail == '"')
	{
		cPos(inData) = tail + 1;
		return rv;
	}
	Gets(inData);
	return appendString(rv, inData);
}

static value_t *getSpecial(inputData_t *inData)
{
	value_t *rv = alloc_val();

	value_type(rv) = V_SPECIAL;
	spvalue(rv) = cChar(inData);
	cPos(inData)++;
	return(rv);
}

static value_t *getArray(inputData_t *inData)
{
	value_t *rv = alloc_val();
	value_t *element;
	array_t *array;

	value_type(rv) = V_ARRAY;
	avalue(rv) = array = alloc_array();
	while (element = getValue(inData), value_type(element) != V_EOF && (value_type(element) != V_SPECIAL || spvalue(element) != ')'))
		append_to_array(array, element);
}

/*------------------- MAIN ------------------------------*/

inputData_t *inputData = NULL;

main(int argc, char *argv[])
{
	inputData = pushInputFile(inputData, stdin);
	do_command();
}

