//! \defgroup Entity
//! Entity system
//! \{
#ifndef CETECH_ENTITY_MANAGER_H
#define CETECH_ENTITY_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// Includes
//==============================================================================

#include <cetech/celib/array.inl>

typedef struct yaml_node_s yaml_node_t;
typedef struct world_s world_t;

struct entity_compile_output;
struct compilator_api;


//==============================================================================
// Typedefs
//==============================================================================

//! Entity typedef
typedef struct entity_s {
    uint32_t h;
} entity_t;

//==============================================================================
// Api
//==============================================================================

//! Entity system API V0
struct entity_api_v0 {
    //! Create new entity
    //! \return New entity
    entity_t (*create)();

    //! Destroy entities
    //! \param world World
    //! \param entity entities
    //! \param count entity count
    void (*destroy)(world_t world,
                    entity_t *entity,
                    uint32_t count);

    //! Is entity alive?
    //! \param entity Entity
    //! \return 1 if entity is alive else 0
    int (*alive)(entity_t entity);


    //! Spawn entity from resource data
    //! \param world World
    //! \param resource Resource data
    //! \return Spawne entity array
    ARRAY_T(entity_t) *(*spawn_from_resource)(world_t world,
                                              void *resource);

    //! Spawn entity
    //! \param world World
    //! \param name Resource name
    //! \return New entity
    entity_t (*spawn)(world_t world,
                      uint64_t name);

#ifdef CETECH_CAN_COMPILE

    //! Create compiler output structure
    //! \return New compiler output structure
    struct entity_compile_output *(*compiler_create_output)();

    //! Destroy compiler output structure
    //! \param output Compiler output
    void (*compiler_destroy_output)(struct entity_compile_output *output);

    //! Compile entity
    //! \param output Compile output
    //! \param root Yaml root node
    //! \param filename Resource filename
    //! \param compilator_api Compilator api
    void (*compiler_compile_entity)(struct entity_compile_output *output,
                                    yaml_node_t root,
                                    const char *filename,
                                    struct compilator_api *compilator_api);

    //! Get entity counter from output
    //! \param output Compiler output
    //! \return Entity counter
    uint32_t (*compiler_ent_counter)(struct entity_compile_output *output);

    //! Write output to build
    //! \param output Output
    //! \param build Build
    void (*compiler_write_to_build)(struct entity_compile_output *output,
                                    ARRAY_T(uint8_t) *build);

    //! Resource compile
    //! \param root Root yaml node
    //! \param filename Filename
    //! \param build Build
    //! \param compilator_api Compilator api
    void (*resource_compiler)(yaml_node_t root,
                              const char *filename,
                              ARRAY_T(uint8_t) *build,
                              struct compilator_api *compilator_api);

#endif
};

#ifdef __cplusplus
}
#endif

#endif //CETECH_ENTITY_MANAGER_H
//! \}