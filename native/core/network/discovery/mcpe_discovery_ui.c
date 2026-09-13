#include "mcpe_discovery_ui.h"
#include "mcpe_discovery_provider.h"
#include "../../../diagnostics/network_debug.h"
#include "../platform/soc_udp_backend.h"
#include "../protocol/mcpe_protocol.h"
#include "../session/mcpe_transport_selector.h"
#include "../../commands/native_types.h"
#include "../../hook_manager.h"
#include "../../internal.h"
#include "../../state.h"
#include "../../seams.h"

enum {
    MCPE_UI_RECORD_CAPACITY = 8,
    MCPE_UI_INFO_CAPACITY = 8,
    MCPE_UI_ROW_SIZE = 0x20,
    MCPE_UI_INFO_SIZE = 0x108,
    MCPE_UI_REFRESH_MS = 2000u,
    MCPE_UI_SCREEN_VECTOR_OFFSET = 0x114u,
    MCPE_UI_SCREEN_COUNT_OFFSET = 0x120u
};

/* Stock SelectServerScreen row layout, recovered from the beacon rebuild at
   0x002694F8 and the app-data writers at 0x0068D1B4/0x0068D1C4:
     +0x00 name string      (beacon app data +0x30)
     +0x04 description string (beacon app data +0x10)
     +0x08 reserved (beacon app data +0x08)
     +0x0C reserved (beacon app data +0x0C)
    +0x10 current players (beacon app data +0x50)
    +0x14 maximum players (beacon app data +0x51)
    +0x18 protocol (beacon app data +0x06); ClientInstance_startExternalNetworkWorld
          at 0x00269278 rejects rows where this != *0x0091C4C0, the game protocol
          global (which mcpe_protocol_override rewrites to 113) with
          disconnectionScreen.outdatedServer, so MCPE rows must carry 113 here
    +0x1C connection pointer */
typedef struct {
    NativeGstdString name;
    NativeGstdString description;
    u32 reserved_08;
    u32 reserved_0c;
    u32 current_players;
    u32 maximum_players;
    u32 protocol;
    void *connection;
} McpeUiServerRow;

typedef struct {
    u8 bytes[MCPE_UI_INFO_SIZE];
    u32 address_be;
    u16 port_be;
    u8 used;
    u8 reserved;
} McpeUiInfoSlot;

typedef void (*SelectServerSetupFn)(void *);
typedef void (*SelectServerRenderFn)(void *, int, int, int);
typedef void (*SelectServerAppendLabelsFn)(void *);
typedef void *(*VectorAllocateFn)(u32, u32);
typedef void (*VectorDeallocateFn)(void *, u32, int);

static NuMC3DS_Hook setup_hook;
static NuMC3DS_Hook render_hook;
static NuMC3DS_Hook append_labels_hook;
static McpeDiscoveryProvider provider;
static McpeDiscoveryRecord records[MCPE_UI_RECORD_CAPACITY];
static McpeUiInfoSlot info_slots[MCPE_UI_INFO_CAPACITY];
static void *active_screen;
static u32 last_refresh_ms;
static u32 last_snapshot;
static u8 provider_ready;

static u32 ui_now_ms(void)
{
    return ((u32 (*)(void))SEAM_RakNet_GetTimeMS)();
}

static u32 ui_hash_bytes(u32 hash, const u8 *bytes, u32 length)
{
    u32 index;
    for (index = 0; index < length; ++index) {
        hash ^= bytes[index];
        hash *= 16777619u;
    }
    return hash;
}

static u32 ui_hash_record(u32 hash, const McpeDiscoveryRecord *record)
{
    if (!record) return hash;
    hash = ui_hash_bytes(hash, (const u8 *)&record->endpoint, sizeof(record->endpoint));
    hash = ui_hash_bytes(hash, (const u8 *)&record->protocol, sizeof(record->protocol));
    hash = ui_hash_bytes(hash, (const u8 *)&record->current_players, sizeof(record->current_players));
    hash = ui_hash_bytes(hash, (const u8 *)&record->maximum_players, sizeof(record->maximum_players));
    hash = ui_hash_bytes(hash, (const u8 *)record->name, sizeof(record->name));
    hash = ui_hash_bytes(hash, (const u8 *)record->world, sizeof(record->world));
    hash = ui_hash_bytes(hash, (const u8 *)record->guid, sizeof(record->guid));
    return hash;
}

static u32 provider_snapshot(void)
{
    u32 index;
    u32 hash = 2166136261u;
    hash = ui_hash_bytes(hash, (const u8 *)&provider.count, sizeof(provider.count));
    for (index = 0; index < provider.count; ++index) {
        hash = ui_hash_record(hash, &records[index]);
    }
    return hash;
}

