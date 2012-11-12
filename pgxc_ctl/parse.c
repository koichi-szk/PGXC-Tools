/*
 * Nov.9, Fri, Did most of value, variable and input.   Need to handle EOF more elegantly.
 */

#include "parse.h"

static char *appendSimpleString(char *to, char *new);

/* Memory allocation */
void *Malloc(size_t s)
{
	void *rv;
	if ((rv = malloc(s)) == NULL)
	{
		fprintf(stderr, "No more memory.\n");
		exit(1);
	}
	return(rv);
}

void *Realloc(void *ptr, size_t sz)
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

string_t *alloc_string()
{
	string_t *rv = Malloc(sizeof(string_t));
	memset(rv, 0, sizeof(string_t));
}

string_t *assign_string(string_t *str, char *s)
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

string_t *cat_string(string_t *str, char *s)
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

void free_string(string_t *str)
{
	free(str->s);
	free(str);
}

/* ============= Value ================*/

int word_to_int(value_t *v)
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

value_t *alloc_val(void)
{
	value_t *v = Malloc(sizeof(value_t));
	memset(v, 0, sizeof(value_t));
	return v;
}

void free_val(value_t *v)
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

array_t *alloc_array(void)
{
	array_t *rv = Malloc(sizeof(array_t));
	memset(rv, 0, sizeof(array_t));
	return rv;
}

/* Append an array to another.  Array *from is freed. Only elements of *from survve */
array_t *appendArray(array_t *to, array_t *from)
{
	arrayElement_t *e;

	for (e = from->head; e; e = e->next)
		e->array = to;
	to->tail->next = from->head;
	from->head->prev = to->tail;
	to->tail = from->tail;
	free(from);
	makeArrayIndex(to);
	return to;
}

/* Cut array element before the given element and make separate array */
array_t *separateArray(arrayElement_t *e)
{
	array_t *rv = alloc_array();
	array_t *org = e->array;
	
	rv->head = e;
	rv->tail = org->tail;
	org->tail = e->prev;
	if (org->tail == NULL)
		org->head = NULL;
	for (; e; e = e->next)
		e->array = rv;
	makeArrayIndex(org);
	makeArrayIndex(rv);
	return(rv);
}



int arraySize(array_t *array)
{
	if (isArrayEmpty(array))
		return 0;
	if (array->idx == NULL)
		makeArrayIndex(array);
	return(array->len);
}

arrayElement_t *arrayElement(array_t *array, int idx)
{
	if (isArrayEmpty(array))
		return NULL;
	if (array->idx == NULL)
		makeArrayIndex(array);
	if (idx < 0 || idx > array->len)
		return NULL;
	return((array->idx)[idx]);
}

void makeArrayIndex(array_t *array)
{
	int array_sz;
	arrayElement_t *e;
	int ii;

	for (array_sz = 0, e = array->head; e; e = e->next, array_sz++);
	array->len = array_sz;
	if (array->size <= array_sz)
	{
		if (array_sz <= 8)
		{
			array->size = 8;
			array->idx = Malloc(sizeof(arrayElement_t *) * 8);
		}
		else if (array_sz <= 16)
		{
			array->size = 16;
			array->idx = Malloc(sizeof(arrayElement_t *) * 16);
		}
		else if (array_sz <= 32)
		{
			array->size = 32;
			array->idx = Malloc(sizeof(arrayElement_t *) * 32);
		}
		else
		{
			array->size = array_sz;
			array->idx = Malloc(sizeof(arrayElement_t *) * array_sz);
		}
	}
	for (ii = 0, e = array->head; e; e = e->next, ii++)
	{
		(array->idx)[ii] = e;
	}
}

