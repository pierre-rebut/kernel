/*
** tools.c for 42sh in /home/rebut_p/Programmation/Projet-b2/sys_unix/42sh
**
** Made by Pierre REBUT
** Login   <rebut_p@epitech.net>
**
** Started on  Sun May 24 17:43:04 2015 Pierre REBUT
** Last update Sun May 24 17:44:10 2015 Pierre REBUT
*/

#include    "define.h"
#include    "functions.h"

char *shift(char *str, int i)
{
    while (str[i] != 0) {
        str[i] = str[i + 1];
        i++;
    }
    return (str);
}

char *epur_str(char *str)
{
    int i;

    i = 0;
    while (str[i] != 0) {
        if (str[i] == '\t') {
            str[i] = ' ';
            epur_str(str);
        } else if (str[i] == ' ' &&
                   (str[i + 1] == ' ' || i == 0 || str[i + 1] == '\0'))
            epur_str(shift(str, i));
        i++;
    }
    return (str);
}

int compare_chara(char c, const char *str)
{
    int i;

    i = 0;
    while (str[i]) {
        if (c == str[i])
            return (SUCCESS);
        i++;
    }
    return (FAIL);
}
