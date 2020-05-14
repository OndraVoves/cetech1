#ifndef CE_CDB_H
#define CE_CDB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "celib_types.h"

#define CE_CDB_A0_STR "ce_cdb_a0"

#define CE_CDB_API \
    CE_ID64_0("ce_cdb_a0", 0xf069efc5d4120b7bULL)

#define CDB_INSTANCE_PROP \
    CE_ID64_0("cdb_instance", 0xb0f74f1d9d7c645dULL)

#define CDB_UID_PROP \
     CE_ID64_0("cdb_uuid", 0x958a636d68e82bd7ULL)

#define CDB_TYPE_PROP \
     CE_ID64_0("cdb_type", 0xfe5986c682be99e0ULL)

#define CDB_OBJSET \
     CE_ID64_0("cdb_objset", 0x2b66a0c3813b3490ULL)

#define CE_CDB_PROP_CHANGE_EVENT \
    CE_ID64_0("change", 0x8694ed4881bfb631ULL)

#define CE_CDB_PROP_MOVE_EVENT \
    CE_ID64_0("move", 0x33603ac62788b5c5ULL)

#define CE_CDB_OBJSET_ADD_EVENT \
    CE_ID64_0("objset_add", 0x7390985657b354e9ULL)

#define CE_CDB_OBJSET_REMOVE_EVENT \
    CE_ID64_0("objset_remove", 0xd973a462dce1b7c8ULL)

#define CE_CDB_OBJ_CHANGE_EVENT \
    CE_ID64_0("obj_change", 0x2cf62adf15c17305ULL)

#define CE_CDB_OBJ_DESTROY_EVENT \
    CE_ID64_0("obj_destroy", 0x5669bedb7db786e4ULL)

typedef struct ce_alloc_t0 ce_alloc_t0;
typedef struct ce_uuid64_t0 ce_uuid64_t0;

typedef struct ce_cdb_uuid_t0 {
    uint64_t id;
} ce_cdb_uuid_t0;

typedef enum ce_cdb_type_e0 {
    CE_CDB_TYPE_NONE = 0,
    CE_CDB_TYPE_UINT64 = 1,
    CE_CDB_TYPE_PTR = 2,
    CE_CDB_TYPE_REF = 3,
    CE_CDB_TYPE_FLOAT = 4,
    CE_CDB_TYPE_BOOL = 5,
    CE_CDB_TYPE_STR = 6,
    CE_CDB_TYPE_SUBOBJECT = 7,
    CE_CDB_TYPE_BLOB = 8,
    CE_CDB_TYPE_SET_SUBOBJECT = 9,
} ce_cdb_type_e0;

typedef enum ce_cdb_prop_flag_e0 {
    CE_CDB_PROP_FLAG_NONE = 0,
    CE_CDB_PROP_FLAG_UNPACK = 1 << 0,
    CE_CDB_PROP_FLAG_RUNTIME = 1 << 1,
} ce_cdb_flag_e0;

typedef struct ce_cdb_obj_o0 ce_cdb_obj_o0;

typedef struct ce_cdb_t0 {
    uint64_t idx;
} ce_cdb_t0;

typedef struct ce_cdb_blob_t0 {
    void *data;
    uint64_t size;
} ce_cdb_blob_t0;

typedef union ce_cdb_value_u0 {
    uint64_t uint64;
    void *ptr;
    uint64_t ref;
    uint64_t subobj;

    float f;
    char *str;
    bool b;
    uint32_t blob;
    uint32_t set;
} ce_cdb_value_u0;

typedef struct ce_cdb_ev_t0 {
    uint64_t ev_type;
    uint64_t obj;
    uint64_t obj_type;
} ce_cdb_ev_t0;

typedef struct ce_cdb_prop_ev_t0 {
    uint64_t ev_type;
    uint64_t obj;
    uint64_t prop;
    uint64_t prop_type;
    union {
        struct {
            union ce_cdb_value_u0 new_value;
            union ce_cdb_value_u0 old_value;
        };
        struct {
            uint64_t to;
            union ce_cdb_value_u0 value;
        };
    };
} ce_cdb_prop_ev_t0;

typedef struct cdb_binobj_header {
    uint64_t version;
    uint64_t node_count;
    uint64_t string_buffer_size;
    uint64_t blob_buffer_size;
} cdb_binobj_header;

