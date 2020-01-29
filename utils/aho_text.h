#ifndef __AHO_TEXT_H__
#define __AHO_TEXT_H__

struct aho_text_t
{
    int id;
    char* text;
    int len;
    struct aho_text_t *prev, *next;
};

#endif//__AHO_TEXT_H__