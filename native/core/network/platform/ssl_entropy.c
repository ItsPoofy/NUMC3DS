#include "ssl_entropy.h"
#include "../../seams.h"

#if defined(NUMC3DS_MCPE_TARGET)

#define MCPE_SSL_COMMAND_INITIALIZE 0x00010002u
#define MCPE_SSL_COMMAND_RANDOM 0x00110042u
#define MCPE_SSL_PROCESS_ID_DESCRIPTOR 0x00000020u
#define MCPE_SSL_RECEIVE_DESCRIPTOR(size) (((size) << 4) | 0xcu)

typedef mcpe_s32 (*McpeGetServiceHandleFn)(mcpe_u32 *session, const char *name);

static mcpe_u32 *ssl_command_buffer(void)
{
    mcpe_u8 *tls;
    __asm__ volatile("mrc p15, 0, %0, c13, c0, 3" : "=r"(tls));
    return (mcpe_u32 *)(tls + 0x80u);
}

__attribute__((naked)) static mcpe_s32 ssl_close_handle(mcpe_u32 handle)
{
    __asm__ volatile("svc 0x23\n bx lr\n");
}

__attribute__((naked)) static mcpe_s32 ssl_send_sync_request(mcpe_u32 handle)
{
    __asm__ volatile("svc 0x32\n bx lr\n");
}

static int ssl_initialize(mcpe_u32 handle)
{
    mcpe_u32 *command = ssl_command_buffer();
    command[0] = MCPE_SSL_COMMAND_INITIALIZE;
    command[1] = MCPE_SSL_PROCESS_ID_DESCRIPTOR;
    if (ssl_send_sync_request(handle) < 0) return 0;
    return (mcpe_s32)command[1] >= 0;
}

int mcpe_ssl_entropy_fill(mcpe_u8 *output, mcpe_u32 length)
{
    McpeGetServiceHandleFn get_service_handle = (McpeGetServiceHandleFn)SEAM_nn_srv_GetServiceHandle;
    mcpe_u32 handle = 0;
    mcpe_u32 *command;
    int result;
    if (!output || !length || length > 0x00ffffffu) return 0;
    if (get_service_handle(&handle, "ssl:C") < 0) return 0;
    result = ssl_initialize(handle);
    if (result) {
        command = ssl_command_buffer();
        command[0] = MCPE_SSL_COMMAND_RANDOM;
        command[1] = length;
        command[2] = MCPE_SSL_RECEIVE_DESCRIPTOR(length);
        command[3] = (mcpe_u32)output;
        result = ssl_send_sync_request(handle) >= 0 && (mcpe_s32)command[1] >= 0;
    }
    ssl_close_handle(handle);
    return result;
}

#else

int mcpe_ssl_entropy_fill(mcpe_u8 *output, mcpe_u32 length)
{
    (void)output;
    (void)length;
    return 0;
}

#endif
