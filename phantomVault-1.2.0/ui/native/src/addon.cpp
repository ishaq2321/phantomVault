#include <napi.h>
#include "vault_wrapper.hpp"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    return phantom_vault_addon::VaultWrapper::Init(env, exports);
}

NODE_API_MODULE(phantom_vault_addon, InitAll)