void convertElementToWord(arrayElement_t *e)
{
	char *buf;

	switch value_type(elementVal(e)) {
		case V_UNDEF:
		case V_COMMENT:
		case V_EOL:
		case V_EOF:
			buf = Malloc(2);
			buf[0] = '\0';
			value_type(elementVal(e)) = V_WORD;
			wvalue(elementVal(e)) = buf;
			return;
		case V_NUMERIC:
			buf = Malloc(32);
			sprintf(buf, "%d", ivalue(elementVal(e)));
			value_type(elementVal(e)) = V_WORD;
			wvalue(elementVal(e)) = buf;
			return;
		case V_WORD:
			return;
		case V_RAWSTRING:
		case V_STRING:
			buf = svalue(elementVal(e))->s;
			value_type(elementVal(e)) = V_WORD;
			free(svalue(elementVal(e)));
			wvalue(elementVal(e)) = buf;
			return;
		case V_SPECIAL:
			buf = Malloc(2);
			buf[0] = spvalue(elementVal(e));
			buf[1] = '0';
			value_type(elementVal(e)) = V_WORD;
			wvalue(elementVal(e)) = buf;
			return;
		case V_ARRAY:
			buf = arrayToString(avalue(elementVal(e)));
			free_array(avalue(elementVal(e)));
			value_type(elementVal(e)) = V_WORD;
			wvalue(elementVal(e)) = buf;
			return;
		default:
			return;
	}
}

/* Add the value at the end of the array */
arrayElement_t *append_to_array(array_t *a, value_t *v)
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

arrayElement_t *append_element(arrayElement_t *e, value_t *v)
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

arrayElement_t *insert_element(arrayElement_t *e, value_t *v)
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

static void zapElementBackward(arrayElement_t *e);

void array_zapToHead(arrayElement_t *e)
{
	array_t *array = e->array;

	array->head = e->next;
	e->next->prev = NULL;
	if (array->head == NULL)
		array->tail = NULL;
	zapElementBackward(e);
	makeArrayIndex(array);
}

static void zapElementBackward(arrayElement_t *e)
{
	if (e->prev)
		zapElemnetBackward(e->prev);
	free_remove_element(e);
}

static void zapElementForward(arrayElement_t *e);

void array_zapToTail(arrayElement_t *e)
{
	array_t *array = e->array;

	array->tail = e->prev;
	e->prev->next = NULL;
	if (array->tail == NULL)
		array->head = NULL;
	zapElementForward(e);
	makeArrayIndex(array);
}

static void zapElementForward(arrayElement_t *e)
{
	if (e->next)
		zapElemnetBackward(e->next);
	free_remove_element(e);
}


arrayElement_t *remove_element(arrayElement_t *e)
{
	array_t *a = e->array;

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
	makeArrayIndex(a);
	return e;
}

void free_element(arrayElement_t *e)
{
	if (e == NULL)
		return;
	free_val(e->val);
	free(e);
}

void free_remove_elemnt(arrayElement_t *e)
{
	free_element(remove_element(e));
}

void free_all_elements(arrayElement_t *e)
{
	if (e->next == NULL)
		free_element(e);
	else
		return free_all_elements(e->next);
	return;
}

void free_array(array_t *a)
{
	free_all_elements(a->head);
	free(a);
}

char *valToString(value_t *val)
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


char *arrayToString(array_t *array)
{
	char *rv = NULL;
	arrayElement_t *cur;

	for (cur = array->head; cur; cur = cur->next)
	{
		char *new = valToString(cur->val);
		rv = appendSimpleString(rv, new);
		free(new);
	}
}

static char *appendSimpleString(char *to, char *new)
{
	int i_to;
	int i_new = strlen(new);
	char *rv;

	if (to == NULL)
		return strdup(new);
	i_to = strlen(to);
	rv = Realloc(to, i_to + i_new + 1);
	strcat(rv, new);
	return rv;
}

/* ============== VARIABLE ======================*/
	
variable_t *var_head = NULL;
variable_t *var_tail = NULL;

variable_t *findVariable(char *name)
{
	variable_t *var;

	for(var = var_head; var; var=var->next)
	{
		if (strcmp(var->var_name, name) == 0)
			return var;
	}
	return NULL;
}

void assignVariable(variable_t *variable, value_t *value)
{
	free_val(variable->value);
	variable->value = value;
}

