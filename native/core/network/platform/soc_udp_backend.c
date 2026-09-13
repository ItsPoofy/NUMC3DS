#include "soc_udp_backend.h"
#include "../../../diagnostics/network_debug.h"
#include "../../seams.h"

#define MCPE_SOC_AF_INET 2u
#define MCPE_SOC_SOCK_DGRAM 2u
#define MCPE_SOC_F_SETFL 4u
#define MCPE_SOC_O_NONBLOCK 4u
#define MCPE_SOC_POLLIN 1u
#define MCPE_SOC_MEMOP_FREE 1u
#define MCPE_SOC_MEMPERM_DONTCARE 0x10000000u
#define MCPE_SOC_MEMOP_ALLOC 3u
#define MCPE_SOC_MEMPERM_READWRITE 3u
#define MCPE_SOC_IPC_CUR_PROCESS_ID 0x20u
#define MCPE_SOC_IPC_BUFFER_R 2u
#define MCPE_SOC_IPC_BUFFER_W 4u
#define MCPE_SOC_SYNC_REQUEST 0x32
#define MCPE_SOC_CREATE_MEMORY_BLOCK 0x1e
#define MCPE_SOC_CLOSE_HANDLE 0x23
#define MCPE_SOC_SERVICE_INIT 0x10044u
#define MCPE_SOC_SERVICE_SHUTDOWN 0x190000u
#define MCPE_SOC_SOCKET 0x200c2u
#define MCPE_SOC_BIND 0x50084u
#define MCPE_SOC_RECV_FROM 0x70104u
#define MCPE_SOC_RECV_FROM_SMALL 0x80102u
#define MCPE_SOC_SEND_TO 0x90106u
#define MCPE_SOC_SEND_TO_SMALL 0xa0106u
#define MCPE_SOC_FCNTL 0x1300c2u
#define MCPE_SOC_POLL 0x140084u
#define MCPE_SOC_CLOSE 0xb0042u
#define MCPE_SOC_SOCKET_ADDRESS_SIZE 8u
#define MCPE_SOC_SOCKET_ADDRESS_STORAGE_SIZE 0x1cu
#define MCPE_SOC_SMALL_IO_LIMIT 0x2000u

typedef mcpe_s32 (*McpeCreateMemoryBlockFn)(mcpe_u32 *handle,
                                             mcpe_u32 address,
                                             mcpe_u32 size,
                                             mcpe_u32 my_permissions,
                                             mcpe_u32 other_permissions);
typedef mcpe_s32 (*McpeGetServiceHandleFn)(mcpe_u32 *session, const char *name);
typedef void *(*GameMemallocExFn)(mcpe_u32 size, mcpe_u32 type, mcpe_u32 alignment, mcpe_u32 flags);
typedef void (*GameFreeFn)(void *ptr);

typedef struct {
    mcpe_s32 fd;
    mcpe_u32 events;
    mcpe_u32 revents;
} McpeSocPollFd;

static mcpe_u32 *soc_command_buffer(void)
{
    mcpe_u32 *tls;
    __asm__ volatile("mrc p15, 0, %0, c13, c0, 3" : "=r"(tls));
    return (mcpe_u32 *)((mcpe_u8 *)tls + 0x80u);
}

__attribute__((naked)) static mcpe_s32 soc_close_handle(mcpe_u32 handle)
{
    __asm__ volatile("svc 0x23\n bx lr\n");
}

__attribute__((naked)) static mcpe_s32 soc_send_sync_request(mcpe_u32 handle)
{
    __asm__ volatile("svc 0x32\n bx lr\n");
}

static void soc_zero(void *destination, mcpe_u32 size)
{
    mcpe_u8 *bytes = (mcpe_u8 *)destination;
    mcpe_u32 index;
    for (index = 0; index < size; ++index) bytes[index] = 0;
}

static mcpe_u32 ipc_header(mcpe_u32 command, mcpe_u32 normal, mcpe_u32 translate)
{
    return (command << 16) | ((normal & 0x3fu) << 6) | (translate & 0x3fu);
}

static mcpe_u32 ipc_static_buffer(mcpe_u32 size, mcpe_u32 identifier)
{
    return (size << 14) | ((identifier & 0xfu) << 10) | 2u;
}

