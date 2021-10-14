#include "config_parser.h"
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

bool string_starts_with(const char * s, const char * prefix)
{
    assert(s);
    assert(prefix);

    unsigned int i = 0;
    while(prefix[i]) {
        if(s[i] != prefix[i]) return false;
        i++;
    }
    return true;
}

bool word_matches(const char * s, const char * find)
{
    assert(s);
    assert(find);

    while(true) {
        if(!*find) return !*s || *s == ' ' || *s == '\t';
        if(!*s || *s == ' ' || *s == '\t') return false;
        if(*s != *find) return false;
        s++;
        find++;
    }
    return true;
}

bool string_matches(const char * s, const char * s2)
{
    assert(s);
    assert(s2);

    while(true) {
        if(*s != *s2) return false;
        if(!*s /* || !*s2 */) return true;
        s++;
        s2++;
    }
    return true;
}


size_t strlen_to_whitespace(const char * s)
{
    assert(s);

    size_t l = 0;
    while(*s && *s != ' ' && *s != '\t') {
        s++;
        l++;
    }
    return l;
}

char * copy_string_to_whitespace_to_malloc(const char * s)
{
    assert(s);

    const size_t l = strlen_to_whitespace(s);
    char * name = malloc(l+1);
    if(!name) return NULL;
    memcpy(name, s, l);
    name[l] = 0;
    return name;
}

static bool char_is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void skip_to_next_line(char ** input)
{
    char * line = *input;

    while(true) {
        while(*line && *line != '\n') line++;
        while(*line && char_is_whitespace(*line)) line++;
        if(!*line) {
            line = NULL;
            break;
        }
        if(*line == '#') {
            continue;
        }
        break;
    }

    *input = line;
}

int parser_match_key_get_value(char ** input, const char * const* keys, unsigned int key_array_size, const char ** value)
{
    if(key_array_size > INT32_MAX) {
        assert(false);
        return -1;
    }

    *value = NULL;

    if(!input) return -1;

    char * in = *input;
    *input = NULL;
    if(!in || !*in) return -1;


    // Skip whitespace at start of line

    while(*in && (*in == ' ' || *in == '\t')) in++;
    if(!*in) return -1;


    // Skip to next line if this one is blank or a comment

    if(*in == '\n' || *in == '\r' || *in == '#') {
        skip_to_next_line(&in);
    }
    if(!in || !*in) return -1;


    // 'in' now points to the start of the key name


    // Find a matching key name

    unsigned int key_index = 0;
    for(; key_index < key_array_size; key_index++) {
        if(string_starts_with(in, keys[key_index])) {
            const size_t l = strlen(keys[key_index]);
            in += l;

            if(!*in) {
                // Match but EOF
                *input = NULL;
                return (int)key_index;
            }
            else if (*in == '\n' || *in == '\r') {
                // Match but no value
                *input = in;
                return (int)key_index;
            }
            else if(char_is_whitespace(*in)) {
                // Match
                *in = 0;
                in++;
                break;
            }
            else {
                // Not a match
                in -= l;
            }
        }
    }
    if(key_index == key_array_size) {
        skip_to_next_line(&in);
        *input = in;
        return -1;
    }

    if(!*in) {
        *input = NULL;
        return (int)key_index;
    }

    // 'in' now points to either the key value, whitespace, or newline

    while (*in == ' ' || *in == '\t') in++;

    if(*in == '\r' || *in == '\n') {
        *input = in;
        return (int)key_index;
    }


    // 'in' points to key value

    unsigned int whitespace_counter = 0;
    unsigned int i = 0;
    for(; in[i] && in[i] != '\n' && in[i] != '\r'; i++) {
        if(char_is_whitespace(in[i])) whitespace_counter++;
        else whitespace_counter = 0;
    }
    if(!in[i]) {
        *input = NULL;
    }
    else {
        *input = &in[i+1];
    }
    in[i-whitespace_counter] = 0;
    *value = in;


    return (int)key_index;
}

