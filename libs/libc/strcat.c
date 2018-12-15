//
// Created by rebut_p on 16/12/18.
//

#include <string.h>
#include <stdlib.h>

char *strcat(char *str1, const char *str2) {
    int i;
    int cpt;
    int len;
    char *my_str;

    cpt = -1;
    len = 1 + strlen(str1) + strlen(str2);
    my_str = malloc(sizeof(char) * len);
    if (my_str == NULL)
        return NULL;

    while (++cpt < strlen(str1))
        my_str[cpt] = str1[cpt];
    i = -1;
    while (str2[++i])
        my_str[cpt++] = str2[i];
    my_str[cpt] = '\0';
    return (my_str);
}