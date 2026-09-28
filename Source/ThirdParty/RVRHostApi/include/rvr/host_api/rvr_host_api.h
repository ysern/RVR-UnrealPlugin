/* Copyright 2026, Iurii Sernivka. */

/*
 * rvr-host-api: what a Host Application's integration uses to work with RVR.
 *
 * A plain C interface: C functions and structs, nothing else, so an integration
 * built with any compiler and runtime library can use it. The implementation,
 * rvr-host-client, is installed with RVR and loaded at run time through the
 * loader in rvr_host_api_loader.h; an integration needs no binary of RVR's to
 * build.
 *
 * Rules that keep the two sides apart:
 *
 *  - Every function is reached through the rvrh_api table, never as an export
 *    of its own. The table and every struct start with a size, so a newer
 *    library can serve an older caller and the reverse.
 *  - No memory crosses the boundary: the caller owns what it passes in, and
 *    strings come back copied into fixed-size UTF-8 buffers.
 *  - Nothing throws. Every call returns an rvrh_result.
 *  - The change callback runs on the library's own thread. It should do nothing
 *    but note that the status changed; read the status on your own thread.
 */

#ifndef RVR_HOST_API_RVR_HOST_API_H
#define RVR_HOST_API_RVR_HOST_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The interface version this header describes. */
#define RVRH_ABI_VERSION 1u

/* The one symbol rvr-host-client exports. */
#define RVRH_GET_API_SYMBOL "rvrh_get_api"

typedef enum rvrh_result
{
    RVRH_OK = 0,
    /* The library does not serve the interface version asked for. */
    RVRH_INCOMPATIBLE = 1,
    RVRH_INVALID_ARGUMENT = 2,
    /* RVR is not running; the command is kept and sent when it is. */
    RVRH_SERVICE_UNAVAILABLE = 3,
    RVRH_FAILED = 4,
} rvrh_result;

typedef enum rvrh_signalling_state
{
    RVRH_SIGNALLING_OFFLINE = 0,
    RVRH_SIGNALLING_CONNECTING = 1,
    RVRH_SIGNALLING_ENROLLING = 2,
    RVRH_SIGNALLING_ONLINE = 3,
    RVRH_SIGNALLING_PRESENT = 4,
    RVRH_SIGNALLING_REFUSED = 5,
} rvrh_signalling_state;

typedef enum rvrh_server_source
{
    RVRH_SOURCE_NONE = 0,
    RVRH_SOURCE_HOST_APPLICATION = 1,
    RVRH_SOURCE_ENVIRONMENT = 2,
    RVRH_SOURCE_CONFIG_FILE = 3,
    RVRH_SOURCE_BUILT_IN = 4,
} rvrh_server_source;

typedef enum rvrh_review_state
{
    RVRH_REVIEW_NONE = 0,
    /* A Reviewer asked to start; the answer is pending. */
    RVRH_REVIEW_REQUESTED = 1,
    RVRH_REVIEW_ACCEPTED = 2,
} rvrh_review_state;

typedef enum rvrh_network_state
{
    RVRH_NETWORK_NONE = 0,
    /* A Device is connected, and no application is sending it frames. */
    RVRH_NETWORK_IDLE = 1,
    RVRH_NETWORK_STREAMING = 2,
} rvrh_network_state;

#define RVRH_ID_SIZE 72
#define RVRH_NAME_SIZE 520
#define RVRH_TEXT_SIZE 512
#define RVRH_MAX_LOBBIES 32

typedef struct rvrh_lobby
{
    char id[RVRH_ID_SIZE];
    char name[RVRH_NAME_SIZE];
} rvrh_lobby;

typedef struct rvrh_description
{
    char application[RVRH_NAME_SIZE];
    char project[RVRH_NAME_SIZE];
    char scene[RVRH_NAME_SIZE];
} rvrh_description;

/*
 * Everything the integration shows, in one snapshot. Set `size` to
 * sizeof(rvrh_status) before asking for it.
 */
typedef struct rvrh_status
{
    uint32_t size;

    /* Whether RVR is running on this machine. When it is not, nothing else here means anything. */
    int32_t service_available;

    int32_t signalling_state;      /* rvrh_signalling_state */
    char error[RVRH_TEXT_SIZE];    /* why refused, or the last connection error */
    int32_t retry_in_ms;           /* while connecting: until the next attempt */
    char server[RVRH_TEXT_SIZE];   /* the Signalling Server in use */
    int32_t server_source;         /* rvrh_server_source */
    char user_code[RVRH_ID_SIZE];  /* while enrolling: the code to approve */
    char verification_uri[RVRH_TEXT_SIZE];

    uint32_t lobby_count;
    rvrh_lobby lobbies[RVRH_MAX_LOBBIES];
    char lobby[RVRH_ID_SIZE];      /* the Lobby present in, or empty */

    int32_t host_session_open;
    int32_t host_session_yours;    /* this integration holds the Host Session */
    rvrh_description description;

    int32_t review_state;          /* rvrh_review_state */
    char review_id[RVRH_ID_SIZE];
    char review_device[RVRH_NAME_SIZE];

    /* A start request waiting for this integration's answer: call answer_start. */
    int32_t start_request_pending;
    char start_request_id[RVRH_ID_SIZE];
    char start_request_device[RVRH_NAME_SIZE];

    int32_t network_state;         /* rvrh_network_state */
    char last_reason[RVRH_TEXT_SIZE];
} rvrh_status;

typedef struct rvrh_client rvrh_client;

typedef void (*rvrh_change_callback)(void *user);

/* Every function of the interface. Set `size` to sizeof(rvrh_api) before asking for it. */
typedef struct rvrh_api
{
    uint32_t size;
    uint32_t abi_version;

    rvrh_result (*create)(rvrh_client **out_client);
    void (*destroy)(rvrh_client *client);

    /* Called on the library's thread whenever the status changes. Null to stop. */
    rvrh_result (*set_on_change)(rvrh_client *client, rvrh_change_callback callback, void *user);
    rvrh_result (*get_status)(rvrh_client *client, rvrh_status *out_status);

    /* Each of server, profile (a Connection Profile file) and ca (a certificate
     * authority file) may be null, to use RVR's own configuration. */
    rvrh_result (*go_online)(rvrh_client *client, const char *server, const char *profile, const char *ca);
    rvrh_result (*go_offline)(rvrh_client *client);

    /* A Lobby by its name or its identifier, as reported. Kept until it can be resolved. */
    rvrh_result (*choose_lobby)(rvrh_client *client, const char *lobby);

    rvrh_result (*open_host_session)(rvrh_client *client, const char *application, const char *project, const char *scene);
    rvrh_result (*close_host_session)(rvrh_client *client);

    /* reason: null, "host_not_ready" or "busy". */
    rvrh_result (*answer_start)(rvrh_client *client, const char *review_id, int32_t accept, const char *reason);
    rvrh_result (*end_review)(rvrh_client *client);

    /*
     * Whether the Host Application should be in its VR session now. Start it
     * when this turns true and stop it when it turns false; the library decides,
     * so every engine behaves the same.
     */
    int32_t (*should_run_vr)(rvrh_client *client);
} rvrh_api;

/* The signature of rvrh_get_api, which fills `out` for `abi_version` or returns RVRH_INCOMPATIBLE. */
typedef rvrh_result (*rvrh_get_api_fn)(uint32_t abi_version, rvrh_api *out);

#ifdef __cplusplus
}
#endif

#endif
