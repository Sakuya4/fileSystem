#include <string.h>
#include "path.h"

/* user define internal function */
void trim(char *s);
void remove_multiple_slashes(char *s);
void rstrip_slash(char *s);
/* user define internal function end */


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
  if (!s||*s == '\0')
  {
    return NULL;
  }

  char *start = s;

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
    *p = s;
  }
  return start;
}

void path_normalize(char *out, size_t outsz, const char *in)
{
    strncpy(out, in, outsz - 1);
    out[outsz - 1] = '\0';

    trim(out);
    remove_multiple_slashes(out);
    rstrip_slash(out);
}

