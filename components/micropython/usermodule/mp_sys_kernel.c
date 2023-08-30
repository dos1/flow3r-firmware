/// kernel is a micropython C module which allows access to the badge's
/// 'kernel', ie. FreeRTOS/ESP-IDF/... This is a low-level API intended to be
/// used for use by badge developers.

#include <stdio.h>
#include <string.h>
#include "flow3r_bsp.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "st3m_console.h"
#include "st3m_io.h"
#include "st3m_usb.h"
#include "st3m_version.h"

#if (configUSE_TRACE_FACILITY != 1)
#error config_USE_TRACE_FACILITY must be set
#endif

/// task object which represents a snapshot of a FreeRTOS task at a given time.
///
/// Properties:
///  - number: The FreeRTOS task number
///  - stack_left: High water mark of stack usage by task, ie. highest ever
///                recorded use of stack. The units seem arbitrary.
///  - run_time: The run time allocated to this task so far, as defined by the
///              FreeRTOS run time stats clock. The units are arbitrary and
///              should only be used comparatively against other task runtimes,
///              and the global total runtime value from scheduler_stats.
///  - state: one of kernel.{RUNNING,READY,BLOCKED,SUSPENDED,DELETED,INVALID}
///  - core_affinity: bitmask of where this task is allowed to be scheduled. Bit
///                   0 is core 0, bit 1 is core 1. The value 0b11 (3) means the
///                   task is allowed to run on any core.
///  - percent: cpu load in percent since last update

typedef struct _task_obj_t {
    mp_obj_base_t base;

    char name[configMAX_TASK_NAME_LEN];
    uint32_t number;
    uint16_t stack_left;
    uint32_t run_time;
    uint32_t cpu_percent;
    eTaskState state;
    uint32_t core_affinity;
} task_obj_t;

const mp_obj_type_t task_type;

STATIC void task_print(const mp_print_t *print, mp_obj_t self_in,
                       mp_print_kind_t kind) {
    (void)kind;
    task_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_print_str(print, "Task(name=");
    mp_print_str(print, self->name);
    mp_print_str(print, ",state=");
    switch (self->state) {
        case eRunning:
            mp_print_str(print, "RUNNING");
            break;
        case eReady:
            mp_print_str(print, "READY");
            break;
        case eBlocked:
            mp_print_str(print, "BLOCKED");
            break;
        case eSuspended:
            mp_print_str(print, "SUSPENDED");
            break;
        case eDeleted:
            mp_print_str(print, "DELETED");
            break;
        case eInvalid:
            mp_print_str(print, "INVALID");
            break;
        default:
            mp_print_str(print, "???");
            break;
    }
    mp_printf(print,
              ",number=%d,stack_left=%d,run_time=%d,,cpu_percent=%d,core_"
              "affinity=%d)",
              self->number, self->stack_left, self->run_time, self->cpu_percent,
              self->core_affinity);
}

STATIC void task_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    task_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }
    switch (attr) {
        case MP_QSTR_name:
            dest[0] = mp_obj_new_str(self->name, strlen(self->name));
            break;
        case MP_QSTR_state:
            dest[0] = MP_OBJ_NEW_SMALL_INT(self->state);
            break;
        case MP_QSTR_number:
            dest[0] = mp_obj_new_int_from_uint(self->number);
            break;
        case MP_QSTR_stack_left:
            dest[0] = mp_obj_new_int_from_uint(self->stack_left);
            break;
        case MP_QSTR_run_time:
            dest[0] = mp_obj_new_int_from_uint(self->run_time);
            break;
        case MP_QSTR_cpu_percent:
            dest[0] = mp_obj_new_int_from_uint(self->cpu_percent);
            break;
        case MP_QSTR_core_affinity:
            dest[0] = mp_obj_new_int_from_uint(self->core_affinity);
            break;
    }
}

MP_DEFINE_CONST_OBJ_TYPE(task_type, MP_QSTR_task, MP_TYPE_FLAG_NONE, print,
                         task_print, attr, task_attr);