static void provider_prepare(void)
{
    if (provider_ready) return;
    mcpe_discovery_provider_init(&provider, mcpe_soc_udp_shared_backend(), records,
                                 MCPE_UI_RECORD_CAPACITY, 5000u);
    provider_ready = 1;
}

static void provider_stop(void)
{
    if (!provider_ready) return;
    net_log_open(NET_LOG_INFO, "ui", "provider_stop");
    net_log_dec("records", provider.count);
    net_log_close();
    mcpe_discovery_provider_stop(&provider);
    active_screen = 0;
    last_refresh_ms = 0;
    last_snapshot = 0;
}

static void address_text(u32 address_be, u16 port_be, char *output, u32 capacity,
                         int include_port)
{
    u32 length = 0;
    u32 octets[4];
    u32 index;
    if (!output || capacity == 0) return;
    octets[0] = (address_be >> 24) & 0xffu;
    octets[1] = (address_be >> 16) & 0xffu;
    octets[2] = (address_be >> 8) & 0xffu;
    octets[3] = address_be & 0xffu;
    for (index = 0; index < 4; ++index) {
        append_int(output, &length, (int)octets[index]);
        if (index != 3u) append(output, &length, ".");
    }
    if (include_port) {
        append(output, &length, ":");
        append_int(output, &length, (int)port_be);
    }
    if (length >= capacity) length = capacity - 1u;
    output[length] = 0;
}

static McpeUiInfoSlot *info_slot_for(const McpeDiscoveryRecord *record,
                                     const char *host)
{
    u32 index;
    McpeUiInfoSlot *free_slot = 0;
    u32 scratch = 0;
    if (!record || !host || !host[0]) return 0;
    for (index = 0; index < MCPE_UI_INFO_CAPACITY; ++index) {
        McpeUiInfoSlot *slot = &info_slots[index];
        if (!slot->used) {
            if (!free_slot) free_slot = slot;
            continue;
        }
        if (slot->address_be == record->endpoint.address_be &&
            slot->port_be == record->endpoint.port_be) return slot;
    }
    if (!free_slot) return 0;
    zero(free_slot, sizeof(*free_slot));
    *(u16 *)free_slot->bytes = 1u;
    *(u32 *)(free_slot->bytes + 8u) = record->endpoint.port_be;
    ((StrCtor)SEAM_StrCtor)(free_slot->bytes + 4u, host, &scratch);
    if (!*(u32 *)(free_slot->bytes + 4u)) return 0;
    free_slot->address_be = record->endpoint.address_be;
    free_slot->port_be = record->endpoint.port_be;
    free_slot->used = 1;
    return free_slot;
}

static void row_destroy(McpeUiServerRow *row)
{
    if (!row) return;
    ((StrDtor)SEAM_StrDtor)(&row->name);
    ((StrDtor)SEAM_StrDtor)(&row->description);
}

static int vector_reserve(NativeVector *vector, u32 required)
{
    u32 old_count;
    u32 old_capacity;
    u32 new_capacity;
    McpeUiServerRow *old_rows;
    McpeUiServerRow *new_rows;
    u32 index;
    VectorAllocateFn allocate = (VectorAllocateFn)SEAM_gstd_allocator_allocate;
    VectorDeallocateFn deallocate = (VectorDeallocateFn)SEAM_gstd_allocator_deallocate;
    if (!vector) return 0;
    old_count = vector->begin && vector->end ?
        (vector->end - vector->begin) / MCPE_UI_ROW_SIZE : 0;
    old_capacity = vector->begin && vector->capacity ?
        (vector->capacity - vector->begin) / MCPE_UI_ROW_SIZE : 0;
    net_log_open(NET_LOG_INFO, "ui", "reserve");
    net_log_hex("begin", vector->begin);
    net_log_hex("end", vector->end);
    net_log_hex("capacity", vector->capacity);
    net_log_dec("old_count", old_count);
    net_log_dec("old_capacity", old_capacity);
    net_log_dec("required", required);
    net_log_close();
    if (required <= old_capacity) return 1;
    new_capacity = old_capacity ? old_capacity : 1u;
    while (new_capacity < required) {
        if (new_capacity > 0x100u) return 0;
        new_capacity = new_capacity + (new_capacity >> 1) + 1u;
    }
    new_rows = (McpeUiServerRow *)allocate(new_capacity * MCPE_UI_ROW_SIZE, 0);
    if (!new_rows) return 0;
    zero(new_rows, new_capacity * MCPE_UI_ROW_SIZE);
    old_rows = (McpeUiServerRow *)vector->begin;
    for (index = 0; index < old_count; ++index) {
        ((void *(*)(void *, void *))SEAM_gstd_string_copyCtor)(
            &new_rows[index].name, &old_rows[index].name);
        ((void *(*)(void *, void *))SEAM_gstd_string_copyCtor)(
            &new_rows[index].description, &old_rows[index].description);
        cp((u8 *)&new_rows[index] + 8u, (u8 *)&old_rows[index] + 8u, 0x18u);
    }
    if (old_rows) {
        for (index = 0; index < old_count; ++index) row_destroy(&old_rows[index]);
        deallocate(old_rows, old_capacity, 0);
    }
    vector->begin = (u32)new_rows;
    vector->end = (u32)(new_rows + old_count);
    vector->capacity = (u32)(new_rows + new_capacity);
    return 1;
}

