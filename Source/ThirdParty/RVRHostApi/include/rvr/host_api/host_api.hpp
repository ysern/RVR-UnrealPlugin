// Copyright 2026, Iurii Sernivka.

/*
 * A C++ wrapper over rvr-host-api, header only, compiled into the caller. It
 * owns the library and the client, and speaks std::string; nothing of it
 * crosses into the library but the C interface.
 */

#pragma once

#include "rvr/host_api/rvr_host_api.h"
#include "rvr/host_api/rvr_host_api_loader.h"

#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace rvr {
namespace host_api {

struct Lobby
{
    std::string id;
    std::string name;
};

struct Status
{
    bool service_available = false;
    rvrh_signalling_state signalling = RVRH_SIGNALLING_OFFLINE;
    std::string error;
    int retry_in_ms = 0;
    std::string server;
    rvrh_server_source source = RVRH_SOURCE_NONE;
    std::string user_code;
    std::string verification_uri;
    std::vector<Lobby> lobbies;
    std::string lobby;
    bool host_session_open = false;
    bool host_session_yours = false;
    std::string application, project, scene;
    rvrh_review_state review = RVRH_REVIEW_NONE;
    std::string review_id;
    std::string review_device;
    bool start_request_pending = false;
    std::string start_request_id;
    std::string start_request_device;
    rvrh_network_state network = RVRH_NETWORK_NONE;
    std::string last_reason;

    //! The name of the Lobby present in, or empty.
    std::string LobbyName() const
    {
        for (const Lobby &l : lobbies) {
            if (l.id == lobby) {
                return l.name;
            }
        }
        return {};
    }
};

class Client
{
public:
    Client()
    {
        load_ = rvrh_load(&library_);
        if (load_ == RVRH_LOADED && library_.api.create(&client_) != RVRH_OK) {
            client_ = nullptr;
        }
    }

    ~Client()
    {
        if (client_) {
            library_.api.destroy(client_);
        }
        rvrh_unload(&library_);
    }

    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;

    //! Loaded, and a client made.
    bool Available() const { return client_ != nullptr; }
    rvrh_load_result LoadResult() const { return load_; }
    //! "RVR is not installed." and the like, for a person.
    std::string LoadMessage() const { return library_.message; }

    void SetOnChange(rvrh_change_callback callback, void *user)
    {
        if (client_) {
            library_.api.set_on_change(client_, callback, user);
        }
    }

    Status GetStatus() const
    {
        Status out;
        if (!client_) {
            return out;
        }
        rvrh_status s;
        std::memset(&s, 0, sizeof(s));
        s.size = sizeof(s);
        if (library_.api.get_status(client_, &s) != RVRH_OK) {
            return out;
        }
        out.service_available = s.service_available != 0;
        out.signalling = static_cast<rvrh_signalling_state>(s.signalling_state);
        out.error = s.error;
        out.retry_in_ms = s.retry_in_ms;
        out.server = s.server;
        out.source = static_cast<rvrh_server_source>(s.server_source);
        out.user_code = s.user_code;
        out.verification_uri = s.verification_uri;
        for (uint32_t i = 0; i < s.lobby_count && i < RVRH_MAX_LOBBIES; ++i) {
            out.lobbies.push_back({s.lobbies[i].id, s.lobbies[i].name});
        }
        out.lobby = s.lobby;
        out.host_session_open = s.host_session_open != 0;
        out.host_session_yours = s.host_session_yours != 0;
        out.application = s.description.application;
        out.project = s.description.project;
        out.scene = s.description.scene;
        out.review = static_cast<rvrh_review_state>(s.review_state);
        out.review_id = s.review_id;
        out.review_device = s.review_device;
        out.start_request_pending = s.start_request_pending != 0;
        out.start_request_id = s.start_request_id;
        out.start_request_device = s.start_request_device;
        out.network = static_cast<rvrh_network_state>(s.network_state);
        out.last_reason = s.last_reason;
        return out;
    }

    rvrh_result GoOnline(const std::string &server = {}, const std::string &profile = {}, const std::string &ca = {})
    {
        return client_ ? library_.api.go_online(client_, Opt(server), Opt(profile), Opt(ca)) : RVRH_SERVICE_UNAVAILABLE;
    }
    rvrh_result GoOffline() { return client_ ? library_.api.go_offline(client_) : RVRH_SERVICE_UNAVAILABLE; }
    rvrh_result ChooseLobby(const std::string &lobby)
    {
        return client_ ? library_.api.choose_lobby(client_, lobby.c_str()) : RVRH_SERVICE_UNAVAILABLE;
    }
    rvrh_result OpenHostSession(const std::string &application, const std::string &project, const std::string &scene)
    {
        return client_ ? library_.api.open_host_session(client_, application.c_str(), project.c_str(), scene.c_str())
                       : RVRH_SERVICE_UNAVAILABLE;
    }
    rvrh_result CloseHostSession() { return client_ ? library_.api.close_host_session(client_) : RVRH_SERVICE_UNAVAILABLE; }
    rvrh_result AnswerStart(const std::string &review_id, bool accept, const std::string &reason = {})
    {
        return client_ ? library_.api.answer_start(client_, review_id.c_str(), accept ? 1 : 0, Opt(reason)) : RVRH_SERVICE_UNAVAILABLE;
    }
    rvrh_result EndReview() { return client_ ? library_.api.end_review(client_) : RVRH_SERVICE_UNAVAILABLE; }
    bool ShouldRunVr() const { return client_ && library_.api.should_run_vr(client_) != 0; }

private:
    static const char *Opt(const std::string &s) { return s.empty() ? nullptr : s.c_str(); }

    rvrh_library library_{};
    rvrh_load_result load_ = RVRH_NOT_INSTALLED;
    rvrh_client *client_ = nullptr;
};

} // namespace host_api
} // namespace rvr
