//==============================================================================
// Includes
//==============================================================================

#include <engine/core/memory_system.h>
#include <engine/core/cvar.h>
#include "celib/memory/memory.h"
#include "engine/core/cvar.h"
#include "celib/string/string.h"

//==============================================================================
// Defines
//==============================================================================

#define MAX_VARIABLES 1024
#define MAX_NAME_LEN 128
#define MAX_DESC_LEN 256
#define LOG_WHERE "cvar"

#define make_cvar(i) (cvar_t){.idx = i}

//==============================================================================
// Enums
//==============================================================================

enum cvar_type {
    CV_NONE = 0,
    CV_FLOAT,
    CV_INT,
    CV_STRING
};

static const char *_type_to_str[4] = {
        [CV_NONE] = "invalid",
        [CV_FLOAT] = "float",
        [CV_INT] = "int",
        [CV_STRING] = "string"
};

//==============================================================================
// Globals
//==============================================================================

#define _G ConfigSystemGlobals


static struct G {
    char name[MAX_VARIABLES][MAX_NAME_LEN];
    char desc[MAX_VARIABLES][MAX_DESC_LEN];

    enum cvar_type types[MAX_VARIABLES];

    union {
        float f;
        int i;
        char *s;
    } values[MAX_VARIABLES];
} ConfigSystemGlobals = {0};


//==============================================================================
// Privates
//==============================================================================


void _dealloc_allm_string() {
    for (int i = 0; i < MAX_VARIABLES; ++i) {
        if (_G.types[i] != CV_STRING) {
            continue;
        }

        CE_DEALLOCATE(memsys_main_allocator(), _G.values[i].s);
    }
}

cvar_t _find_first_free() {

    for (u64 i = 1; i < MAX_VARIABLES; ++i) {
        if (_G.name[i][0] != '\0') {
            continue;
        }

        return make_cvar(i);
    }

    log_error(LOG_WHERE, "Could not create new config variable");

    return make_cvar(0);
}

//==============================================================================
// Interface
//==============================================================================

int cvar_init() {
    log_debug(LOG_WHERE, "Init");

    return 1;
}

void cvar_shutdown() {
    log_debug(LOG_WHERE, "Shutdown");

    _dealloc_allm_string();
}


int cvar_parse_args(struct args args) {
    if (os_cmd_has_argument(args, "cvars", 0)) {
        for (u64 i = 1; i < MAX_VARIABLES; ++i) {
            if (_G.name[i][0] == '\0') {
                continue;
            }

            log_info(LOG_WHERE, "%s : %s - %s", _G.name[i], _type_to_str[_G.types[i]], _G.desc[i]);

        }
        return 0;
    }


    struct args tmp_args = args;
    for (int j = 0; j < tmp_args.argc; ++j) {
        int idx = os_cmd_find_argument(tmp_args, "set", 's');
        if (idx == tmp_args.argc) {
            return 1;
        }

        const char *name = os_cmd_get_parameter(tmp_args, "set", 's', 0);
        const char *value = os_cmd_get_parameter(tmp_args, "set", 's', 1);

        log_info(LOG_WHERE, "%s : %s ", name, value);

        union {
            float f;
            int i;
            const char *s;
        } tmp_var;

        cvar_t cvar = cvar_find(name);
        if (cvar.idx != 0) {
            enum cvar_type type = _G.types[cvar.idx];
            switch (type) {
                case CV_FLOAT:
                    sscanf(value, "%f", &tmp_var.f);
                    cvar_set_float(cvar, tmp_var.f);
                    break;

                case CV_INT:
                    sscanf(value, "%d", &tmp_var.i);
                    cvar_set_int(cvar, tmp_var.i);
                    break;

                case CV_STRING:
                    cvar_set_string(cvar, value);
                    break;

                default:
                    log_error(LOG_WHERE, "Invalid type for cvar \"%s\"", name);
                    break;
            }

        } else {
            log_error(LOG_WHERE, "Invalid cvar \"%s\"", name);
        }

        tmp_args.argc -= 1 + 2;
        tmp_args.argv = tmp_args.argv + idx + 1 + 2;
        j = 0;
    }

    if (!os_cmd_has_argument(args, "set", 's')) {
        return 1;
    }


    return 1;
}

cvar_t cvar_find(const char *name) {
    for (u64 i = 1; i < MAX_VARIABLES; ++i) {
        if (_G.name[i][0] == '\0') {
            continue;
        }

        if (str_compare(_G.name[i], name) != 0) {
            continue;
        }

        return make_cvar(i);
    }

    return make_cvar(0);
}

cvar_t cvar_find_or_create(const char *name,
                           int *new) {
    if (new) *new = 0;

    for (u64 i = 1; i < MAX_VARIABLES; ++i) {
        if (_G.name[i][0] == '\0') {
            continue;
        }

        if (str_compare(_G.name[i], name) != 0) {
            continue;
        }

        return make_cvar(i);
    }

    const cvar_t var = _find_first_free();

    if (var.idx != 0) {
        str_set(_G.name[var.idx], name);

        if (new) *new = 1;
        return var;
    }

    return make_cvar(0);
}

cvar_t cvar_new_float(const char *name,
                      const char *desc,
                      float f) {
    int new;
    cvar_t find = cvar_find_or_create(name, &new);

    if (new) {
        str_set(_G.name[find.idx], name);
        _G.types[find.idx] = CV_FLOAT;
        _G.values[find.idx].f = f;
    }

    str_set(_G.desc[find.idx], desc);

    return find;
}

cvar_t cvar_new_int(const char *name,
                    const char *desc,
                    int i) {
    int new;
    cvar_t find = cvar_find_or_create(name, &new);

    if (new) {
        str_set(_G.name[find.idx], name);
        _G.types[find.idx] = CV_INT;
        _G.values[find.idx].i = i;
    }

    str_set(_G.desc[find.idx], desc);

    return find;
}

cvar_t cvar_new_str(const char *name,
                    const char *desc,
                    const char *s) {
    int new;
    cvar_t find = cvar_find_or_create(name, &new);

    if (new) {
        str_set(_G.name[find.idx], name);
        _G.types[find.idx] = CV_STRING;
        _G.values[find.idx].s = str_duplicate(s, memsys_main_allocator());
    }

    str_set(_G.desc[find.idx], desc);

    return find;
}

float cvar_get_float(cvar_t var) {
    return _G.values[var.idx].f;
}

int cvar_get_int(cvar_t var) {
    return _G.values[var.idx].i;
}

const char *cvar_get_string(cvar_t var) {
    return _G.values[var.idx].s;
}

void cvar_set_float(cvar_t var,
                    float f) {
    _G.values[var.idx].f = f;
}

void cvar_set_int(cvar_t var,
                  int i) {
    _G.values[var.idx].i = i;
}

void cvar_set_string(cvar_t var,
                     const char *s) {
    char *_s = _G.values[var.idx].s;

    if (_s != NULL) {
        allocator_deallocate(memsys_main_allocator(), _s);
    }

    _G.values[var.idx].s = str_duplicate(s, memsys_main_allocator());
}