static int vector_append(void *screen, const char *name, const char *description,
                         u32 current_players, u32 maximum_players, void *connection)
{
    NativeVector *vector;
    McpeUiServerRow *row;
    u32 count;
    u32 scratch = 0;
    if (!screen || !name || !description || !connection) return 0;
    vector = (NativeVector *)((u8 *)screen + MCPE_UI_SCREEN_VECTOR_OFFSET);
    count = vector->begin && vector->end ?
        (vector->end - vector->begin) / MCPE_UI_ROW_SIZE : 0;
    if (!vector_reserve(vector, count + 1u)) return 0;
    row = &((McpeUiServerRow *)vector->begin)[count];
    zero(row, sizeof(*row));
    ((StrCtor)SEAM_StrCtor)(&row->name, name, &scratch);
    if (!row->name.handle) return 0;
    scratch = 0;
    ((StrCtor)SEAM_StrCtor)(&row->description, description, &scratch);
    if (!row->description.handle) {
        ((StrDtor)SEAM_StrDtor)(&row->name);
        return 0;
    }
    row->current_players = current_players;
    row->maximum_players = maximum_players;
    row->reserved_08 = 0;
    row->reserved_0c = 0;
    row->protocol = MCPE_PROTOCOL_VERSION;
    row->connection = connection;
    vector->end = (u32)(row + 1);
    return 1;
}

static int vector_contains_connection(const NativeVector *vector, void *connection)
{
    McpeUiServerRow *row;
    McpeUiServerRow *end;
    if (!vector || !connection || !vector->begin || !vector->end) return 0;
    row = (McpeUiServerRow *)vector->begin;
    end = (McpeUiServerRow *)vector->end;
    while (row != end) {
        if (row->connection == connection) return 1;
        ++row;
    }
    return 0;
}

static void append_discovered_rows(void *screen)
{
    u32 index;
    NativeVector *vector;
    if (!screen || !provider_ready) return;
    if (!active_screen) active_screen = screen;
    net_log_open(NET_LOG_VERBOSE, "ui", "append_rows");
    net_log_hex("screen", (u32)screen);
    net_log_dec("records", provider.count);
    net_log_close();
    for (index = 0; index < provider.count; ++index) {
        McpeDiscoveryRecord *record = &records[index];
        McpeUiInfoSlot *slot;
        char host[32];
        char endpoint_text[32];
        char title[192];
        char subtitle[192];
        const char *display_name = record->name[0] ? record->name : "MCPE World";
        address_text(record->endpoint.address_be, record->endpoint.port_be,
                     host, sizeof(host), 0);
        address_text(record->endpoint.address_be, record->endpoint.port_be,
                     endpoint_text, sizeof(endpoint_text), 1);
        slot = info_slot_for(record, host);
        if (!slot) continue;
        /* The 3DS row's +0x00 is the beacon secondary field (world) and +0x04 is
           the primary field (name/MOTD). Show the world name as the title and the
           MCPE MOTD below it, matching the local-wireless list semantics. */
        copy_text(title, record->world[0] ? record->world : display_name, sizeof(title));
        copy_text(subtitle, record->name[0] ? record->name : endpoint_text, sizeof(subtitle));
        net_log_open(NET_LOG_VERBOSE, "ui", "row");
        net_log_text("title", title);
        net_log_text("subtitle", subtitle);
        net_log_dec("players", record->current_players);
        net_log_dec("max", record->maximum_players);
        net_log_close();
        vector = (NativeVector *)((u8 *)screen + MCPE_UI_SCREEN_VECTOR_OFFSET);
        if (!vector_contains_connection(vector, slot->bytes)) {
            vector_append(screen, title, subtitle,
                          record->current_players, record->maximum_players,
                          slot->bytes);
        }
    }
    vector = (NativeVector *)((u8 *)screen + MCPE_UI_SCREEN_VECTOR_OFFSET);
    if (vector->begin && vector->end) {
        *(u32 *)((u8 *)screen + MCPE_UI_SCREEN_COUNT_OFFSET) =
            (u32)(((u8 *)vector->end - (u8 *)vector->begin) / MCPE_UI_ROW_SIZE);
    }
}

