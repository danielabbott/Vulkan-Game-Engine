#include "config_parser.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static inline int pigeon_test_config_parser(void)
{
#define TEST_ASSERT(x) if(!(x)){assert(false); return 1;}

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
    TEST_ASSERT(input);
    memcpy(input, input_string, strlen(input_string));


    const char * value;
    int key = parser_match_key_get_value(&input, keys, sizeof keys / sizeof(*keys), &value);
    TEST_ASSERT(key == 3);
    TEST_ASSERT(value);
    TEST_ASSERT(string_matches(value, "123 456"));


    key = parser_match_key_get_value(&input, keys, sizeof keys / sizeof(*keys), &value);
    TEST_ASSERT(key == 4);
    TEST_ASSERT(value);
    TEST_ASSERT(string_matches(value, "ABC"));

    key = parser_match_key_get_value(&input, keys, sizeof keys / sizeof(*keys), &value);
    TEST_ASSERT(key == 3);
    TEST_ASSERT(!value);


    key = parser_match_key_get_value(&input, keys, sizeof keys / sizeof(*keys), &value);
    TEST_ASSERT(key < 0);
    TEST_ASSERT(!value);
    
    TEST_ASSERT(string_starts_with("ABC123", "AB"));
    TEST_ASSERT(word_matches("ABC DEF", "ABC"));
    TEST_ASSERT(!word_matches("ABC DEF", "DEF"));
    TEST_ASSERT(!word_matches("AB C DEF", "ABC"));
    TEST_ASSERT(word_matches("ABC DEF", "ABC"));
    TEST_ASSERT(!word_matches("ABC DEF", "AB"));

    return 0;
}
