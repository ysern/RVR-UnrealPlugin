/* Copyright 2026, Iurii Sernivka. */

/*
 * Finds and loads rvr-host-client, the implementation RVR installs, and fills
 * an rvrh_api table from it. Header only, C, and no import library: an
 * integration builds with no binary of RVR's, and reports "not installed" at run
 * time until RVR is.
 *
 * Where it looks, in order:
 *   1. the RVR_HOST_CLIENT_DLL environment variable, for development;
 *   2. HKEY_CURRENT_USER\Software\RVR, value HostClientPath, which RVR records
 *      every time it starts.
 *
 * Include it in one source file. Windows only for now.
 */

#ifndef RVR_HOST_API_RVR_HOST_API_LOADER_H
#define RVR_HOST_API_RVR_HOST_API_LOADER_H

#include "rvr/host_api/rvr_host_api.h"

#include <stdio.h>
#include <string.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum rvrh_load_result
{
    RVRH_LOADED = 0,
    RVRH_NOT_INSTALLED = 1,
    RVRH_LOAD_INCOMPATIBLE = 2,
} rvrh_load_result;

typedef struct rvrh_library
{
    rvrh_api api;
    HMODULE module;
    /* What happened, in words for a person. */
    char message[512];
} rvrh_library;

static int
rvrh__path_from_environment(wchar_t *out, DWORD capacity)
{
    DWORD n = GetEnvironmentVariableW(L"RVR_HOST_CLIENT_DLL", out, capacity);
    return n > 0 && n < capacity;
}

static int
rvrh__path_from_registry(wchar_t *out, DWORD capacity)
{
    DWORD bytes = capacity * sizeof(wchar_t);
    LSTATUS status = RegGetValueW(HKEY_CURRENT_USER, L"Software\\RVR", L"HostClientPath", RRF_RT_REG_SZ, NULL, out, &bytes);
    return status == ERROR_SUCCESS && out[0] != 0;
}

/* Loads the library into `lib`. On anything but RVRH_LOADED, nothing of it is called and `lib->module` is null. */
static rvrh_load_result
rvrh_load(rvrh_library *lib)
{
    wchar_t path[MAX_PATH * 2];
    rvrh_get_api_fn get_api;
    rvrh_result result;

    memset(lib, 0, sizeof(*lib));
    if (!rvrh__path_from_environment(path, MAX_PATH * 2) && !rvrh__path_from_registry(path, MAX_PATH * 2)) {
        snprintf(lib->message, sizeof(lib->message), "RVR is not installed.");
        return RVRH_NOT_INSTALLED;
    }
    /* Its own directory is searched for what it depends on: a plain LoadLibraryW with a full path does not
     * look there, and a Host Application's process has no reason to have RVR's directory on its PATH. */
    lib->module = LoadLibraryExW(path, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (lib->module == NULL) {
        snprintf(lib->message, sizeof(lib->message), "RVR is not installed (its library could not be loaded, error %lu).",
                 (unsigned long)GetLastError());
        return RVRH_NOT_INSTALLED;
    }
    get_api = (rvrh_get_api_fn)(void *)GetProcAddress(lib->module, RVRH_GET_API_SYMBOL);
    if (get_api == NULL) {
        FreeLibrary(lib->module);
        lib->module = NULL;
        snprintf(lib->message, sizeof(lib->message), "RVR is not installed (its library is not the one expected).");
        return RVRH_NOT_INSTALLED;
    }
    lib->api.size = sizeof(lib->api);
    result = get_api(RVRH_ABI_VERSION, &lib->api);
    if (result != RVRH_OK) {
        FreeLibrary(lib->module);
        lib->module = NULL;
        memset(&lib->api, 0, sizeof(lib->api));
        snprintf(lib->message, sizeof(lib->message),
                 "The installed RVR does not support this integration (interface version %u). Update RVR or the integration.",
                 (unsigned)RVRH_ABI_VERSION);
        return RVRH_LOAD_INCOMPATIBLE;
    }
    snprintf(lib->message, sizeof(lib->message), "RVR is installed.");
    return RVRH_LOADED;
}

static void
rvrh_unload(rvrh_library *lib)
{
    if (lib->module != NULL) {
        FreeLibrary(lib->module);
    }
    memset(lib, 0, sizeof(*lib));
}

#ifdef __cplusplus
}
#endif

#endif
