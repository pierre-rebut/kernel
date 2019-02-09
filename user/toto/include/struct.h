/*
** struct.h for 42sh in /home/rebut_p/Programmation/Projet-b2/sys_unix/42sh
**
** Made by Pierre REBUT
** Login   <rebut_p@epitech.net>
**
** Started on  Wed May  6 16:46:59 2015 Pierre REBUT
** Last update Tue May 19 17:45:26 2015 Pierre REBUT
*/

#ifndef		STRUCT_H_
# define	STRUCT_H_

typedef	struct	s_env
{
  int		len;
  char		**env;
}		t_env;

typedef	struct	s_cmd
{
  char		*prg;
  char		**args;
  char		*redir_in;
  char		*redir_out;
  int		pipefd[2];
  struct s_cmd	*next;
  struct s_cmd	*prev;
}		t_cmd;

#endif		/* STRUCT_H_ */