static mcpe_u32 ipc_buffer(mcpe_u32 size, mcpe_u32 rights)
{
    return (size << 4) | 8u | rights;
}

static mcpe_s32 soc_error(mcpe_s32 value)
{
    if (value == -6) return MCPE_DATAGRAM_WOULD_BLOCK;
    return MCPE_DATAGRAM_PLATFORM_ERROR;
}

static mcpe_s32 soc_command_value(const mcpe_u32 *command)
{
    mcpe_s32 value = (mcpe_s32)command[1];
    if (value == 0) value = (mcpe_s32)command[2];
    return value;
}

static int soc_command_status(const mcpe_u32 *command)
{
    mcpe_s32 value = soc_command_value(command);
    return value < 0 ? soc_error(value) : MCPE_DATAGRAM_OK;
}

static void endpoint_to_sockaddr(mcpe_u8 output[MCPE_SOC_SOCKET_ADDRESS_SIZE],
                                 const McpeDatagramEndpoint *endpoint)
{
    output[0] = MCPE_SOC_SOCKET_ADDRESS_SIZE;
    output[1] = (mcpe_u8)MCPE_SOC_AF_INET;
    output[2] = (mcpe_u8)(endpoint->port_be >> 8);
    output[3] = (mcpe_u8)endpoint->port_be;
    output[4] = (mcpe_u8)(endpoint->address_be >> 24);
    output[5] = (mcpe_u8)(endpoint->address_be >> 16);
    output[6] = (mcpe_u8)(endpoint->address_be >> 8);
    output[7] = (mcpe_u8)endpoint->address_be;
}

static int sockaddr_to_endpoint(const mcpe_u8 input[MCPE_SOC_SOCKET_ADDRESS_SIZE],
                                McpeDatagramEndpoint *endpoint)
{
    if (input[1] != MCPE_SOC_AF_INET || input[0] < MCPE_SOC_SOCKET_ADDRESS_SIZE) {
        return MCPE_DATAGRAM_PLATFORM_ERROR;
    }
    endpoint->port_be = ((mcpe_u16)input[2] << 8) | input[3];
    endpoint->address_be = ((mcpe_u32)input[4] << 24) |
                           ((mcpe_u32)input[5] << 16) |
                           ((mcpe_u32)input[6] << 8) |
                           input[7];
    endpoint->reserved = 0;
    return MCPE_DATAGRAM_OK;
}

static int soc_service_initialize(McpeSocUdpBackend *backend)
{
    mcpe_u32 *command = soc_command_buffer();
    mcpe_s32 result;
    command[0] = MCPE_SOC_SERVICE_INIT;
    command[1] = backend->context_size;
    command[2] = MCPE_SOC_IPC_CUR_PROCESS_ID;
    command[4] = 0;
    command[5] = backend->memory_handle;
    result = soc_send_sync_request(backend->service_handle);
    if (result < 0) return MCPE_DATAGRAM_PLATFORM_ERROR;
    return (mcpe_s32)command[1] == 0 ? MCPE_DATAGRAM_OK : MCPE_DATAGRAM_PLATFORM_ERROR;
}

static int soc_service_shutdown(McpeSocUdpBackend *backend)
{
    mcpe_u32 *command = soc_command_buffer();
    mcpe_s32 result;
    if (!backend->service_handle) return MCPE_DATAGRAM_OK;
    command[0] = MCPE_SOC_SERVICE_SHUTDOWN;
    result = soc_send_sync_request(backend->service_handle);
    if (result < 0 || (mcpe_s32)command[1] < 0) return MCPE_DATAGRAM_PLATFORM_ERROR;
    return MCPE_DATAGRAM_OK;
}

static int soc_open_socket(McpeSocUdpBackend *backend)
{
    mcpe_u32 *command = soc_command_buffer();
    mcpe_s32 result;
    command[0] = MCPE_SOC_SOCKET;
    command[1] = MCPE_SOC_AF_INET;
    command[2] = MCPE_SOC_SOCK_DGRAM;
    command[3] = 0;
    command[4] = MCPE_SOC_IPC_CUR_PROCESS_ID;
    result = soc_send_sync_request(backend->service_handle);
    if (result < 0 || (mcpe_s32)command[1] < 0 || (mcpe_s32)command[2] < 0) {
        return MCPE_DATAGRAM_PLATFORM_ERROR;
    }
    backend->socket_handle = (mcpe_s32)command[2];
    return MCPE_DATAGRAM_OK;
}

