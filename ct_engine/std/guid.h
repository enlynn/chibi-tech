#ifndef _GUID_H_
#define _GUID_H_

#include "types.h"


fn_export guid_t platform_generate_guid();
fn_export bool platform_compare_guids(const guid_t *left, const guid_t *right);
fn_export bool platform_is_guid_valid(guid_t guid);
fn_export guid_t platform_get_invalid_guid();
fn_export int platfor_guid_to_string(guid_t ct_guid, char *str_buffer, int buffer_len);
fn_export guid_t platform_string_to_guid(const char* guid_str);

#endif //_GUID_H_