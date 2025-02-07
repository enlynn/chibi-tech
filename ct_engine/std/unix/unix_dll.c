#include "../dll.h"

#include <assert.h>
#include <dlfcn.h>

fn_export os_dll_t
dll_load(const char *libpath)
{
    os_dll_t result = {};
    result.handle = dlopen(libpath, RTLD_NOW | RTLD_LOCAL);
    assert(result.handle != NULL);

    return result;
}

fn_export bool
dll_is_loaded(os_dll_t *dll)
{
    return dll->handle != NULL;
}

fn_export void
dll_unload(os_dll_t *dll)
{
    if (dll->handle)
    {
        dlclose(dll->handle);
        dll->handle = NULL;
    }
}

fn_export void*
dll_get_fn(os_dll_t *dll, const char* fn_name)
{
    if (dll->handle)
    {
        return dlsym(dll->handle, fn_name);
    }

    return NULL;
}
