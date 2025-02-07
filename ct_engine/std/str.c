#include "str.h"

//#define STRING_NO_UNPAIRED_SURROGATES
#define STRING_REPLACEMENT_CHAR 0xfffd



fn_export int
str_len(const char* str)
{
	int result = 0;
	for (const char* iter = str; *iter; iter++) result += 1;
	return result;
}

/*fn_export int str16_len(const wchar_t* str)
{
	int result = 0;
	for (const wchar_t* iter = str; *iter; iter++) result += 1;
	return result;

	int index = 0;
    u64 size = 0;
    while (str[index])
    {
        int consumed;
        char32_t code_point = to_code_point(&str[index], &consumed);
        index += consumed;
        size += size_in_utf8(code_point);
    }
    return Size;
}*/

fn_export bool
str_cmp(const char* str_a, const char* str_b)
{
    const int str_a_len = str_len(str_a);
    const int str_b_len = str_len(str_b);

    if (str_a_len != str_b_len) return false;

    for (int i = 0; i < str_a_len; ++i)
    {
        if (str_a[i] != str_b[i]) return false;
    }

    return true;
}