static int soc_set_nonblocking(McpeSocUdpBackend *backend)
{
    mcpe_u32 *command = soc_command_buffer();
    mcpe_s32 result;
    command[0] = MCPE_SOC_FCNTL;
    command[1] = (mcpe_u32)backend->socket_handle;
    command[2] = MCPE_SOC_F_SETFL;
    command[3] = MCPE_SOC_O_NONBLOCK;
    command[4] = MCPE_SOC_IPC_CUR_PROCESS_ID;
    result = soc_send_sync_request(backend->service_handle);
    if (result < 0 || (mcpe_s32)command[1] < 0 || (mcpe_s32)command[2] < 0) {
        return MCPE_DATAGRAM_PLATFORM_ERROR;
    }
    backend->nonblocking = 1;
    return MCPE_DATAGRAM_OK;
}

static int soc_set_broadcast(McpeSocUdpBackend *backend, int enable)
{
    mcpe_u32 *command = soc_command_buffer();
    mcpe_u32 value = enable ? 1u : 0u;
    mcpe_s32 result;
    if (!backend || !backend->service_handle || backend->socket_handle < 0) {
        return MCPE_DATAGRAM_PLATFORM_ERROR;
    }
    command[0] = 0x120104u;
    command[1] = (mcpe_u32)backend->socket_handle;
    command[2] = 0xffffu;
    command[3] = 0x0020u;
    command[4] = sizeof(value);
    command[5] = MCPE_SOC_IPC_CUR_PROCESS_ID;
    command[6] = 0;
    command[7] = ipc_static_buffer(sizeof(value), 9);
    command[8] = (mcpe_u32)&value;
    result = soc_send_sync_request(backend->service_handle);
    if (result < 0 || soc_command_status(command) != MCPE_DATAGRAM_OK) {
        return MCPE_DATAGRAM_PLATFORM_ERROR;
    }
    return MCPE_DATAGRAM_OK;
}

static int soc_bind(McpeSocUdpBackend *backend, mcpe_u16 port)
{
    mcpe_u32 *command = soc_command_buffer();
    mcpe_u8 address[MCPE_SOC_SOCKET_ADDRESS_STORAGE_SIZE];
    mcpe_s32 result;
    McpeDatagramEndpoint endpoint;
    endpoint.address_be = 0;
    endpoint.port_be = port;
    endpoint.reserved = 0;
    endpoint_to_sockaddr(address, &endpoint);
    command[0] = MCPE_SOC_BIND;
    command[1] = (mcpe_u32)backend->socket_handle;
    command[2] = MCPE_SOC_SOCKET_ADDRESS_SIZE;
    command[3] = MCPE_SOC_IPC_CUR_PROCESS_ID;
    command[5] = ipc_static_buffer(MCPE_SOC_SOCKET_ADDRESS_SIZE, 0);
    command[6] = (mcpe_u32)address;
    result = soc_send_sync_request(backend->service_handle);
    if (result < 0 || soc_command_status(command) != MCPE_DATAGRAM_OK) return MCPE_DATAGRAM_PLATFORM_ERROR;
    return MCPE_DATAGRAM_OK;
}