/// snapshot of the FreeRTOS scheduler state. Will not update dynamically,
/// instead needs to be re-created by calling scheduler_snapshot() again.
///
/// Properties:
///  - tasks: list of tasks
///  - total_runtime: the total run time since boot as defined by the FreeRTOS
///                   run time stats clock
typedef struct _scheduler_snapshot_obj_t {
    mp_obj_base_t base;

    mp_obj_t tasks;
    uint32_t total_runtime;
} scheduler_snapshot_obj_t;

const mp_obj_type_t scheduler_snapshot_type;

STATIC void scheduler_snapshot_print(const mp_print_t *print, mp_obj_t self_in,
                                     mp_print_kind_t kind) {
    (void)kind;
    scheduler_snapshot_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SchedulerSnapshot(tasks=[...], total_runtime=%d)",
              self->total_runtime);
}

STATIC void scheduler_snapshot_attr(mp_obj_t self_in, qstr attr,
                                    mp_obj_t *dest) {
    scheduler_snapshot_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }
    switch (attr) {
        case MP_QSTR_total_runtime:
            dest[0] = mp_obj_new_int_from_uint(self->total_runtime);
            break;
        case MP_QSTR_tasks:
            dest[0] = self->tasks;
            break;
    }
}

MP_DEFINE_CONST_OBJ_TYPE(scheduler_snapshot_type, MP_QSTR_scheduler_snapshot,
                         MP_TYPE_FLAG_NONE, print, scheduler_snapshot_print,
                         attr, scheduler_snapshot_attr);

STATIC mp_obj_t mp_scheduler_snapshot(void) {
    static uint32_t run_time_prev[50];
    static uint32_t total_runtime_prev;
    mp_obj_t tasks_out = mp_obj_new_list(0, NULL);

    UBaseType_t ntasks = uxTaskGetNumberOfTasks();
    TaskStatus_t *tasks = calloc(sizeof(TaskStatus_t), ntasks);
    if (tasks == NULL) {
        mp_raise_msg(&mp_type_MemoryError, "out of memory");
        return mp_const_none;
    }
    uint32_t total_runtime;
    UBaseType_t ntasks_returned =
        uxTaskGetSystemState(tasks, ntasks, &total_runtime);

    scheduler_snapshot_obj_t *snap = m_new_obj(scheduler_snapshot_obj_t);
    snap->base.type = &scheduler_snapshot_type;
    snap->total_runtime = total_runtime;
    uint32_t total_runtime_delta_percent =
        (snap->total_runtime - total_runtime_prev) / 100;

    for (UBaseType_t i = 0; i < ntasks_returned; i++) {
        task_obj_t *task = m_new_obj(task_obj_t);
        task->base.type = &task_type;
        strncpy(task->name, tasks[i].pcTaskName, configMAX_TASK_NAME_LEN - 1);
        task->number = tasks[i].xTaskNumber;
        task->stack_left = tasks[i].usStackHighWaterMark;
        task->run_time = tasks[i].ulRunTimeCounter;
        if (total_runtime_delta_percent && (task->number < 50)) {
            task->cpu_percent = (task->run_time - run_time_prev[task->number]) /
                                total_runtime_delta_percent;
            run_time_prev[task->number] = task->run_time;
        } else {
            task->cpu_percent = 300;  // indicates smth's wrong
        }

        task->state = tasks[i].eCurrentState;
        task->core_affinity = 0b11;
        switch (tasks[i].xCoreID) {
            case 0:
                task->core_affinity = 1;
                break;
            case 1:
                task->core_affinity = 2;
                break;
        }
        mp_obj_list_append(tasks_out, MP_OBJ_FROM_PTR(task));
    }
    total_runtime_prev = snap->total_runtime;
    snap->tasks = tasks_out;
    return snap;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_scheduler_snapshot_obj,
                                 mp_scheduler_snapshot);

typedef struct _heap_kind_stats_obj_t {
    mp_obj_base_t base;

    qstr kind;
    multi_heap_info_t info;
} heap_kind_stats_obj_t;

