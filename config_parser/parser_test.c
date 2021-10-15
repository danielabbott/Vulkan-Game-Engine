#include "config_parser.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    const char * keys[] = {
        "ABC",
        "ab",
        "ABD",
        "abc",
        "AbC",
        "abcd",
    };

    const char * input_string = "\n\n\t\t  \n\n#hello\n\nabc 123 456\n\n AbC ABC\nabc\nwrong";

    char * input = malloc(strlen(input_string));
    assert(input);
    memcpy(input, input_string, strlen(input_string));


    const char * value;
    int key = parser_match_key_get_value(&input, keys, sizeof keys / sizeof(*keys), &value);
    assert(key == 3);
    assert(value);
    assert(string_matches(value, "123 456"));


    key = parser_match_key_get_value(&input, keys, sizeof keys / sizeof(*keys), &value);
    assert(key == 4);
    assert(value);
    assert(string_matches(value, "ABC"));

    key = parser_match_key_get_value(&input, keys, sizeof keys / sizeof(*keys), &value);
    assert(key == 3);
    assert(!value);


    key = parser_match_key_get_value(&input, keys, sizeof keys / sizeof(*keys), &value);
    assert(key < 0);
    assert(!value);
    
    assert(string_starts_with("ABC123", "AB"));
    assert(word_matches("ABC DEF", "ABC"));
    assert(!word_matches("ABC DEF", "DEF"));
    assert(!word_matches("AB C DEF", "ABC"));
    assert(word_matches("ABC DEF", "ABC"));
    assert(!word_matches("ABC DEF", "AB"));

    return 0;
}