static int soc_start(void *context, mcpe_u16 port, int allow_broadcast)
{
    McpeSocUdpBackend *backend = (McpeSocUdpBackend *)context;
    GameMemallocExFn heap_alloc = (GameMemallocExFn)SEAM_Heap_alloc;
    McpeCreateMemoryBlockFn create_memory_block = (McpeCreateMemoryBlockFn)SEAM_nn_svc_CreateMemoryBlock;
    McpeGetServiceHandleFn get_service_handle = (McpeGetServiceHandleFn)SEAM_nn_srv_GetServiceHandle;
    mcpe_s32 result;
    mcpe_u32 alloc_size;
    if (!backend || backend->started || !backend->context_size) return MCPE_DATAGRAM_INVALID_ARGUMENT;
    if ((backend->context_size & (MCPE_SOC_CONTEXT_ALIGNMENT - 1u)) != 0) return MCPE_DATAGRAM_INVALID_ARGUMENT;
    net_log_open(NET_LOG_INFO, "soc", "start");
    net_log_dec("port", port);
    net_log_flag("broadcast", allow_broadcast);
    net_log_hex("context_size", backend->context_size);
    net_log_close();
    backend->broadcast_requested = (mcpe_u8)(allow_broadcast != 0);
    /* svcCreateMemoryBlock requires a page-aligned address, but the game heap
       does not guarantee the requested alignment for large blocks. Allocate
       context_size plus one alignment unit and align the block ourselves so the
       memory block is always accepted. */
    alloc_size = backend->context_size + MCPE_SOC_CONTEXT_ALIGNMENT;
    backend->context_raw = 0;
    backend->context_address = 0;
    while (alloc_size >= MCPE_SOC_CONTEXT_ALIGNMENT + 0x20000u) {
        backend->context_raw = (mcpe_u32)heap_alloc(alloc_size, 0, 4, 0);
        if (!backend->context_raw) {
            backend->context_raw = (mcpe_u32)heap_alloc(alloc_size, 1, 4, 0);
        }
        if (backend->context_raw) break;
        alloc_size >>= 1;
    }
    if (backend->context_raw) {
        backend->context_address =
            (backend->context_raw + (MCPE_SOC_CONTEXT_ALIGNMENT - 1u)) &
            ~(MCPE_SOC_CONTEXT_ALIGNMENT - 1u);
        backend->context_size = alloc_size - (backend->context_address - backend->context_raw);
        backend->context_size &= ~(MCPE_SOC_CONTEXT_ALIGNMENT - 1u);
        result = create_memory_block(&backend->memory_handle, backend->context_address,
                                     backend->context_size, 0, MCPE_SOC_MEMPERM_READWRITE);
    } else {
        result = create_memory_block(&backend->memory_handle, 0,
                                     backend->context_size, 0, MCPE_SOC_MEMPERM_READWRITE);
    }
    net_log_open(NET_LOG_INFO, "soc", "context");
    net_log_hex("address", backend->context_address);
    net_log_hex("raw", backend->context_raw);
    net_log_hex("size", backend->context_size);
    net_log_signed("memory_result", result);
    net_log_hex("memory_handle", backend->memory_handle);
    net_log_close();
    if (result < 0 || !backend->memory_handle) goto failed;
    result = get_service_handle(&backend->service_handle, "soc:U");
    net_log_open(NET_LOG_INFO, "soc", "service");
    net_log_signed("result", result);
    net_log_hex("service_handle", backend->service_handle);
    net_log_close();
    if (result < 0) goto failed;
    if (soc_service_initialize(backend) != MCPE_DATAGRAM_OK) goto failed;
    backend->service_initialized = 1;
    if (soc_open_socket(backend) != MCPE_DATAGRAM_OK) goto failed;
    if (soc_set_nonblocking(backend) != MCPE_DATAGRAM_OK) goto failed;
    if (backend->broadcast_requested) {
        (void)soc_set_broadcast(backend, 1);
    }
    if (soc_bind(backend, port) != MCPE_DATAGRAM_OK) goto failed;
    backend->started = 1;
    backend->wake_requested = 0;
    net_log_open(NET_LOG_INFO, "soc", "start_ok");
    net_log_dec("socket", (u32)backend->socket_handle);
    net_log_dec("port", port);
    net_log_close();
    return MCPE_DATAGRAM_OK;

failed:
    net_log_open(NET_LOG_INFO, "soc", "start_failed");
    net_log_dec("socket", (u32)backend->socket_handle);
    net_log_hex("memory_handle", backend->memory_handle);
    net_log_hex("service_handle", backend->service_handle);
    net_log_hex("context", backend->context_address);
    net_log_close();
    if (backend->socket_handle >= 0) {
        mcpe_u32 *command = soc_command_buffer();
        command[0] = MCPE_SOC_CLOSE;
        command[1] = (mcpe_u32)backend->socket_handle;
        command[2] = MCPE_SOC_IPC_CUR_PROCESS_ID;
        (void)soc_send_sync_request(backend->service_handle);
        backend->socket_handle = -1;
    }
    if (backend->memory_handle) {
        (void)soc_close_handle(backend->memory_handle);
        backend->memory_handle = 0;
    }
    if (backend->service_initialized) {
        (void)soc_service_shutdown(backend);
        backend->service_initialized = 0;
    }
    if (backend->service_handle) {
        (void)soc_close_handle(backend->service_handle);
        backend->service_handle = 0;
    }
    if (backend->context_raw) {
        ((GameFreeFn)SEAM_operator_delete)((void *)backend->context_raw);
        backend->context_raw = 0;
    }
    backend->context_address = 0;
    return MCPE_DATAGRAM_PLATFORM_ERROR;
}