static void provider_start(void *screen)
{
    u32 now;
    if (!screen) return;
    provider_prepare();
    if (active_screen && active_screen != screen) provider_stop();
    active_screen = screen;
    now = ui_now_ms();
    (void)mcpe_discovery_provider_refresh(&provider, now,
                                          ((mcpe_u64)now << 32) | now);
    last_refresh_ms = now;
    last_snapshot = provider_snapshot();
    mcpe_transport_selector_set_remote_available(provider.count > 0);
    if (provider.count > 0) {
        mcpe_transport_selector_set_remote_endpoint(records[0].endpoint.address_be,
                                                    records[0].endpoint.port_be);
    }
    net_log_open(NET_LOG_INFO, "ui", "provider_start");
    net_log_hex("screen", (u32)screen);
    net_log_dec("records", provider.count);
    net_log_dec("now", now);
    net_log_close();
}

static void provider_tick(void *screen)
{
    u32 now;
    u32 snapshot;
    if (!screen) return;
    if (active_screen != screen || !provider.started) {
        provider_start(screen);
    }
    if (!provider_ready || !provider.started) return;
    now = ui_now_ms();
    if ((u32)(now - last_refresh_ms) >= MCPE_UI_REFRESH_MS) {
        (void)mcpe_discovery_provider_refresh(&provider, now,
                                              ((mcpe_u64)now << 32) | now);
        last_refresh_ms = now;
    }
    (void)mcpe_discovery_provider_tick(&provider, now, 0);
    mcpe_transport_selector_set_remote_available(provider.count > 0);
    if (provider.count > 0) {
        mcpe_transport_selector_set_remote_endpoint(records[0].endpoint.address_be,
                                                    records[0].endpoint.port_be);
    }
    snapshot = provider_snapshot();
    if (snapshot != last_snapshot) {
        last_snapshot = snapshot;
        net_log_open(NET_LOG_INFO, "ui", "rebuild");
        net_log_dec("records", provider.count);
        net_log_dec("now", now);
        net_log_close();
        ((void (*)(void *))SEAM_SelectServerScreen_rebuild)(screen);
    }
}

static void on_select_server_setup(void *screen)
{
    net_log_open(NET_LOG_INFO, "ui", "setup");
    net_log_hex("screen", (u32)screen);
    net_log_close();
    ((SelectServerSetupFn)setup_hook.trampoline)(screen);
    provider_start(screen);
}

static void on_select_server_render(void *screen, int touch_x, int touch_y, int use_screen)
{
    ((SelectServerRenderFn)render_hook.trampoline)(screen, touch_x, touch_y, use_screen);
    provider_tick(screen);
}

static void on_select_server_append_labels(void *screen)
{
    append_discovered_rows(screen);
    ((SelectServerAppendLabelsFn)append_labels_hook.trampoline)(screen);
}

int mcpe_discovery_ui_install(void)
{
    provider_prepare();

    zero(&setup_hook, sizeof(setup_hook));
    setup_hook.target = SEAM_SelectServerScreen_setupPositions;
    setup_hook.replacement = (u32)on_select_server_setup;
    setup_hook.expected[0] = 0xE92D4FF0u;
    setup_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&setup_hook)) return -81;

    zero(&render_hook, sizeof(render_hook));
    render_hook.target = SEAM_SelectServerScreen_render;
    render_hook.replacement = (u32)on_select_server_render;
    render_hook.expected[0] = 0xE92D40F0u;
    render_hook.expected[1] = 0xE3130040u;
    if (s->host.install_hook(&render_hook)) return -82;

    zero(&append_labels_hook, sizeof(append_labels_hook));
    append_labels_hook.target = SEAM_SelectServerScreen_appendLabels;
    append_labels_hook.replacement = (u32)on_select_server_append_labels;
    append_labels_hook.expected[0] = 0xE92D4FF0u;
    append_labels_hook.expected[1] = 0xE24DD044u;
    if (s->host.install_hook(&append_labels_hook)) return -83;

    /* The SelectServerScreen destructor is deliberately NOT hooked. Its first
       instruction is PC-relative (ldr r12,[pc,#0x78]); running it through a
       relocated trampoline proved unsafe here, and the discovery SOC backend is
       process-lifetime now, so no teardown work is required. The provider is
       refreshed/reused the next time the screen renders. */
    return 0;
}
