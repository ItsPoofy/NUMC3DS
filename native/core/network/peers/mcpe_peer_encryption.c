#include "mcpe_peer_encryption.h"
#include "../auth/mcpe_crypto.h"
#include "../../commands/native_types.h"
#include "../../rt.h"

typedef void *(*OperatorNewFn)(u32);
typedef unsigned int *(*GstdAllocateFn)(unsigned int, unsigned int, unsigned int);
typedef void (*GstdDtorFn)(void *);
typedef void (*OperatorDeleteFn)(void *);

typedef struct {
    u32 vtable;
    McpeAes256Cfb8Context send_stream;
    McpeAes256Cfb8Context recv_stream;
} McpePeerCipher;

typedef struct {
    u32 vtable;
    McpeSha256Context hash;
} McpePeerChecksumObject;

typedef struct {
    McpePeerChecksumObject *object;
    NativeGstdString secret;
    u32 checksum_size;
} McpePeerChecksumHolder;

static void cipher_cleanup(McpePeerCipher *cipher)
{
    (void)cipher;
}

static void cipher_destroy(McpePeerCipher *cipher)
{
    if (cipher) ((OperatorDeleteFn)SEAM_operator_delete)(cipher);
}

static void checksum_object_destroy(McpePeerChecksumObject *object)
{
    if (object) ((OperatorDeleteFn)SEAM_operator_delete)(object);
}

static void checksum_object_reset(McpePeerChecksumObject *object)
{
    if (object) mcpe_sha256_init(&object->hash);
}

static void checksum_object_update(McpePeerChecksumObject *object, const void *bytes, u32 length)
{
    if (object) mcpe_sha256_update(&object->hash, bytes, length);
}

static void checksum_object_final(McpePeerChecksumObject *object, void *output)
{
    mcpe_u8 digest[MCPE_CRYPTO_SHA256_BYTES];
    u32 index;
    if (!object || !output) return;
    mcpe_sha256_final(&object->hash, digest);
    for (index = 0; index < MCPE_CRYPTO_CHECKSUM_BYTES; ++index) ((mcpe_u8 *)output)[index] = digest[index];
}

static u32 checksum_object_result_size(const McpePeerChecksumObject *object)
{
    (void)object;
    return MCPE_CRYPTO_CHECKSUM_BYTES;
}

static NativeGstdString *cipher_output(NativeGstdString *output, McpePeerCipher *cipher,
                                       NativeGstdString *input, int decrypt)
{
    unsigned int *base;
    u32 length;

    if (!output) return 0;
    if (!cipher || !input || !input->handle) {
        output->handle = 0x00B3EB10u;
        return output;
    }

    length = *(u32 *)(input->handle - 4u);
    if (length == 0u) {
        output->handle = 0x00B3EB10u;
        return output;
    }

    base = ((GstdAllocateFn)SEAM_gstd_string_allocate)(0, length, length);
    if (!base) {
        output->handle = 0x00B3EB10u;
        return output;
    }

    output->handle = (u32)(base + 3);
    *(char *)(output->handle + length) = '\0';

    if ((decrypt ? mcpe_aes256_cfb8_decrypt(&cipher->recv_stream, (const void *)input->handle,
                                             (void *)output->handle, length)
                 : mcpe_aes256_cfb8_encrypt(&cipher->send_stream, (const void *)input->handle,
                                             (void *)output->handle, length)) != MCPE_CRYPTO_OK) {
        ((GstdDtorFn)SEAM_StrDtor)(output);
        output->handle = 0x00B3EB10u;
        return output;
    }

    return output;
}

static void cipher_init(McpePeerCipher *cipher, NativeGstdString *key, NativeGstdString *iv)
{
    (void)cipher;
    (void)key;
    (void)iv;
}

static NativeGstdString *cipher_encrypt(NativeGstdString *output, McpePeerCipher *cipher,
                                        NativeGstdString *input)
{
    return cipher_output(output, cipher, input, 0);
}

static NativeGstdString *cipher_decrypt(NativeGstdString *output, McpePeerCipher *cipher,
                                        NativeGstdString *input)
{
    return cipher_output(output, cipher, input, 1);
}

static const u32 cipher_vtable[] = {
    (u32)cipher_cleanup,
    (u32)cipher_destroy,
    (u32)cipher_init,
    (u32)cipher_encrypt,
    (u32)cipher_decrypt
};

static const u32 checksum_vtable[] = {
    (u32)checksum_object_destroy,
    (u32)checksum_object_destroy,
    (u32)checksum_object_reset,
    (u32)checksum_object_update,
    (u32)checksum_object_final,
    (u32)checksum_object_result_size
};

static McpePeerCipher *cipher_create(const mcpe_u8 key[32])
{
    McpePeerCipher *cipher;
    cipher = (McpePeerCipher *)((OperatorNewFn)SEAM_operator_new)(sizeof(*cipher));
    if (!cipher) return 0;
    zero(cipher, sizeof(*cipher));
    cipher->vtable = (u32)cipher_vtable;
    if (mcpe_aes256_cfb8_init(&cipher->send_stream, key, key) != MCPE_CRYPTO_OK ||
        mcpe_aes256_cfb8_init(&cipher->recv_stream, key, key) != MCPE_CRYPTO_OK) {
        cipher_destroy(cipher);
        return 0;
    }
    return cipher;
}

static McpePeerChecksumHolder *checksum_holder_create(const mcpe_u8 key[32])
{
    McpePeerChecksumHolder *holder;
    McpePeerChecksumObject *object;
    unsigned int *base;
    holder = (McpePeerChecksumHolder *)((OperatorNewFn)SEAM_operator_new)(sizeof(*holder));
    if (!holder) return 0;
    zero(holder, sizeof(*holder));
    object = (McpePeerChecksumObject *)((OperatorNewFn)SEAM_operator_new)(sizeof(*object));
    if (!object) {
        ((OperatorDeleteFn)SEAM_operator_delete)(holder);
        return 0;
    }
    zero(object, sizeof(*object));
    object->vtable = (u32)checksum_vtable;
    checksum_object_reset(object);
    base = ((GstdAllocateFn)SEAM_gstd_string_allocate)(0, 32u, 32u);
    if (!base) {
        checksum_object_destroy(object);
        ((OperatorDeleteFn)SEAM_operator_delete)(holder);
        return 0;
    }
    holder->object = object;
    holder->secret.handle = (u32)(base + 3);
    cp((void *)holder->secret.handle, key, 32u);
    *(char *)(holder->secret.handle + 32u) = '\0';
    holder->checksum_size = MCPE_CRYPTO_CHECKSUM_BYTES;
    return holder;
}

int mcpe_peer_encryption_enable(void *encrypted_peer, const mcpe_u8 key[32])
{
    McpePeerCipher *cipher;
    McpePeerChecksumHolder *holder;
    if (!encrypted_peer || !key) return 0;
    if (*(void **)((u8 *)encrypted_peer + 8u) ||
        *(void **)((u8 *)encrypted_peer + 12u)) return 0;
    cipher = cipher_create(key);
    if (!cipher) return 0;
    holder = checksum_holder_create(key);
    if (!holder) {
        cipher_destroy(cipher);
        return 0;
    }
    *(void **)((u8 *)encrypted_peer + 8u) = cipher;
    *(void **)((u8 *)encrypted_peer + 12u) = holder;
    zero((u8 *)encrypted_peer + 16u, 16u);
    return 1;
}