static void soc_stop(void *context)
{
    McpeSocUdpBackend *backend = (McpeSocUdpBackend *)context;
    mcpe_u32 *command;
    if (!backend) return;
    net_log_open(NET_LOG_INFO, "soc", "stop");
    net_log_dec("socket", (u32)backend->socket_handle);
    net_log_hex("memory_handle", backend->memory_handle);
    net_log_hex("service_handle", backend->service_handle);
    net_log_hex("context", backend->context_address);
    net_log_close();
    if (backend->socket_handle >= 0 && backend->service_handle) {
        command = soc_command_buffer();
        command[0] = MCPE_SOC_CLOSE;
        command[1] = (mcpe_u32)backend->socket_handle;
        command[2] = MCPE_SOC_IPC_CUR_PROCESS_ID;
        (void)soc_send_sync_request(backend->service_handle);
    }
    backend->socket_handle = -1;
    if (backend->memory_handle) (void)soc_close_handle(backend->memory_handle);
    backend->memory_handle = 0;
    if (backend->service_initialized) (void)soc_service_shutdown(backend);
    if (backend->service_handle) (void)soc_close_handle(backend->service_handle);
    if (backend->context_raw) {
        net_log_open(NET_LOG_INFO, "soc", "free_context");
        net_log_hex("raw", backend->context_raw);
        net_log_hex("address", backend->context_address);
        net_log_close();
        ((GameFreeFn)SEAM_operator_delete)((void *)backend->context_raw);
        backend->context_raw = 0;
    }
    backend->service_handle = 0;
    backend->context_address = 0;
    backend->started = 0;
    backend->service_initialized = 0;
    backend->nonblocking = 0;
    backend->wake_requested = 0;
}

static int soc_send(void *context, const McpeDatagramEndpoint *destination,
                    const void *payload, mcpe_u32 length)
{
    McpeSocUdpBackend *backend = (McpeSocUdpBackend *)context;
    mcpe_u32 *command;
    mcpe_u8 address[MCPE_SOC_SOCKET_ADDRESS_STORAGE_SIZE];
    mcpe_s32 result;
    if (!backend || !backend->started || !destination || (length && !payload)) return MCPE_DATAGRAM_INVALID_ARGUMENT;
    if (length >= 0x01000000u) return MCPE_DATAGRAM_TOO_LARGE;
    endpoint_to_sockaddr(address, destination);
    command = soc_command_buffer();
    if (length < MCPE_SOC_SMALL_IO_LIMIT) {
        command[0] = MCPE_SOC_SEND_TO_SMALL;
        command[1] = (mcpe_u32)backend->socket_handle;
        command[2] = length;
        command[3] = 0;
        command[4] = MCPE_SOC_SOCKET_ADDRESS_SIZE;
        command[5] = MCPE_SOC_IPC_CUR_PROCESS_ID;
        command[6] = 0;
        command[7] = ipc_static_buffer(length, 2);
        command[8] = (mcpe_u32)payload;
        command[9] = ipc_static_buffer(MCPE_SOC_SOCKET_ADDRESS_SIZE, 1);
        command[10] = (mcpe_u32)address;
    } else {
        command[0] = MCPE_SOC_SEND_TO;
        command[1] = (mcpe_u32)backend->socket_handle;
        command[2] = length;
        command[3] = 0;
        command[4] = MCPE_SOC_SOCKET_ADDRESS_SIZE;
        command[5] = MCPE_SOC_IPC_CUR_PROCESS_ID;
        command[6] = 0;
        command[7] = ipc_static_buffer(MCPE_SOC_SOCKET_ADDRESS_SIZE, 1);
        command[8] = (mcpe_u32)address;
        command[9] = ipc_buffer(length, MCPE_SOC_IPC_BUFFER_R);
        command[10] = (mcpe_u32)payload;
    }
    result = soc_send_sync_request(backend->service_handle);
    net_log_open(NET_LOG_VERBOSE, "soc", "sendto");
    net_log_ipv4("dest", destination->address_be, destination->port_be);
    net_log_dec("length", length);
    net_log_signed("sync", result);
    net_log_signed("command1", (mcpe_s32)command[1]);
    net_log_signed("command2", (mcpe_s32)command[2]);
    net_log_close();
    if (result < 0) return MCPE_DATAGRAM_PLATFORM_ERROR;
    if (soc_command_status(command) != MCPE_DATAGRAM_OK) {
        result = soc_command_value(command);
        return result == -6 ? MCPE_DATAGRAM_WOULD_BLOCK : MCPE_DATAGRAM_PLATFORM_ERROR;
    }
    return MCPE_DATAGRAM_OK;
}