const mp_obj_type_t heap_kind_stats_type;

STATIC void heap_kind_stats_print(const mp_print_t *print, mp_obj_t self_in,
                                  mp_print_kind_t kind) {
    (void)kind;
    heap_kind_stats_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print,
              "HeapKindStats(kind=%s,total_free_bytes=%d,total_allocated_bytes="
              "%d,largest_free_block=%d)",
              qstr_str(self->kind), self->info.total_free_bytes,
              self->info.total_allocated_bytes, self->info.largest_free_block);
}

STATIC void heap_kind_stats_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    heap_kind_stats_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }
    switch (attr) {
        case MP_QSTR_kind:
            dest[0] = MP_OBJ_NEW_QSTR(self->kind);
            break;
        case MP_QSTR_total_free_bytes:
            dest[0] = mp_obj_new_int_from_uint(self->info.total_free_bytes);
            break;
        case MP_QSTR_total_allocated_bytes:
            dest[0] =
                mp_obj_new_int_from_uint(self->info.total_allocated_bytes);
            break;
        case MP_QSTR_largest_free_block:
            dest[0] = mp_obj_new_int_from_uint(self->info.largest_free_block);
            break;
    }
}

MP_DEFINE_CONST_OBJ_TYPE(heap_kind_stats_type, MP_QSTR_heap_kind_stats,
                         MP_TYPE_FLAG_NONE, print, heap_kind_stats_print, attr,
                         heap_kind_stats_attr);

STATIC mp_obj_t heap_kind_stats_from_caps(qstr kind, uint32_t caps) {
    heap_kind_stats_obj_t *stats = m_new_obj(heap_kind_stats_obj_t);
    stats->base.type = &heap_kind_stats_type;
    stats->kind = kind;

    heap_caps_get_info(&stats->info, caps);
    return MP_OBJ_FROM_PTR(stats);
}

typedef struct _heap_stats_obj_t {
    mp_obj_base_t base;

    mp_obj_t general;
    mp_obj_t dma;
} heap_stats_obj_t;

const mp_obj_type_t heap_stats_type;

STATIC void heap_stats_print(const mp_print_t *print, mp_obj_t self_in,
                             mp_print_kind_t kind) {
    (void)kind;
    mp_printf(print, "HeapStats(general=[...],dma=[...])");
}

STATIC void heap_stats_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    heap_stats_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (dest[0] != MP_OBJ_NULL) {
        return;
    }
    switch (attr) {
        case MP_QSTR_general:
            dest[0] = self->general;
            break;
        case MP_QSTR_dma:
            dest[0] = self->dma;
            break;
    }
}

MP_DEFINE_CONST_OBJ_TYPE(heap_stats_type, MP_QSTR_heap_stats, MP_TYPE_FLAG_NONE,
                         print, heap_stats_print, attr, heap_stats_attr);

STATIC mp_obj_t mp_heap_stats(void) {
    mp_obj_t general =
        heap_kind_stats_from_caps(MP_QSTR_general, MALLOC_CAP_DEFAULT);
    mp_obj_t dma = heap_kind_stats_from_caps(MP_QSTR_dma, MALLOC_CAP_DMA);

    heap_stats_obj_t *stats = m_new_obj(heap_stats_obj_t);
    stats->base.type = &heap_stats_type;
    stats->general = general;
    stats->dma = dma;

    return MP_OBJ_FROM_PTR(stats);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_heap_stats_obj, mp_heap_stats);

STATIC mp_obj_t mp_firmware_version(void) {
    return mp_obj_new_str(st3m_version, strlen(st3m_version));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_firmware_version_obj, mp_firmware_version);

STATIC mp_obj_t mp_hardware_version(void) {
    mp_obj_t str =
        mp_obj_new_str(flow3r_bsp_hw_name, strlen(flow3r_bsp_hw_name));
    return str;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_hardware_version_obj, mp_hardware_version);