void assign_variableByName(char *name, value_t *value)
{
	variable_t *var;

	if ((var = findVariable("name")) == NULL)
		add_variable(name, value);
	else
		assign_variable(var, value);
}

void add_variable(char *name, value_t *val)
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

char *getVarStringValue(value_t *val)
{
	variable_t *variable = findVariable(vvalue(val));

	if (variable == NULL)
		return NULL;
	else
		return valToString(variable->value);
}

char *getVarStringValueByName(char *name)
{
	variable_t *variable = findVariable(name);

	if (variable == NULL)
		return NULL;
	else
		return valToString(variable->value);
}

/* ================ INPUT ====================*/

inputData_t *pushInputFile(inputData_t *inData, FILE *newf)
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
		ind->line = Malloc(MAXLINE);
		ind->len = MAXLINE;
		ind->cur = NULL;
	}
	else
	{
		ind = inData;
		ind->cur = NULL;
	}
	f = Malloc(sizeof(inFile_t));
	f->next = ind->inf;
	f->infile = newf;
	f->tty = isatty(fd);
	ind->inf = f;
	return(ind);
}

inputData_t *pushInputFileName(inputData_t *inData, char *fname)
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

FILE *popInputFile(inputData_t *inData)
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

char *Gets(inputData_t *inData)
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

value_t *getValue(inputData_t *inData)
{
	for(; cChar(inData) == ' ' || cChar(inData) == '\t'; inData->cur++);
	if (cChar(inData) == '\0' || cChar(inData) == '\n')
	{
		value_t *rv = alloc_val();
		value_type(rv) = V_EOL;
		return(rv);
	}
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
	{
		cPos(inData)++;
		return(getArray(inData));
	}
	else
		return(getSpecial(inData));
}

void skipTo(inputData_t *inData, char to, int detect_comment)
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

value_t *getVariable(inputData_t *inData)
{
	value_t *rv;
	int		paren_f = false;
	if (cChar(inData) == '(')
	{
		paren_f = true;
		cPos(inData)++;
	}
	rv = getWord(inData);
	if (paren_f)
		skipTo(inData, ')', true);
	word_to_variable(rv);
	return rv;
}

value_t *getWord(inputData_t *inData)
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

value_t *makeWordValue(char *word)
{
	value_t *rv;

	rv = alloc_val();
	value_type(rv) = V_WORD;
	wvalue(rv) = strdup(word);
	return(rv);
}

value_t *getComment(inputData_t *inData)
{
	value_t *rv = alloc_val();

	rv->type = V_COMMENT;
	return rv;
}

value_t *getRawString(inputData_t *inData)
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
		*(tail+1) = '\0';
	rsvalue(rv) = assign_string(str, start);
	Gets(inData);
	return appendRawString(rv, inData);
}

value_t *appendRawString(value_t *rv, inputData_t *inData)
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

value_t *getString(inputData_t *inData)
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
	if (*tail == '$')
	{
		/* Need to add variable handling.  The following handles only \n and \0 */
	}
	bkup = *(tail+1);
	*(tail+1) = '\0';
	str = assign_string(str, start);
	rsvalue(rv) = str;
	Gets(inData);
	return appendString(rv, inData);
}

value_t *appendString(value_t *rv, inputData_t *inData)
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

value_t *getSpecial(inputData_t *inData)
{
	value_t *rv = alloc_val();

	value_type(rv) = V_SPECIAL;
	spvalue(rv) = cChar(inData);
	cPos(inData)++;
	return(rv);
}

value_t *getArray(inputData_t *inData)
{
	value_t *rv = alloc_val();
	value_t *element;
	array_t *array;

	value_type(rv) = V_ARRAY;
	avalue(rv) = array = alloc_array();
	while (element = getValue(inData), value_type(element) != V_EOF && (value_type(element) != V_SPECIAL || spvalue(element) != ')'))
	{
		if (value_type(element) == V_COMMENT)
			continue;
		append_to_array(array, element);
	}
	return rv;
}