static int soc_receive(void *context, McpeDatagramEndpoint *source,
                       void *payload, mcpe_u32 capacity, mcpe_u32 *length)
{
    McpeSocUdpBackend *backend = (McpeSocUdpBackend *)context;
    mcpe_u32 *command;
    mcpe_u8 address[MCPE_SOC_SOCKET_ADDRESS_STORAGE_SIZE];
    mcpe_u32 *static_buffers;
    mcpe_u32 saved_static[4];
    mcpe_s32 result;
    if (length) *length = 0;
    if (!backend || !backend->started || !source || !payload || !capacity) return MCPE_DATAGRAM_INVALID_ARGUMENT;
    if (capacity >= 0x01000000u) return MCPE_DATAGRAM_TOO_LARGE;
    soc_zero(address, sizeof(address));
    command = soc_command_buffer();
    if (capacity < MCPE_SOC_SMALL_IO_LIMIT) {
        command[0] = MCPE_SOC_RECV_FROM_SMALL;
        command[1] = (mcpe_u32)backend->socket_handle;
        command[2] = capacity;
        command[3] = 0;
        command[4] = MCPE_SOC_SOCKET_ADDRESS_STORAGE_SIZE;
        command[5] = MCPE_SOC_IPC_CUR_PROCESS_ID;
        command[6] = 0;
        static_buffers = command + (0x100u / 4u);
        saved_static[0] = static_buffers[0];
        saved_static[1] = static_buffers[1];
        saved_static[2] = static_buffers[2];
        saved_static[3] = static_buffers[3];
        static_buffers[0] = ipc_static_buffer(capacity, 0);
        static_buffers[1] = (mcpe_u32)payload;
        static_buffers[2] = ipc_static_buffer(MCPE_SOC_SOCKET_ADDRESS_STORAGE_SIZE, 1);
        static_buffers[3] = (mcpe_u32)address;
    } else {
        command[0] = MCPE_SOC_RECV_FROM;
        command[1] = (mcpe_u32)backend->socket_handle;
        command[2] = capacity;
        command[3] = 0;
        command[4] = MCPE_SOC_SOCKET_ADDRESS_STORAGE_SIZE;
        command[5] = MCPE_SOC_IPC_CUR_PROCESS_ID;
        command[6] = 0;
        command[7] = ipc_buffer(capacity, MCPE_SOC_IPC_BUFFER_W);
        command[8] = (mcpe_u32)payload;
        static_buffers = command + (0x100u / 4u);
        saved_static[0] = static_buffers[0];
        saved_static[1] = static_buffers[1];
        saved_static[2] = static_buffers[2];
        saved_static[3] = static_buffers[3];
        static_buffers[0] = ipc_static_buffer(MCPE_SOC_SOCKET_ADDRESS_STORAGE_SIZE, 0);
        static_buffers[1] = (mcpe_u32)address;
        static_buffers[2] = saved_static[2];
        static_buffers[3] = saved_static[3];
    }
    result = soc_send_sync_request(backend->service_handle);
    static_buffers[0] = saved_static[0];
    static_buffers[1] = saved_static[1];
    static_buffers[2] = saved_static[2];
    static_buffers[3] = saved_static[3];
    if (result < 0) return MCPE_DATAGRAM_PLATFORM_ERROR;
    if (soc_command_status(command) != MCPE_DATAGRAM_OK) {
        result = soc_command_value(command);
        return result == -6 ? MCPE_DATAGRAM_WOULD_BLOCK : MCPE_DATAGRAM_PLATFORM_ERROR;
    }
    result = soc_command_value(command);
    if (result < 0) return MCPE_DATAGRAM_PLATFORM_ERROR;
    if (sockaddr_to_endpoint(address, source) != MCPE_DATAGRAM_OK) return MCPE_DATAGRAM_PLATFORM_ERROR;
    if (length) *length = (mcpe_u32)result;
    net_log_open(NET_LOG_VERBOSE, "soc", "recvfrom");
    net_log_dec("length", (u32)result);
    net_log_ipv4("source", source->address_be, source->port_be);
    net_log_close();
    return MCPE_DATAGRAM_OK;
}

