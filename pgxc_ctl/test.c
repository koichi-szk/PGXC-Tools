#include "parse.h"

inputData_t *indata = NULL;

main(int ac, char *av[])
{
	value_t *val;

	indata = pushInputFile(indata, stdin);
	for (Gets(indata);;Gets(indata))
	{
		for (;;)
		{
			val = getValueo(indata);
			if (value_type(val) == V_EOL || value_type(val) == V_COMMENT)
				break;
			if (value_type(val) == V_EOF)
				exit(0);
			if (value_type(val) == V_SPECIAL)
			{
				if (spvalue(val) == '\n' || spvalue(val) == 0)
					break;
			}
		}
	}
}
