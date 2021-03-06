#ifndef CE_TASKMANAGER_H
#define CE_TASKMANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#define CE_TASK_API \
    CE_ID64_0("ce_task_a0", 0x4dbd12f32a50782eULL)

//! Worker enum
typedef enum ce_workers_e0 {
    TASK_WORKER_MAIN = 0,  //!< Main worker
    TASK_MAX_WORKERS = 32, //!< Max workers
} ce_workers_e0;

//! Task item struct
typedef struct ce_task_item_t0 {
    const char *name;               //!< Task name
    void (*work)(void *data);       //!< Task work
    void *data;                     //!< Worker data
} ce_task_item_t0;

typedef struct ce_task_counter_t0 ce_task_counter_t0;

//! Task API V0
struct ce_task_a0 {
    //! Workers count
    //! \return Workers count
    int (*worker_count)();

    //! Curent worker id
    //! \return Worker id
    char (*worker_id)();

    //! Add new task
    //! \param items Task item array
    //! \param count Task item count
    void (*add)(ce_task_item_t0 *items,
                uint32_t count,
                ce_task_counter_t0 **counter);

    void (*add_specific)(uint32_t worker_id,
                         ce_task_item_t0 *items,
                         uint32_t count,
                         ce_task_counter_t0 **counter);

    void (*wait_for_counter)(ce_task_counter_t0 *signal,
                             int32_t value);

    void (*wait_for_counter_no_work)(ce_task_counter_t0 *signal,
                                     int32_t value);
};

CE_MODULE(ce_task_a0);

#ifdef __cplusplus
};
#endif

#endif //CE_TASKMANAGER_H
