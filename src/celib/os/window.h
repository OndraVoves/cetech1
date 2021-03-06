//                          **OS window**
//

#ifndef CE_OS_WINDOW_H
#define CE_OS_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "celib/celib_types.h"

#define CE_OS_WINDOW_API \
    CE_ID64_0("ce_os_window_a0", 0xe0214a9e3b01137bULL)


enum {
    EVENT_WINDOW_INVALID = 0,   //< Invalid type

    EVENT_WINDOW_RESIZED = 13, //< Window resized
};

struct ce_window_resized_event {
    uint32_t window_id;
    int32_t width;
    int32_t height;
};

enum ce_window_flags {
    WINDOW_NOFLAG = (1 << 0),
    WINDOW_FULLSCREEN = (1 << 1),
    WINDOW_SHOWN = (1 << 2),
    WINDOW_HIDDEN = (1 << 3),
    WINDOW_BORDERLESS = (1 << 4),
    WINDOW_RESIZABLE = (1 << 5),
    WINDOW_MINIMIZED = (1 << 6),
    WINDOW_MAXIMIZED = (1 << 7),
    WINDOW_INPUT_GRABBED = (1 << 8),
    WINDOW_INPUT_FOCUS = (1 << 9),
    WINDOW_MOUSE_FOCUS = (1 << 10),
    WINDOW_FULLSCREEN_DESKTOP = (1 << 11),
    WINDOW_ALLOW_HIGHDPI = (1 << 12),
    WINDOW_MOUSE_CAPTURE = (1 << 13),
    WINDOW_ALWAYS_ON_TOP = (1 << 14),
    WINDOW_SKIP_TASKBAR = (1 << 15),
    WINDOW_UTILITY = (1 << 16),
    WINDOW_TOOLTIP = (1 << 17),
    WINDOW_POPUP_MENU = (1 << 18),
};

enum ce_window_pos {
    WINDOWPOS_NOFLAG = (1 << 0),
    WINDOWPOS_CENTERED = (1 << 1),
    WINDOWPOS_UNDEFINED = (1 << 2),
};

typedef void ce_window_o0;

typedef struct ce_window_t0 {
    ce_window_o0 *inst;

    void (*set_title)(ce_window_o0 *inst,
                      const char *title);

    const char *(*get_title)(ce_window_o0 *inst);

    void (*resize)(ce_window_o0 *inst,
                   uint32_t width,
                   uint32_t height);

    void (*size)(ce_window_o0 *inst,
                 uint32_t *width,
                 uint32_t *height);

    void *(*native_window_ptr)(ce_window_o0 *inst);

    void *(*native_display_ptr)(ce_window_o0 *inst);

    void (*warp_mouse)(ce_window_o0 *inst,
                       int x,
                       int y);
} ce_window_t0;


struct ce_os_window_a0 {
    ce_window_t0 *(*create)(const char *title,
                            enum ce_window_pos x,
                            enum ce_window_pos y,
                            const int32_t width,
                            const int32_t height,
                            uint32_t flags,
                            ce_alloc_t0 *alloc);

    ce_window_t0 *(*create_from)(void *hndl,
                                 ce_alloc_t0 *alloc);

    void (*destroy)(ce_window_t0 *w,
                    ce_alloc_t0 *alloc);
};

CE_MODULE(ce_os_window_a0);

#ifdef __cplusplus
};
#endif

#endif //CE_OS_WINDOW_H
