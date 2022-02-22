/* hexembed.c - copyright Lewis Van Winkle */
/* zlib license */
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int replacechar(char *str, char orig, char rep) {
  char *ix = str;
  int n = 0;
  while ((ix = strchr(ix, orig)) != NULL) {
    *ix++ = rep;
    n++;
  }
  return n;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage:\n\thexembed <filename>\n");
    return 1;
  }

  const char *fname = argv[1];
  FILE *fp = fopen(fname, "rb");
  if (!fp) {
    fprintf(stderr, "Error opening file: %s.\n", fname);
    return 1;
  }

  char *name = basename(fname);
  replacechar(name, '.', '_');

  fseek(fp, 0, SEEK_END);
  const unsigned long long fsize = ftell(fp);

  fseek(fp, 0, SEEK_SET);
  unsigned char *b = malloc(fsize);

  fread(b, fsize, 1, fp);
  fclose(fp);

  printf("/* Embedded file: %s */\n", fname);
  printf("const unsigned long long fsize_%s = %lluL;\n", name, fsize);
  printf("const unsigned char file_%s[] = {\n", name);

  unsigned long long i;
  for (i = 0; i < fsize; ++i) {
    printf("0x%02x%s", b[i],
           i == fsize - 1 ? "" : ((i + 1) % 16 == 0 ? ",\n" : ","));
  }
  printf("\n};\n");

  free(b);
  return 0;
}