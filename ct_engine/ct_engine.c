#include "ct_engine.h"
#include "std/types.h"
#include "std/mem.h"
#include "std/log.h"
#include "std/guid.h"
#include "std/cassert.h"

#include <window/window.h>
#include <render/renderer.h>

typedef struct {
	struct os_window_o *client_window;
	size_t              os_window_size;

	struct renderer_t  *renderer;
	int                 renderer_size;
} ct_engine_t;

ct_engine_t *g_engine = NULL;

fn_export void
create_ct_engine()
{
    create_memory_subsystem();

	g_engine = SYS_ALLOC_STRUCT(ct_engine_t);
	ZERO_STRUCT(g_engine);

	os_window_create(1920, 1080, "Chibi Tech", NULL, &g_engine->os_window_size);
	g_engine->client_window = SYS_ALLOC(g_engine->os_window_size);
	os_window_create(1920, 1080, "Chibi Tech", g_engine->client_window, &g_engine->os_window_size);

	renderer_create(NULL, NULL, &g_engine->renderer_size);
	g_engine->renderer = SYS_ALLOC(g_engine->renderer_size);

	struct os_window_surface native_surface = os_window_get_surface(g_engine->client_window);

	renderer_info_t render_info = {
	    .software_name    = "Chibi Tech",
		.software_version = MAKE_APP_VERSION(0, 0, 1),
		.native_surface   = &native_surface,
	};

	renderer_create(&render_info, g_engine->renderer, &g_engine->renderer_size);

	u32 width, height;
	os_window_get_dims(g_engine->client_window, &width, &height);
	renderer_on_resize(g_engine->renderer, width, height);

	os_window_show(g_engine->client_window, WINDOW_SHOW_FLAG_NORMAL);

	guid_t test = platform_generate_guid();

	char test_buff[1024];
	platfor_guid_to_string(test, test_buff, 1024);

	ASSERT(platform_is_guid_valid(test));

	LOG_DEBUG("%s", test_buff);

	guid_t other_guid = platform_string_to_guid(test_buff);

	char test_buff_two[1024];
	platfor_guid_to_string(other_guid, test_buff_two, 1024);
	LOG_DEBUG("%s", test_buff_two);

	ASSERT(platform_is_guid_valid(other_guid));
	ASSERT(platform_compare_guids(&test, &other_guid));
}

fn_export void
destroy_ct_engine()
{
	os_window_destroy(g_engine->client_window);
	SYS_FREE(g_engine->client_window);

	SYS_FREE(g_engine);

	destroy_memory_subsystem();
}

fn_export bool
ct_begin_frame()
{
    reset_frame_allocator(0);

	os_window_process_pending_messages(g_engine->client_window);
	if (!os_window_is_valid(g_engine->client_window)) return false;

	renderer_on_render(g_engine->renderer);

	return true;
}
