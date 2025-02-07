#ifndef _CTENGINE_H_
#define _CTENGINE_H_

#include "std/types.h"

static const u32   c_engine_version = MAKE_APP_VERSION(0, 0, 1);
static const char *c_engine_name    = "Chibi Engine";

fn_export void create_ct_engine();
fn_export void destroy_ct_engine();
fn_export bool ct_begin_frame();

#endif //_CTENGINE_H_
