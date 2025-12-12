#ifndef _PATH_H_
#define _PATH_H_

// void trim(char *s);
// void remove_multiple_slashes(char *s);
// void rstrip_slash(char *s);
// char *next_token(char **p);

void path_normalize(char *out, size_t outsz, const char *in);
char *next_token(char **p);


#endif /* _PATH_H_ */