#ifndef CETECH_PRIMITIVE_MESH_H
#define CETECH_PRIMITIVE_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct ct_primitive_mesh {
    uint64_t material;
} ct_primitive_mesh_c;


#define PRIMITIVE_MESH_COMPONENT \
    CE_ID64_0("primitive_mesh", 0x41994de0b780f885ULL)

#define PRIMITIVE_MESH_MATERIAL_PROP \
    CE_ID64_0("material", 0xeac0b497876adedfULL)

#ifdef __cplusplus
};
#endif

#endif //CETECH_PRIMITIVE_MESH_H