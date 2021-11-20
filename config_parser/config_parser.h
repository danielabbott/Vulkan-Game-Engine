#pragma once

#include <stdbool.h>
#include <stddef.h>

bool string_starts_with(const char* s, const char* prefix);

// Returns true if s starts with find, followed by whitespace or NULL
// E.g. word_matches("ABC DEF", "ABC") == true
// find must not contain whitespace, s must not start with whitespace
bool word_matches(const char* s, const char* find);

bool string_matches(const char* s, const char* s2);

size_t strlen_to_whitespace(const char* s);

// Returns NULL on malloc error
char* copy_string_to_whitespace_to_malloc(const char* s);

void parser_skip_to_next_line(char** input);

// Returns index into list or returns -1
int parser_match_key_get_value(char** input, const char* const* keys, unsigned int key_array_size, const char** value);
