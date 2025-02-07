#ifndef _DLL_H_
#define _DLL_H_

#include "types.h"

typedef struct {
    void *handle;
} os_dll_t;

fn_export os_dll_t dll_load(const char *libpath);
fn_export bool dll_is_loaded(os_dll_t *dll);
fn_export void dll_unload(os_dll_t *dll);
fn_export void* dll_get_fn(os_dll_t *dll, const char* fn_name);

#endif //_DLL_H_
