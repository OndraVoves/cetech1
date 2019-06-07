#include <celib/macros.h>
#include <celib/memory/allocator.h>
#include <celib/containers/array.h>
#include <celib/id.h>
#include <celib/memory/memory.h>
#include <celib/api.h>
#include <celib/module.h>
#include <celib/cdb.h>

#include <cetech/property_editor/property_editor.h>
#include <stdio.h>
#include <celib/fs.h>
#include <cetech/resource_browser/resource_browser.h>
#include <cetech/debugui/icons_font_awesome.h>

#include <cetech/renderer/gfx.h>
#include <cetech/debugui/debugui.h>
#include <cetech/editor/selcted_object.h>
#include <cetech/editor/editor_ui.h>

#include "../dock.h"

#define _G explorer_globals
static struct _G {
    uint64_t docks_n;
    uint64_t *docks;

    uint64_t *contexts;

    ce_alloc_t0 *allocator;
} _G;

#define MAX_CONTEXT 8

#define _PROP_DOCK_ID\
    CE_ID64_0("dock_id", 0x4a6df3bdedc53da2ULL)

struct ct_dock_i0 *_find_dock_i(uint64_t type) {
    ce_api_entry_t0 it = ce_api_a0->first(CT_DOCK_I);

    while (it.api) {
        struct ct_dock_i0 *i = (it.api);

        if (i && i->cdb_type
            && (i->cdb_type() == type)) {
            return i;
        }

        it = ce_api_a0->next(it);
    }

    return NULL;
}

uint64_t create_dock(uint64_t type,
                     bool visible) {

    ct_dock_i0 *i = _find_dock_i(type);

    if (!i) {
        return 0;
    }

    uint64_t obj = ce_cdb_a0->create_object(ce_cdb_a0->db(), DOCK_TYPE);

    uint64_t flags = 0;

    if (i->dock_flags) {
        flags = i->dock_flags();
    }

    ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), obj);
    ce_cdb_a0->set_bool(w, PROP_DOCK_VISIBLE, visible);
    ce_cdb_a0->set_uint64(w, PROP_DOCK_flags, (uint64_t) flags);
    ce_cdb_a0->set_uint64(w, _PROP_DOCK_ID, _G.docks_n++);
    ce_cdb_a0->set_uint64(w, PROP_DOCK_TYPE, type);
    ce_cdb_a0->write_commit(w);

    ce_array_push(_G.docks, obj, _G.allocator);

    if (i->open) {
        i->open(obj);
    }

    return obj;
}

void close_dock(uint64_t dock) {
    const ce_cdb_obj_o0 *dock_r = ce_cdb_a0->read(ce_cdb_a0->db(), dock);
    uint64_t type = ce_cdb_a0->read_uint64(dock_r, PROP_DOCK_TYPE, 0);

    ct_dock_i0 *i = _find_dock_i(type);

    if (i && i->close) {
        i->close(dock, 0);
    }

    uint32_t dock_n = ce_array_size(_G.docks);
    for (int u = 0; u < dock_n; ++u) {
        if (_G.docks[u] != dock) {
            continue;
        }

        uint32_t last_idx = dock_n - 1;
        _G.docks[u] = _G.docks[last_idx];
        ce_array_pop_back(_G.docks);
    }
}

void draw_all() {
    uint64_t n = ce_array_size(_G.docks);

    for (int i = 0; i < n; ++i) {
        uint64_t dock = _G.docks[i];
        const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), dock);

        uint64_t type = ce_cdb_a0->read_uint64(reader, PROP_DOCK_TYPE, 0);

        struct ct_dock_i0 *di = _find_dock_i(type);

        if (di) {
            uint64_t id = ce_cdb_a0->read_uint64(reader, _PROP_DOCK_ID, 0);
            char title[128] = {};
            snprintf(title, CE_ARRAY_LEN(title), "%s##%s_dock%llu",
                     di->display_title(dock), di->name(dock), id);

            uint64_t flags = ce_cdb_a0->read_uint64(reader, PROP_DOCK_flags, 0);
            bool visible = ce_cdb_a0->read_bool(reader, PROP_DOCK_VISIBLE, false);

            if (ct_debugui_a0->BeginDock(title, &visible, flags)) {

                uint64_t context = ce_cdb_a0->read_uint64(reader,
                                                          PROP_DOCK_CONTEXT, 0);

                if (ct_debugui_a0->BeginPopup("select_dock_context", 0)) {
                    for (int j = 0; j < MAX_CONTEXT; ++j) {
                        snprintf(title, CE_ARRAY_LEN(title), "%d", j);

                        bool selected = context == j;

                        if (ct_debugui_a0->MenuItem(title, NULL,
                                                    selected, true)) {
                            ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), dock);
                            ce_cdb_a0->set_uint64(w, PROP_DOCK_CONTEXT, j);
                            ce_cdb_a0->write_commit(w);
                        }
                    }
                    ct_debugui_a0->EndPopup();
                }

                if (di->draw_menu) {
                    di->draw_menu(dock);
                }

                snprintf(title, CE_ARRAY_LEN(title), "%s##%s_child_dock%llu",
                         di->display_title(dock), di->name(dock), id);

                ct_debugui_a0->BeginChild(title, (ce_vec2_t) {}, false, 0);

                if (di->draw_ui) {
                    di->draw_ui(dock);
                }
                ct_debugui_a0->EndChild();
            }
            ct_debugui_a0->EndDock();

            if (!visible) {
                close_dock(dock);
            }

