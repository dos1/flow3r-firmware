#pragma once

// USB support for st3m, via the USB-OTG peripheral (not USB UART/JTAG!).
//
// USB is a complex beast. Attempting to make user-friendly wrappers that are
// also general-purpose is a waste of time.
//
// What we do instead is provide a limited feature set for USB. A device is in
// one of either three modes, and these modes can be switch at runtime:
//  1. Disabled,
//  2. Application, or
//  3. Disk.
//
// Disabled mode makes the device appear as if it was just a passive charging
// device. It does not appear on any host device if plugged in. This is the
// default mode until the stack fully initializes. Then, the device switches
// to application mode.
//
// Application mode is where the device spends most of its time. If plugged in,
// it will enumerate as a `flow3r` and expose a CDC-ACM (serial) endpoint which
// hosts the Micropython console. In the future more USB endpoints will appear
// here: USB MIDI, for example.
//
// Device mode can be requested by software. If the device is plugged into a
// host, it will enumerate as a USB mass storage device that can be written and
// read from.
//
// Note that this codebase does not implement any of the logic behind any
// endpoint (like CDC-ACM or MSC). Instead it just provides a callback-base
// interface that is activated by switching to a mode configuration which
// collects these callbacks. Higher level layers implement the rest of the
// stack, like USB console for Micropython or flash/SD card mounting.
//
// The current API design assumes there is only one kind of each endpoint in
// each mode, and thus things are a bit simplistic. This might change in the
// future.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    // Device should not enumerate.
    st3m_usb_mode_kind_disabled = 0,
    // Device should appear as a 'flow3er' with a CDC-ACM (serial) endpoint.
    st3m_usb_mode_kind_app = 1,
    // Device should appear as a 'flower (disk mode)' with a MSC (mass storage)
    // endpoint.
    st3m_usb_mode_kind_disk = 2,
} st3m_usb_mode_kind_t;

// Description of the device in disk mode.
typedef struct {
    // Number of blocks.
    size_t block_size;
    // Size of each block (usually 512 bytes).
    size_t block_count;
    // Product ID, padded with zeroes.
    uint8_t product_id[16];

    // Optional. Called whenever the host asks if the device is ready. Defaults
    // to always ready.
    bool (*fn_ready)(uint8_t lun);
    // Optional. Called whenever the hosts executes a scsi start/stop. Defaults
    // to 'yeah sure' stub.
    bool (*fn_start_stop)(uint8_t lun, uint8_t power_condition, bool start,
                          bool load_eject);
    // Required. Called when the host wishes to read from an LBA/offset. Address
    // = lba*block_size+offset. Must return however many bytes were actually
    // read.
    int32_t (*fn_read10)(uint8_t lun, uint32_t lba, uint32_t offset,
                         void *buffer, uint32_t bufsize);
    // Optional. Called when the host wishes to write to an LBA/offset. Defaults
    // to ignoring writes.
    int32_t (*fn_write10)(uint8_t lun, uint32_t lba, uint32_t offset,
                          const void *buffer, uint32_t bufsize);
} st3m_usb_msc_conf_t;

// Description of the device in application mode.
typedef struct {
    // Required. Called whenever the host wrote some bytes. All bytes must be
    // processed.
    void (*fn_rx)(const uint8_t *buffer, size_t bufsize);
    // Required. Called whenever the host requests bytes to read. Must return
    // how many bytes are actually available to transmit to the host.
    size_t (*fn_txpoll)(uint8_t *buffer, size_t bufsize);
    // Optional. Called whenever the host has detached from the device.
    void (*fn_detach)(void);
} st3m_usb_app_conf_t;

// Main configuration structure, passed by pointer to st3m_usb_mode_switch.
// Describes a requested configuration mode of the USB subsystem.
typedef struct {
    st3m_usb_mode_kind_t kind;

    // Only valid if kind == disk.
    st3m_usb_msc_conf_t *disk;
    // Only valid if kind == app.
    st3m_usb_app_conf_t *app;
} st3m_usb_mode_t;

// Immediately switch to a given mode, blocking until that mode is active. A
// mode being active does not indicate that the device is connected to a host.
void st3m_usb_mode_switch(st3m_usb_mode_t *target);

// Initialize the subsystem. Must be called, bad things will happen otherwise.
void st3m_usb_init();

// Return true if the badge is connected to a host.
bool st3m_usb_connected(void);

// Private.
void st3m_usb_descriptors_switch(st3m_usb_mode_t *mode);
void st3m_usb_msc_set_conf(st3m_usb_msc_conf_t *conf);
void st3m_usb_cdc_set_conf(st3m_usb_app_conf_t *conf);
void st3m_usb_descriptors_set_serial(const char *serial);
void st3m_usb_cdc_init(void);
void st3m_usb_cdc_txpoll(void);
void st3m_usb_cdc_detached(void);

typedef enum {
    st3m_usb_interface_disk_msc = 0,
    st3m_usb_interface_disk_total,
} st3m_usb_interface_disk_t;

typedef enum {
    st3m_usb_interface_app_cdc = 0,
    st3m_usb_interface_app_cdc_data,
    st3m_usb_interface_app_total,
} st3m_usb_interface_app_t;
