#include "common.h"

/***
 * Strip whitespace from end of string
 * @param s string
 * @return count of characters stripped
 */
size_t rstrip(char *s) {
    char *ch;
    size_t i;

    i = 0;
    ch = &s[strlen(s)];
    if (ch) {
        while (isspace(*ch) || iscntrl(*ch)) {
            *ch = '\0';
            --ch;
            i++;
        }
    }
    return i;
}

