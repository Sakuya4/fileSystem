#include <string.h>
#include "path.h"

void trim(char *s)
{
  char *p = s;
  int len;

  if (s == NULL)
  {
    return;
  }

  while (*p == ' ' || *p == '\t')
  {
    p++;
  }
  if (p != s)
  {
    memmove(s, p, strlen(p) + 1);
  }

  len = (int)strlen(s);
  while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t'))
  {
    s[len - 1] = '\0';
    len--;
  }
}

void remove_multiple_slashes(char *s)
{
  char *dst = s;
  char *src = s;
  int slash = 0;

  if (s == NULL)
  {
    return;
  }

  while (*src)
  {
    if (*src == '/')
    {
      if (!slash)
      {
        *dst++ = '/';
      }
      slash = 1;
    }
    else
    {
      slash = 0;
      *dst++ = *src;
    }
    src++;
  }
  *dst = '\0';
}

void rstrip_slash(char *s)
{
  int len;

  if (s == NULL)
  {
    return;
  }

  len = (int)strlen(s);
  while (len > 1 && s[len - 1] == '/')
  {
    s[len - 1] = '\0';
    len--;
  }
}

char *next_token(char **p)
{
  char *s = *p;
  char *start;

  if (s == NULL || *s == '\0')
  {
    return NULL;
  }

  start = s;

  while (*s && *s != '/')
  {
    s++;
  }

  if (*s == '/')
  {
    *s = '\0';
    *p = s + 1;
  }
  else
  {
    *p = s; /* 指到 '\0' */
  }

  return start;
}