STATIC mp_obj_t mp_freertos_sleep(mp_obj_t ms_in) {
    uint32_t ms = mp_obj_get_int(ms_in);
    MP_THREAD_GIL_EXIT();
    vTaskDelay(ms / portTICK_PERIOD_MS);
    MP_THREAD_GIL_ENTER();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_freertos_sleep_obj, mp_freertos_sleep);

STATIC mp_obj_t mp_usb_connected(void) {
    static int64_t last_check = 0;
    static bool value = false;
    int64_t now = esp_timer_get_time();

    if (last_check == 0) {
        last_check = now;
        value = st3m_usb_connected();
    }

    if ((now - last_check) > 10000) {
        value = st3m_usb_connected();
        last_check = now;
    }
    return mp_obj_new_bool(value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_usb_connected_obj, mp_usb_connected);

STATIC mp_obj_t mp_usb_console_active(void) {
    static int64_t last_check = 0;
    static bool value = false;
    int64_t now = esp_timer_get_time();

    if (last_check == 0) {
        last_check = now;
        value = st3m_console_active();
    }

    if ((now - last_check) > 10000) {
        value = st3m_console_active();
        last_check = now;
    }
    return mp_obj_new_bool(value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_usb_console_active_obj,
                                 mp_usb_console_active);

STATIC mp_obj_t mp_i2c_scan(void) {
    flow3r_bsp_i2c_scan_result_t scan;
    flow3r_bsp_i2c_scan(&scan);

    mp_obj_t res = mp_obj_new_list(0, NULL);
    for (int i = 0; i < 127; i++) {
        size_t ix = i / 32;
        size_t offs = i % 32;
        if (scan.res[ix] & (1 << offs)) {
            mp_obj_list_append(res, mp_obj_new_int_from_uint(i));
        }
    }
    return res;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_i2c_scan_obj, mp_i2c_scan);

STATIC mp_obj_t mp_battery_charging(void) {
    bool res = st3m_io_charger_state_get();
    return mp_obj_new_bool(!res);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_battery_charging_obj, mp_battery_charging);

STATIC const mp_rom_map_elem_t globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_scheduler_snapshot),
      MP_ROM_PTR(&mp_scheduler_snapshot_obj) },
    { MP_ROM_QSTR(MP_QSTR_heap_stats), MP_ROM_PTR(&mp_heap_stats_obj) },
    { MP_ROM_QSTR(MP_QSTR_firmware_version),
      MP_ROM_PTR(&mp_firmware_version_obj) },
    { MP_ROM_QSTR(MP_QSTR_hardware_version),
      MP_ROM_PTR(&mp_hardware_version_obj) },
    { MP_ROM_QSTR(MP_QSTR_freertos_sleep), MP_ROM_PTR(&mp_freertos_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_usb_connected), MP_ROM_PTR(&mp_usb_connected_obj) },
    { MP_ROM_QSTR(MP_QSTR_usb_console_active),
      MP_ROM_PTR(&mp_usb_console_active_obj) },
    { MP_ROM_QSTR(MP_QSTR_i2c_scan), MP_ROM_PTR(&mp_i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_battery_charging),
      MP_ROM_PTR(&mp_battery_charging_obj) },

    { MP_ROM_QSTR(MP_QSTR_RUNNING), MP_ROM_INT(eRunning) },
    { MP_ROM_QSTR(MP_QSTR_READY), MP_ROM_INT(eReady) },
    { MP_ROM_QSTR(MP_QSTR_BLOCKED), MP_ROM_INT(eBlocked) },
    { MP_ROM_QSTR(MP_QSTR_SUSPENDED), MP_ROM_INT(eSuspended) },
    { MP_ROM_QSTR(MP_QSTR_DELETED), MP_ROM_INT(eDeleted) },
    { MP_ROM_QSTR(MP_QSTR_INVALID), MP_ROM_INT(eInvalid) },
};

STATIC MP_DEFINE_CONST_DICT(globals, globals_table);

const mp_obj_module_t mp_module_sys_kernel_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&globals,
};

MP_REGISTER_MODULE(MP_QSTR_sys_kernel, mp_module_sys_kernel_user_cmodule);
