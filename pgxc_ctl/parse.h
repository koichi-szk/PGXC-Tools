#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define true 1
#define false 0

void *Malloc(size_t s);
void *Realloc(void *ptr, size_t sz);

/*========== String ========================*/
typedef struct strig_t {
	int size;		/* Allocated size */
	int	len;		/* Used size */
	char *s;
} string_t;

string_t *alloc_string();
string_t *assign_string(string_t *str, char *s);
string_t *cat_string(string_t *str, char *s);
void free_string(string_t *str);

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
	V_EOL,
	V_EOF
} vtype_t;

typedef struct value_t {
	vtype_t type;
	union {
		int				i_value;	/* Numeric */
		char 			*w_value;	/* Word */
		string_t 		*s_value;	/* String */
		string_t 		*rs_value;	/* Raw string */
		struct array_t	*a_value;	/* Array */
		char			sp_value;	/* Special char */
		char			*v_value;	/* Variable: only variable name is stored here */
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
	arrayElement_t **idx;
	int	size;			/* Allocated index size */
	int	len;			/* Used index size */
} array_t;

void free_array(array_t *a);
#define value_type(v) ((v)->type)
#define ivalue(v) ((v)->val.i_value)
#define wvalue(v) ((v)->val.w_value)
#define svalue(v) ((v)->val.s_value)
#define rsvalue(v) ((v)->val.rs_value)
#define avalue(v) ((v)->val.a_value)
#define spvalue(v) ((v)->val.sp_value)
#define vvalue(v) ((v)->val.v_value)
#define word_to_variable(v) ((v)->type = V_VARIABLE)
char *getVarStringValue(value_t *val);
char *arrayToString(array_t *array);
int word_to_int(value_t *v);
value_t *alloc_val(void);
void free_val(value_t *v);
#define isArrayEmpty(a) (((a)->head == NULL) && ((a)->tail == NULL))
#define elementVal(e) ((e)->val)
void convertElementToWord(arrayElement_t *e);
array_t *alloc_array(void);
arrayElement_t *append_to_array(array_t *a, value_t *v);	/* Append at the end of the array */
void makeArrayIndex(array_t *array);
array_t *appendArray(array_t *to, array_t *from);
arrayElement_t *arrayElement(array_t *array, int idx);
#define arrayElement_(a, x) (((a)->idx)[x])
arrayElement_t *append_element(arrayElement_t *e, value_t *v); /* Append next to the element */
arrayElement_t *insert_element(arrayElement_t *e, value_t *v); /* Insert before the element */
arrayElement_t *remove_element(arrayElement_t *e);  /* Only remove the element from the array, the element remains */
static void free_element(arrayElement_t *e);	/* Frees element */
void free_remove_elemnt(arrayElement_t *e);		/* Remove the element and free it */
void free_all_elements(arrayElement_t *e);
void free_array(array_t *a);
char *valToString(value_t *val);	/* Convert the value to string */
void dumpValue(value_t *val);

/* ============== VARIABLE ======================*/
	
typedef struct variable_t {
	struct variable_t *next;
	char	*var_name;
	value_t *value;
} variable_t;

extern variable_t *var_head;
extern variable_t *var_tail;

variable_t *findVariable(char *name);
void add_variable(char *name, value_t *val);
void assignVariable(variable_t *variable, value_t *value);
void asign_variableByName(char *name, value_t *value);
char *getVarStringValue(value_t *val);	/* Convert to string */
char *getVarStringValueByName(char *name);	/* Convert to string */
#define varName(v) ((v)->var_name)
#define varValue(v) ((v)->value)

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

inputData_t *pushInputFile(inputData_t *inData, FILE *newf);	/* Add file to the input stack */
inputData_t *pushInputFileName(inputData_t *inData, char *fname); /* Open file and add it to the input stack */
FILE *popInputFile(inputData_t *inData);	/* Pop input stack.  Return NULL if no more file to read */
char *Gets(inputData_t *inData);	/* Get one line and put it to indata struct */
value_t *getVariable(inputData_t *inData);	/* Read variable name, handles (...) within a string */
value_t *getComment(inputData_t *inData);	/* Read the comment */
value_t *getRawString(inputData_t *inData);	/* Read ' ... ' string */
value_t *getString(inputData_t *inData);	/* Read "..." string */
value_t *getWord(inputData_t *inData);	/* Read simple word */
value_t *getArray(inputData_t *inData);	/* Read whole array */
value_t *getSpecial(inputData_t *inData);	/* Read special token, such as '(', ')', '.', etc. */
value_t *getValue(inputData_t *inData);	/* Read the value */
value_t *makeWordValue(char *word);		/* Make string into word value */
void skipTo(inputData_t *inData, char to, int detect_comment); /* Skip the line to the given character. Read next line if needed. */
value_t *appendRawString(value_t *rv, inputData_t *inData);
value_t *appendString(value_t *rv, inputData_t *inData);

