#ifndef CETECH_CELIB_TYPES_H
#define CETECH_CELIB_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>

#ifdef DEBUG

#define CE_ASSERT(where, condition)                                         \
    do {                                                                    \
        if (!(condition)) {                                                 \
            ce_os_error_a0->assert(where, #condition, __FILE__, __LINE__);  \
        }                                                                   \
    } while (0)
#else
#define CE_ASSERT(where, condition) \
    do {} while (0)
#endif

typedef struct ce_ptr_pair_t0 {
    void *ptr;
    uint32_t len;
} ce_ptr_pair_t0;

typedef struct ce_vec2_t {
    float x, y;
} ce_vec2_t;

typedef struct ce_vec3_t {
    float x, y, z;
} ce_vec3_t;

typedef struct ce_vec4_t {
    float x, y, z, w;
} ce_vec4_t;

typedef struct ce_mat4_t {
    float m[16];
} ce_mat4_t;

typedef struct ce_transform_t {
    ce_vec3_t pos;
    ce_vec4_t rot;
    ce_vec3_t scl;
} ce_transform_t;


typedef struct ce_time_t {
    uint64_t o;
} ce_time_t;


static const ce_vec2_t CE_VEC2_ZERO = (ce_vec2_t) {0.0f, 0.0f};
static const ce_vec2_t CE_VEC2_ONE = (ce_vec2_t) {1.0f, 1.0f};
static const ce_vec2_t CE_VEC2_UNIT_X = (ce_vec2_t) {1.0f, 0.0f};
static const ce_vec2_t CE_VEC2_UNIT_Y = (ce_vec2_t) {0.0f, 1.0f};

static const ce_vec3_t CE_VEC3_ZERO = (ce_vec3_t) {0.0f, 0.0f, 0.0f};
static const ce_vec3_t CE_VEC3_UNIT = (ce_vec3_t) {1.0f, 1.0f, 1.0f};
static const ce_vec3_t CE_VEC3_UNIT_X = (ce_vec3_t) {1.0f, 0.0f, 0.0f};
static const ce_vec3_t CE_VEC3_UNIT_Y = (ce_vec3_t) {0.0f, 1.0f, 0.0f};
static const ce_vec3_t CE_VEC3_UNIT_Z = (ce_vec3_t) {0.0f, 0.0f, 1.0f};

static const ce_vec4_t CE_VEC4_ZERO = (ce_vec4_t) {0.0f, 0.0f, 0.0f, 0.0f};
static const ce_vec4_t CE_VEC4_ONE = (ce_vec4_t) {1.0f, 1.0f, 1.0f, 1.0f};

static const ce_vec4_t CE_QUAT_IDENTITY = (ce_vec4_t) {0.0f, 0.0f, 0.0f, 1.0f};

#define  CE_MAT4_IDENTITY (ce_mat4_t){{1.0f, 0.0f, 0.0f, 0.0f,\
                                       0.0f, 1.0f, 0.0f, 0.0f,\
                                       0.0f, 0.0f, 1.0f, 0.0f,\
                                       0.0f, 0.0f, 0.0f, 1.0f}}

static const ce_transform_t CE_TRANFORM_INIT = {
        .pos = {0.0f, 0.0f, 0.0f},
        .rot = {0.0f, 0.0f, 0.0f, 1.0f},
        .scl = {1.0f, 1.0f, 1.0f}
};

typedef char cache_line_pad_t[64];

#define CE_ID64_0(str, hash) hash

#if defined(CE_DYNAMIC_MODULE)
#define CE_MODULE_API static
#else
#define CE_MODULE_API extern
#endif

#define CE_MODULE(name) CE_MODULE_API struct name* name

#include <celib/os/error.h>

#define CE_ALIGNOF(_type) __alignof(_type)

#ifdef __cplusplus
}
#endif

#endif //CETECH_CELIB_TYPES_H