typedef enum ct_cdb_node_type_e0 {
    CT_CDB_NODE_INVALID = 0,
    CT_CDB_NODE_FLOAT,
    CT_CDB_NODE_UINT,
    CT_CDB_NODE_REF,
    CT_CDB_NODE_STRING,
    CT_CDB_NODE_BOOL,
    CT_CDB_NODE_BLOB,
    CT_CDB_NODE_OBJ_BEGIN,
    CT_CDB_NODE_OBJ_END,
    CT_CDB_NODE_OBJSET,
    CT_CDB_NODE_OBJSET_END,
} ct_cdb_node_type_e0;

typedef struct ct_cdb_node_t {
    ct_cdb_node_type_e0 type;

    uint64_t key;
    union {
        struct {
            ce_cdb_uuid_t0 uuid;
            uint64_t type;
            ce_cdb_uuid_t0 instance_of;
        } obj;

        struct {
            void *data;
            uint64_t size;
        } blob;

        ce_cdb_uuid_t0 uuid;

        ce_cdb_value_u0 value;
    };
} ct_cdb_node_t;

typedef uint64_t (*ct_cdb_obj_loader_t0)(ce_cdb_t0 db,
                                         ce_cdb_uuid_t0 uuid);

typedef struct ce_cdb_prop_def_t0 {
    const char *name;
    uint8_t type;
    ce_cdb_value_u0 value;
    uint64_t obj_type;
    uint64_t flags;
} ce_cdb_prop_def_t0;

typedef struct ct_cdb_type_def_t0 {
    const ce_cdb_prop_def_t0 *props;
    uint32_t n;
} ct_cdb_type_def_t0;

struct ce_cdb_a0 {
    void (*set_loader)(ct_cdb_obj_loader_t0 loader);

    ce_cdb_t0 (*db)();

    ce_cdb_t0 (*create_db)(uint64_t max_objects);

    void (*destroy_db)(ce_cdb_t0 db);


    void (*reg_obj_type)(uint64_t type,
                         const ce_cdb_prop_def_t0 *prop_def,
                         uint32_t n);

    ct_cdb_type_def_t0 (*obj_type_def)(uint64_t type);
    ce_cdb_prop_def_t0 (*obj_type_prop_def)(uint64_t type, uint64_t prop);

    ce_cdb_uuid_t0 (*gen_uid)(ce_cdb_t0 db);

    uint64_t (*create_object)(ce_cdb_t0 db,
                              uint64_t type);

    uint64_t (*create_object_uid)(ce_cdb_t0 db,
                                  ce_cdb_uuid_t0 uuid,
                                  uint64_t type,
                                  bool init);

    uint64_t (*create_from)(ce_cdb_t0 db,
                            uint64_t obj);

    uint64_t (*create_from_uid)(ce_cdb_t0 db,
                                uint64_t from,
                                ce_cdb_uuid_t0 uuid);

    void (*destroy_object)(ce_cdb_t0 db,
                           uint64_t obj);

    uint64_t (*obj_type)(ce_cdb_t0 db,
                         uint64_t obj);

    //
    void (*move_obj)(ce_cdb_t0 db,
                     uint64_t from_obj,
                     uint64_t to);

    void (*move_objset_obj)(ce_cdb_obj_o0 *from_w,
                            ce_cdb_obj_o0 *to_w,
                            uint64_t prop,
                            uint64_t obj);

    //

    const ce_cdb_ev_t0 *(*changes)(ce_cdb_t0 db,
                                   uint32_t *n);

    const ce_cdb_prop_ev_t0 *(*objs_changes)(ce_cdb_t0 db,
                                             uint32_t *n);
    //

    void (*tick)();

    //

    void (*log_obj)(const char *where,
                    ce_cdb_t0 db,
                    uint64_t obj);

    void (*dump)(ce_cdb_t0 db,
                 uint64_t obj,
                 char **output,
                 ce_alloc_t0 *allocator);

    uint64_t (*load)(ce_cdb_t0 db,
                     const char *input,
                     ce_alloc_t0 *allocator);

    uint64_t (*find_root)(ce_cdb_t0 _db,
                          uint64_t obj);

    // PROP
    bool (*prop_exist)(const ce_cdb_obj_o0 *reader,
                       uint64_t key);

    enum ce_cdb_type_e0 (*prop_type)(const ce_cdb_obj_o0 *reader,
                                     uint64_t key);

    const uint64_t *(*prop_keys)(const ce_cdb_obj_o0 *reader);

    uint64_t (*prop_count)(const ce_cdb_obj_o0 *reader);

    bool (*prop_equal)(const ce_cdb_obj_o0 *r1,
                       const ce_cdb_obj_o0 *r2,
                       uint64_t prorp);