//            ce_cdb_obj_o0 *w=ce_cdb_a0->write_begin(ce_cdb_a0->db(), dock);
//            ce_cdb_a0->set_bool(w, PROP_DOCK_VISIBLE, visible);
//            ce_cdb_a0->write_commit(w);

        }
    }
}

static const char *display_name(ct_dock_i0 *i) {
    const char *name = NULL;

    if (i->display_title) {
        name = i->display_title(0);
    }

    if (!name) {
        name = "(no name)";
    }
    return name;
}

static void draw_menu() {
    const uint64_t n = ce_array_size(_G.docks);

    if (ct_debugui_a0->BeginMenu("Docks", true)) {

        if (ct_debugui_a0->BeginMenu(ICON_FA_PLUS" ""Add new dock", true)) {
            struct ce_api_entry_t0 it = ce_api_a0->first(CT_DOCK_I);

            while (it.api) {
                struct ct_dock_i0 *i = (it.api);

                const char *name = display_name(i);

                if (ct_debugui_a0->MenuItem(name, NULL, false, true)) {
                    create_dock(i->cdb_type(), true);
                }

                it = ce_api_a0->next(it);
            }

            ct_debugui_a0->EndMenu();
        }

        if (ct_debugui_a0->BeginMenu("Layout", true)) {
            if (ct_debugui_a0->MenuItem("Save", NULL, false, true)) {
                struct ce_vio_t0 *f = ce_fs_a0->open(RESOURCE_BROWSER_SOURCE,
                                                     "core/default.dock_layout",
                                                     FS_OPEN_WRITE);
                ct_debugui_a0->SaveDock(f);
                ce_fs_a0->close(f);
            }

            if (ct_debugui_a0->MenuItem("Load", NULL, false, true)) {
                ct_debugui_a0->LoadDock("core/default.dock_layout");
            }
            ct_debugui_a0->EndMenu();
        }

        ct_debugui_a0->Separator();

        for (int i = 0; i < n; ++i) {
            uint64_t dock = _G.docks[i];

            const ce_cdb_obj_o0 *dock_r = ce_cdb_a0->read(ce_cdb_a0->db(), dock);
            uint64_t type = ce_cdb_a0->read_uint64(dock_r, PROP_DOCK_TYPE, 0);

            struct ct_dock_i0 *di = _find_dock_i(type);

            const char *name = display_name(di);

            char label[256] = {};
            snprintf(label, CE_ARRAY_LEN(label), "%s##menu_%llx", name, dock);

            if (ct_debugui_a0->BeginMenu(label, true)) {
                if (ct_debugui_a0->MenuItem(ICON_FA_WINDOW_CLOSE" ""Close", "", false, true)) {
                    close_dock(dock);
                }
                ct_debugui_a0->EndMenu();
            }
        }

        ct_debugui_a0->EndMenu();
    }

    for (int i = 0; i < n; ++i) {
        uint64_t dock = _G.docks[i];

        const ce_cdb_obj_o0 *dock_r = ce_cdb_a0->read(ce_cdb_a0->db(), dock);
        uint64_t type = ce_cdb_a0->read_uint64(dock_r, PROP_DOCK_TYPE, 0);

        struct ct_dock_i0 *di = _find_dock_i(type);

        if (di && di->draw_main_menu) {
            di->draw_main_menu(dock);
        }

    }
}

static uint64_t create_context(const char *name) {
    const uint64_t context_id = ce_id_a0->id64(name);
    const uint64_t context_idx = ce_array_size(_G.contexts);
    ce_array_push(_G.contexts, context_id, _G.allocator);
    return context_idx;
}

