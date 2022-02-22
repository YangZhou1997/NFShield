#ifndef __PARSING_MAC_H__
#define __PARSING_MAC_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parse_mac(uint8_t mac[], char *str) {
  char *token = strtok(str, ":");
  int cnt = 0;
  while (token != NULL) {
    //   printf( " %s\n", token );
    mac[cnt++] = (uint8_t)strtol(token, NULL, 16);
    token = strtok(NULL, ":");
  }
}

#endif