static int soc_poll(void *context, mcpe_u32 timeout_ms)
{
    McpeSocUdpBackend *backend = (McpeSocUdpBackend *)context;
    mcpe_u32 *command;
    mcpe_u32 *static_buffers;
    mcpe_u32 saved_static[2];
    McpeSocPollFd poll_fd;
    mcpe_s32 result;
    if (!backend || !backend->started) return MCPE_DATAGRAM_NOT_STARTED;
    if (backend->wake_requested) {
        backend->wake_requested = 0;
        return MCPE_DATAGRAM_WOULD_BLOCK;
    }
    poll_fd.fd = backend->socket_handle;
    poll_fd.events = MCPE_SOC_POLLIN;
    poll_fd.revents = 0;
    command = soc_command_buffer();
    command[0] = MCPE_SOC_POLL;
    command[1] = 1;
    command[2] = timeout_ms > 0x7fffffffu ? 0x7fffffffu : timeout_ms;
    command[3] = MCPE_SOC_IPC_CUR_PROCESS_ID;
    command[4] = 0;
    command[5] = ipc_static_buffer(sizeof(poll_fd), 10);
    command[6] = (mcpe_u32)&poll_fd;
    static_buffers = command + (0x100u / 4u);
    saved_static[0] = static_buffers[0];
    saved_static[1] = static_buffers[1];
    static_buffers[0] = ipc_static_buffer(sizeof(poll_fd), 0);
    static_buffers[1] = (mcpe_u32)&poll_fd;
    result = soc_send_sync_request(backend->service_handle);
    static_buffers[0] = saved_static[0];
    static_buffers[1] = saved_static[1];
    if (result < 0) return MCPE_DATAGRAM_PLATFORM_ERROR;
    if (soc_command_status(command) != MCPE_DATAGRAM_OK) {
        result = soc_command_value(command);
        return result == -6 ? MCPE_DATAGRAM_WOULD_BLOCK : MCPE_DATAGRAM_PLATFORM_ERROR;
    }
    return poll_fd.revents ? MCPE_DATAGRAM_OK : MCPE_DATAGRAM_WOULD_BLOCK;
}

static void soc_wakeup(void *context)
{
    McpeSocUdpBackend *backend = (McpeSocUdpBackend *)context;
    if (backend) backend->wake_requested = 1;
}

static const McpeDatagramBackendOps soc_ops = {
    soc_start,
    soc_stop,
    soc_send,
    soc_receive,
    soc_poll,
    soc_wakeup
};

void mcpe_soc_udp_backend_init(McpeSocUdpBackend *backend, mcpe_u32 context_size)
{
    if (!backend) return;
    soc_zero(backend, sizeof(*backend));
    backend->context_size = context_size ? context_size : MCPE_SOC_CONTEXT_SIZE_DEFAULT;
    backend->socket_handle = -1;
}

const McpeDatagramBackendOps *mcpe_soc_udp_backend_ops(void)
{
    return &soc_ops;
}

static McpeSocUdpBackend shared_soc_backend;
static McpeDatagramBackend shared_datagram_backend;
static int shared_backend_ready;

McpeDatagramBackend *mcpe_soc_udp_shared_backend(void)
{
    if (!shared_backend_ready) {
        mcpe_soc_udp_backend_init(&shared_soc_backend, MCPE_SOC_CONTEXT_SIZE_DEFAULT);
        mcpe_datagram_backend_init(&shared_datagram_backend, mcpe_soc_udp_backend_ops(),
                                   &shared_soc_backend, MCPE_SOC_SHARED_PAYLOAD_LIMIT);
        shared_backend_ready = 1;
    }
    return &shared_datagram_backend;
}
