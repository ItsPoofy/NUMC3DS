#include "mcpe_native_login.h"
#include "mcpe_login_builder.h"

int mcpe_native_login_build_payload(NativeGstdString *payload, const char *username)
{
    return mcpe_login_builder_build(payload, username);
}
