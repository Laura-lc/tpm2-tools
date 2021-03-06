/* SPDX-License-Identifier: BSD-3-Clause */
#include <inttypes.h>

#include "log.h"
#include "tpm2.h"
#include "tpm2_tool.h"
#include "tpm2_util.h"

typedef struct tpm2_setclock_ctx tpm2_setclock_ctx;
struct tpm2_setclock_ctx {
    bool time_set;
    UINT64 new_time;
    const char *auth_hierarchy;
    tpm2_loaded_object object;
    const char *auth_value;
};

static tpm2_setclock_ctx ctx = {
        .auth_hierarchy = "o" /* default to owner hierarchy */
};

static bool on_arg(int argc, char **argv) {

    if (argc != 1) {
        LOG_ERR("Can only specify 1 time to set, got: %d", argc);
        return false;
    }

    bool result = tpm2_util_string_to_uint64(argv[0], &ctx.new_time);
    if (!result) {
        LOG_ERR("Could not convert argument to time, got: \"%s\"", argv[0]);
        return false;
    }

    ctx.time_set = true;

    return true;
}

static bool on_option(char key, char *value) {

    switch (key) {
    case 'c':
        ctx.auth_hierarchy = value;
        break;
    case 'p':
        ctx.auth_value = value;
        break;
    /* no default */
    }

    return true;
}

bool tpm2_tool_onstart(tpm2_options **opts) {

    const struct option topts[] = {
        { "hierarchy", required_argument, NULL, 'c' },
        { "auth",      required_argument, NULL, 'p' }
    };

    *opts = tpm2_options_new("c:p:", ARRAY_LEN(topts), topts, on_option,
    on_arg, 0);

    return *opts != NULL;
}

tool_rc tpm2_tool_onrun(ESYS_CONTEXT *ectx, tpm2_option_flags flags) {

    UNUSED(flags);

    if (!ctx.time_set) {
        LOG_ERR("Expected single argument of time to set");
        return tool_rc_general_error;
    }

    tool_rc rc = tpm2_util_object_load_auth(ectx, ctx.auth_hierarchy,
            ctx.auth_value, &ctx.object, false,
            TPM2_HANDLE_FLAGS_P|TPM2_HANDLE_FLAGS_O);
    if (rc != tool_rc_success) {
        return rc;
    }

    return tpm2_setclock(ectx, &ctx.object, ctx.new_time);
}