static bool context_btn(uint64_t dock) {
    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), dock);
    uint64_t context = ce_cdb_a0->read_uint64(reader,
                                              PROP_DOCK_CONTEXT, 0);
    char title[256] = {};
    snprintf(title, CE_ARRAY_LEN(title),
             ICON_FA_MAP_SIGNS
                     " %llu##dock_context_btn_%llx", context, dock);

    bool change = ct_debugui_a0->Button(title, &CE_VEC2_ZERO);
    if (change) {
        ct_debugui_a0->OpenPopup("select_dock_context");
    };

    ct_debugui_a0->SameLine(0, 4);

    bool has_prev = ct_selected_object_a0->has_previous(context);
    bool has_next = ct_selected_object_a0->has_next(context);

    snprintf(title, CE_ARRAY_LEN(title),
             ICON_FA_ARROW_LEFT
                     "##dock_context_btn_prev_selected%llx", dock);

    if (!has_prev) {
        ct_editor_ui_a0->begin_disabled();
    }

    if (ct_debugui_a0->Button(title, &CE_VEC2_ZERO)) {
        ct_selected_object_a0->set_previous(context);
    };

    if (!has_prev) {
        ct_editor_ui_a0->end_disabled();
    }

    ct_debugui_a0->SameLine(0, 4);

    snprintf(title, CE_ARRAY_LEN(title),
             ICON_FA_ARROW_RIGHT
                     "##dock_context_btn_next_selected%llx", dock);

    if (!has_next) {
        ct_editor_ui_a0->begin_disabled();
    }

    if (ct_debugui_a0->Button(title, &CE_VEC2_ZERO)) {
        ct_selected_object_a0->set_next(context);
    };

    if (!has_next) {
        ct_editor_ui_a0->end_disabled();
    }

    return change;
}

struct ct_dock_a0 dock_a0 = {
        .draw_all = draw_all,
        .draw_menu = draw_menu,
        .create_dock = create_dock,
        .context_btn = context_btn,
};

struct ct_dock_a0 *ct_dock_a0 = &dock_a0;

static const ce_cdb_prop_def_t0 dock_cdb_type_def[] = {
        {
                .name = "visible",
                .type = CE_CDB_TYPE_BOOL,
        },
        {
                .name = "flags",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "dock_id",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "dock_type",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "dock_data",
                .type = CE_CDB_TYPE_PTR,
        },
        {
                .name = "locked_obj",
                .type = CE_CDB_TYPE_REF,
        },
};

static const ce_cdb_prop_def_t0 docks_layout_cdb_type_def[] = {
        {
                .name = "docks",
                .type = CE_CDB_TYPE_SET_SUBOBJECT,
        },
};

static const ce_cdb_prop_def_t0 dock_layout_cdb_type_def[] = {
        {
                .name = "index",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "label",
                .type = CE_CDB_TYPE_STR,
        },
        {
                .name = "location",
                .type = CE_CDB_TYPE_STR,
        },
        {
                .name = "x",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "y",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "size_x",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "size_y",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "status",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "active",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "opened",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "prev",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "next",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "child0",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "child1",
                .type = CE_CDB_TYPE_UINT64,
        },
        {
                .name = "parent",
                .type = CE_CDB_TYPE_UINT64,
        },
};

void CE_MODULE_LOAD(dock)(struct ce_api_a0 *api,
                          int reload) {
    CE_UNUSED(reload);
    CE_INIT_API(api, ce_memory_a0);
    CE_INIT_API(api, ce_module_a0);

    _G = (struct _G) {
            .allocator = ce_memory_a0->system,
    };

    ce_api_a0->register_api(CT_DOCK_API, ct_dock_a0, sizeof(dock_a0));
    ce_cdb_a0->reg_obj_type(DOCK_TYPE, CE_ARR_ARG(dock_cdb_type_def));
    ce_cdb_a0->reg_obj_type(DOCKS_LAYOUT_TYPE, CE_ARR_ARG(docks_layout_cdb_type_def));
    ce_cdb_a0->reg_obj_type(DOCK_LAYOUT_TYPE, CE_ARR_ARG(dock_layout_cdb_type_def));

    create_context("");
}

void CE_MODULE_UNLOAD(dock)(struct ce_api_a0 *api,
                            int reload) {

    CE_UNUSED(reload);
    CE_UNUSED(api);

    _G = (struct _G) {};
}