    void (*prop_copy)(const ce_cdb_obj_o0 *from,
                      ce_cdb_obj_o0 *to,
                      uint64_t prorp);

    uint64_t (*parent)(ce_cdb_t0 db,
                       uint64_t object);

    // SET
    ce_cdb_obj_o0 *(*write_begin)(ce_cdb_t0 db,
                                  uint64_t object);

    void (*write_commit)(ce_cdb_obj_o0 *writer);

    bool (*write_try_commit)(ce_cdb_obj_o0 *writer);


    void (*set_bool)(ce_cdb_obj_o0 *writer,
                     uint64_t property,
                     bool value);

    void (*set_float)(ce_cdb_obj_o0 *writer,
                      uint64_t property,
                      float value);

    void (*set_str)(ce_cdb_obj_o0 *writer,
                    uint64_t property,
                    const char *value);

    void (*set_uint64)(ce_cdb_obj_o0 *writer,
                       uint64_t property,
                       uint64_t value);

    void (*set_ptr)(ce_cdb_obj_o0 *writer,
                    uint64_t property,
                    const void *value);

    void (*set_blob)(ce_cdb_obj_o0 *writer,
                     uint64_t property,
                     void *blob,
                     uint64_t blob_size);

    void (*set_ref)(ce_cdb_obj_o0 *writer,
                    uint64_t property,
                    uint64_t ref);


    void (*set_subobject)(ce_cdb_obj_o0 *writer,
                          uint64_t property,
                          uint64_t subobject);

    void (*objset_add_obj)(ce_cdb_obj_o0 *writer,
                           uint64_t property,
                           uint64_t obj);

    void (*objset_remove_obj)(ce_cdb_obj_o0 *writer,
                              uint64_t property,
                              uint64_t obj);

    // READ
    const ce_cdb_obj_o0 *(*read)(ce_cdb_t0 db,
                                 uint64_t object);

    uint64_t (*read_to)(ce_cdb_t0 db,
                        uint64_t object,
                        void *to,
                        size_t max_size);

    uint64_t (*read_instance_of)(const ce_cdb_obj_o0 *reader);

    float (*read_float)(const ce_cdb_obj_o0 *reader,
                        uint64_t property,
                        float defaultt);

    bool (*read_bool)(const ce_cdb_obj_o0 *reader,
                      uint64_t property,
                      bool defaultt);

    const char *(*read_str)(const ce_cdb_obj_o0 *reader,
                            uint64_t property,
                            const char *defaultt);

    uint64_t (*read_uint64)(const ce_cdb_obj_o0 *reader,
                            uint64_t property,
                            uint64_t defaultt);

    void *(*read_ptr)(const ce_cdb_obj_o0 *reader,
                      uint64_t property,
                      void *defaultt);

    void *(*read_blob)(const ce_cdb_obj_o0 *reader,
                       uint64_t property,
                       uint64_t *size,
                       void *defaultt);

    uint64_t (*read_ref)(const ce_cdb_obj_o0 *reader,
                         uint64_t property,
                         uint64_t defaultt);

    ce_cdb_uuid_t0 (*read_ref_uuid)(const ce_cdb_obj_o0 *reader,
                                    uint64_t property,
                                    ce_cdb_uuid_t0 defaultt);

    uint64_t (*read_subobject)(const ce_cdb_obj_o0 *reader,
                               uint64_t property,
                               uint64_t defaultt);

    uint64_t (*read_objset_num)(const ce_cdb_obj_o0 *reader,
                                uint64_t property);

    void (*read_objset)(const ce_cdb_obj_o0 *reader,
                        uint64_t property,
                        uint64_t *objs);

    uint64_t (*obj_from_uid)(ce_cdb_t0 db,
                             ce_cdb_uuid_t0 uuid);

    ce_cdb_uuid_t0 (*obj_uid)(ce_cdb_t0 db,
                              uint64_t obj);

    uint64_t (*load_from_cnodes)(const ct_cdb_node_t *cnodes,
                                 ce_cdb_t0 tmp_db);

    void (*dump_cnodes)(const ct_cdb_node_t *cnodes,
                        char **outputs);

    void *(*get_aspect)(uint64_t type,
                        uint64_t aspect_type);

    void (*set_aspect)(uint64_t type,
                       uint64_t aspect_type,
                       void *aspect);
};

extern void foo();

CE_MODULE(ce_cdb_a0);

#ifdef __cplusplus
};
#endif

#endif //CE_CDB_H
