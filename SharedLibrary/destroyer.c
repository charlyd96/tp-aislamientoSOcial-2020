#include <stdlib.h>
#include "destroyer.h"

void free_split (char **string)
{
    for (int i=0; *(string + i) != NULL ; i++)
    free (*(string+i));
    free (string);
}