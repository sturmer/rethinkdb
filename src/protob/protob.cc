// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "protob/protob.hpp"

#include <google/protobuf/stubs/common.h>

#include <set>
#include <string>
#include <limits>

#include "errors.hpp"
#include <boost/lexical_cast.hpp>

#include "arch/arch.hpp"
#include "arch/io/network.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/queue/limited_fifo.hpp"
#include "containers/auth_key.hpp"
#include "perfmon/perfmon.hpp"
#include "protob/json_shim.hpp"
#include "rdb_protocol/env.hpp"
#include "rpc/semilattice/view.hpp"
#include "utils.hpp"

#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/query_server.hpp"

struct protob_server_exc_t : public std::exception {
public:
    explicit protob_server_exc_t(const std::string& data) : info(data) { }
    ~protob_server_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

const uint32_t MAX_QUERY_SIZE = 64 * MEGABYTE;
const size_t MAX_RESPONSE_SIZE = std::numeric_limits<uint32_t>::max();

class json_protocol_t {
public:
    static bool parse_query(tcp_conn_t *conn,
                            signal_t *interruptor,
                            query_handler_t *handler,
                            ql::protob_t<Query> *query_out) {
        int64_t token;
        uint32_t size;
        conn->read(&token, sizeof(token), interruptor);
        conn->read(&size, sizeof(size), interruptor);

        if (size > MAX_QUERY_SIZE) {
            Response error_response;
            handler->unparseable_query(token, &error_response,
                                       strprintf("Payload size (%" PRIu32 ") greater than maximum (%" PRIu32 ").",
                                                 size, MAX_QUERY_SIZE));
            send_response(error_response, handler, conn, interruptor);
            throw tcp_conn_read_closed_exc_t();
        } else {
            scoped_array_t<char> data(size + 1);
            conn->read(data.data(), size, interruptor);
            data[size] = 0; // Null terminate the string, which the json parser requires

            if (!json_shim::parse_json_pb(query_out->get(), token, data.data())) {
                Response error_response;
                handler->unparseable_query(
                    token, &error_response,
                    "Client is buggy (failed to deserialize query).");
                send_response(error_response, handler, conn, interruptor);
                return false;
            }
        }
        return true;
    }

    static void send_response(const Response &response,
                              query_handler_t *handler,
                              tcp_conn_t *conn,
                              signal_t *interruptor) {
        const int64_t token = response.token();

        uint32_t data_size_32; // filled in below
        const size_t prefix_size = sizeof(token) + sizeof(data_size_32);
        // Reserve space for the token and the size
        std::string str(prefix_size, '\0');

        json_shim::write_json_pb(response, &str);
        guarantee(str.size() >= prefix_size);
        const size_t data_size = str.size() - prefix_size;

        if (data_size > MAX_RESPONSE_SIZE) {
            Response error_response;
            handler->unparseable_query(token, &error_response,
                strprintf("Response size (%zu) is greater than maximum (%zu).",
                          data_size, MAX_RESPONSE_SIZE));
            send_response(error_response, handler, conn, interruptor);
            return;
        }

        // Fill in the prefix.
        // std::string::operator[] has unspecified complexity, but in practice
        // it should be fine.
        for (size_t i = 0; i < sizeof(token); ++i) {
            str[i] = reinterpret_cast<const char *>(&token)[i];
        }
        data_size_32 = static_cast<uint32_t>(data_size);
        for (size_t i = 0; i < sizeof(data_size_32); ++i) {
            str[i + sizeof(token)] = reinterpret_cast<const char *>(&data_size_32)[i];
        }

        conn->write(str.data(), str.size(), interruptor);
    }
};

class protobuf_protocol_t {
public:
    static bool parse_query(tcp_conn_t *conn,
                            signal_t *interruptor,
                            query_handler_t *handler,
                            ql::protob_t<Query> *query_out) {
        uint32_t size;
        conn->read(&size, sizeof(size), interruptor);

        if (size > MAX_QUERY_SIZE) {
            Response error_response;
            handler->unparseable_query(
                0,
                &error_response,
                strprintf("Payload size (%" PRIu32 ") greater than maximum (%" PRIu32 ").",
                          size, MAX_QUERY_SIZE));
            send_response(error_response, handler, conn, interruptor);
            return false;
        } else {
            scoped_array_t<char> data(size);
            conn->read(data.data(), size, interruptor);

            if (!query_out->get()->ParseFromArray(data.data(), size)) {
                Response error_response;
                int64_t token = query_out->get()->has_token() ? query_out->get()->token() : 0;
                handler->unparseable_query(token, &error_response,
                                           "Client is buggy (failed to deserialize query).");
                send_response(error_response, handler, conn, interruptor);
                return false;
            }
        }
        return true;
    }

    static void send_response(const Response &response,
                              query_handler_t *handler,
                              tcp_conn_t *conn,
                              signal_t *interruptor) {
        const size_t data_size = response.ByteSize();
        if (data_size > MAX_RESPONSE_SIZE) {
            Response error_response;
            handler->unparseable_query(
                response.token(),
                &error_response,
                strprintf("Response size (%zu) is greater than maximum (%zu).",
                          data_size, MAX_RESPONSE_SIZE));
            send_response(error_response, handler, conn, interruptor);
            return;
        }
        const uint32_t data_size_32 = static_cast<uint32_t>(data_size);
        const size_t prefix_size = sizeof(data_size_32);
        const size_t total_size = prefix_size + data_size;

        scoped_array_t<char> scoped_array(total_size);
        memcpy(scoped_array.data(), &data_size_32, sizeof(data_size_32));
        response.SerializeToArray(scoped_array.data() + prefix_size, data_size_32);

        conn->write(scoped_array.data(), total_size, interruptor);
    }
};

query_server_t::query_server_t(
    rdb_context_t *_rdb_ctx,
    const std::set<ip_address_t> &local_addresses,
    int port,
    query_handler_t *_handler,
    boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t> >
        _auth_metadata)
    : rdb_ctx(_rdb_ctx),
      handler(_handler),
      auth_metadata(_auth_metadata),
      shutting_down_conds(),
      pulse_sdc_on_shutdown(&main_shutting_down_cond),
      next_thread(0) {
    rassert(rdb_ctx != NULL);
    for (int i = 0; i < get_num_threads(); ++i) {
        shutting_down_conds.push_back(
                make_scoped<cross_thread_signal_t>(&main_shutting_down_cond,
                                                   threadnum_t(i)));
    }

    try {
        tcp_listener.init(new tcp_listener_t(local_addresses, port,
            std::bind(&query_server_t::handle_conn,
                      this, ph::_1, auto_drainer_t::lock_t(&auto_drainer))));
    } catch (const address_in_use_exc_t &ex) {
        throw address_in_use_exc_t(
            strprintf("Could not bind to RDB protocol port: %s", ex.what()));
    }
}

query_server_t::~query_server_t() { }

int query_server_t::get_port() const {
    return tcp_listener->get_port();
}

std::string query_server_t::read_sized_string(tcp_conn_t *conn,
                                              size_t max_size,
                                              const std::string &length_error_msg,
                                              signal_t *interruptor) {
    uint32_t str_length;
    conn->read(&str_length, sizeof(uint32_t), interruptor);

    if (str_length > max_size) {
        throw protob_server_exc_t(length_error_msg);
    }

    scoped_array_t<char> buffer(max_size);
    conn->read(buffer.data(), str_length, interruptor);

    return std::string(buffer.data(), str_length);
}

auth_key_t query_server_t::read_auth_key(tcp_conn_t *conn,
                                         signal_t *interruptor) {
    const std::string length_error_msg("Client provided an authorization key that is too long.");
    std::string auth_key = read_sized_string(conn, auth_key_t::max_length,
                                             length_error_msg, interruptor);
    auth_key_t ret;
    if (!ret.assign_value(auth_key)) {
        // This should never happen, since we already checked above.
        rassert(false);
        throw protob_server_exc_t(length_error_msg);
    }
    return ret;
}

void query_server_t::handle_conn(const scoped_ptr_t<tcp_conn_descriptor_t> &nconn,
                                 auto_drainer_t::lock_t keepalive) {
    // This must be read here because of home threads and stuff
    auth_key_t auth_key = auth_metadata->get().auth_key.get_ref();

    threadnum_t chosen_thread = threadnum_t(next_thread);
    next_thread = (next_thread + 1) % get_num_db_threads();

    cross_thread_signal_t ct_keepalive(keepalive.get_drain_signal(), chosen_thread);
    on_thread_t rethreader(chosen_thread);

    scoped_ptr_t<tcp_conn_t> conn;
    nconn->make_overcomplicated(&conn);
    conn->enable_keepalive();

#ifdef __linux
    linux_event_watcher_t *ew = conn->get_event_watcher();
    linux_event_watcher_t::watch_t conn_interrupted(ew, poll_event_rdhup);
    wait_any_t interruptor(&conn_interrupted, shutdown_signal(), &ct_keepalive);
#else
    wait_any_t interruptor(shutdown_signal(), &ct_keepalive);
#endif  // __linux

    std::string init_error;

    try {
        int32_t client_magic_number;
        conn->read(&client_magic_number, sizeof(client_magic_number), &interruptor);

        bool pre_2 = client_magic_number == VersionDummy::V0_1;
        bool pre_3 = pre_2 || client_magic_number == VersionDummy::V0_2;
        bool pre_4 = pre_3 || client_magic_number == VersionDummy::V0_3;
        bool legal = pre_4 || client_magic_number == VersionDummy::V0_4;

        // With version 0_2 and up, the client drivers specifies the authorization key
        if (pre_2) {
            if (!auth_key.str().empty()) {
                throw protob_server_exc_t(
                    "Authorization required but client does not support it.");
            }
        } else if (legal) {
            auth_key_t provided_auth = read_auth_key(conn.get(), &interruptor);
            if (!timing_sensitive_equals(provided_auth, auth_key)) {
                throw protob_server_exc_t("Incorrect authorization key.");
            }
            const char *success_msg = "SUCCESS";
            conn->write(success_msg, strlen(success_msg) + 1, &interruptor);
        } else {
            throw protob_server_exc_t("Received an unsupported protocol version. "
                                      "This port is for RethinkDB queries. Does your "
                                      "client driver version not match the server?");
        }

        size_t max_concurrent_queries = pre_4 ? 1 : 1024;

        // With version 0_3, the client driver specifies which protocol to use
        int32_t wire_protocol = VersionDummy::PROTOBUF;
        if (!pre_3) {
            conn->read(&wire_protocol, sizeof(wire_protocol), &interruptor);
        }

        client_context_t client_ctx(
            rdb_ctx,
            pre_4 ? ql::return_empty_normal_batches_t::YES
                  : ql::return_empty_normal_batches_t::NO);

        if (wire_protocol == VersionDummy::JSON) {
            connection_loop<json_protocol_t>(
                conn.get(), max_concurrent_queries, &interruptor, &client_ctx);
        } else if (wire_protocol == VersionDummy::PROTOBUF) {
            connection_loop<protobuf_protocol_t>(
                conn.get(), max_concurrent_queries, &interruptor, &client_ctx);
        } else {
            throw protob_server_exc_t(strprintf("Unrecognized protocol specified: '%d'",
                                                wire_protocol));
        }
    } catch (const protob_server_exc_t &ex) {
        // Can't write response here due to coro switching inside exception handler
        init_error = strprintf("ERROR: %s\n", ex.what());
    } catch (const tcp_conn_read_closed_exc_t &) {
        return;
    } catch (const tcp_conn_write_closed_exc_t &) {
        return;
    }

    try {
        if (!init_error.empty()) {
            conn->write(init_error.c_str(), init_error.length() + 1, &interruptor);
            conn->shutdown_write();
        }
    } catch (const tcp_conn_write_closed_exc_t &) {
        // Do nothing
    }
}

template <class protocol_t>
void query_server_t::connection_loop(tcp_conn_t *conn,
                                     size_t max_concurrent_queries,
                                     signal_t *raw_interruptor,
                                     client_context_t *client_ctx) {
    scoped_perfmon_counter_t connection_counter(&rdb_ctx->stats.client_connections);

    ip_and_port_t peer;
    bool got_peer_name = conn->getpeername(&peer);
    if (!got_peer_name) {
        // Return early and close the connection.
        return;
    }

    std::exception_ptr err;
    cond_t abort;
    wait_any_t conn_interruptor(raw_interruptor, &abort);
    new_mutex_t send_mutex;

    std_function_callback_t<ql::protob_t<Query> > callback(
        [&](ql::protob_t<Query> query, signal_t *coro_pool_interruptor) {
            try {
                wait_any_t cb_interruptor(
                    &conn_interruptor, coro_pool_interruptor);
                Response response;
                if (handler->run_query(
                        query, &response, &cb_interruptor, client_ctx, peer)) {
                    new_mutex_acq_t send_lock(&send_mutex);
                    protocol_t::send_response(response, handler, conn, &cb_interruptor);
                }
            } catch (...) {
                if (!err) {
                    err = std::current_exception();
                }
                abort.pulse_if_not_already_pulsed();
            }
        });
    // Pick a small limit so queries back up on the TCP connection.
    limited_fifo_queue_t<ql::protob_t<Query> > coro_queue(4);
    coro_pool_t<ql::protob_t<Query> > coro_pool(
        max_concurrent_queries, &coro_queue, &callback);

    for (;;) {
        ql::protob_t<Query> query(ql::make_counted_query());
        try {
            if (protocol_t::parse_query(conn, &conn_interruptor, handler, &query)) {
                coro_queue.push(std::move(query));
            }
        } catch (...) {
            if (err) {
                std::rethrow_exception(err);
            } else {
                throw;
            }
        }
    }
}

class conn_acq_t {
public:
    conn_acq_t() : conn(NULL) { }

    bool init(http_conn_cache_t::http_conn_t *_conn) {
        guarantee(conn == NULL);
        if (_conn->acquire()) {
            conn = _conn;
            return true;
        }
        return false;
    }

    ~conn_acq_t() {
        if (conn != NULL) {
            conn->release();
        }
    }
private:
    http_conn_cache_t::http_conn_t *conn;
};

void query_server_t::handle(const http_req_t &req,
                            http_res_t *result,
                            signal_t *interruptor) {
    auto_drainer_t::lock_t auto_drainer_lock(&auto_drainer);
    if (req.method == POST &&
        req.resource.as_string().find("open-new-connection") != std::string::npos) {
        int32_t conn_id = http_conn_cache.create(rdb_ctx);

        std::string body_data;
        body_data.assign(reinterpret_cast<char *>(&conn_id), sizeof(conn_id));
        result->set_body("application/octet-stream", body_data);
        result->code = HTTP_OK;
        return;
    }

    boost::optional<std::string> optional_conn_id = req.find_query_param("conn_id");
    if (!optional_conn_id) {
        *result = http_res_t(HTTP_BAD_REQUEST, "application/text",
                             "Required parameter \"conn_id\" missing\n");
        return;
    }

    std::string string_conn_id = *optional_conn_id;
    int32_t conn_id = boost::lexical_cast<int32_t>(string_conn_id);

    if (req.method == POST &&
        req.resource.as_string().find("close-connection") != std::string::npos) {
        http_conn_cache.erase(conn_id);
        result->code = HTTP_OK;
        return;
    }

    ql::protob_t<Query> query(ql::make_counted_query());
    Response response;
    int64_t token;

    if (req.body.size() < sizeof(token)) {
        *result = http_res_t(HTTP_BAD_REQUEST, "application/text",
                             "Client is buggy (request too small).");
        return;
    }

    // Parse the token out from the start of the request
    const char *data = req.body.c_str();
    token = *reinterpret_cast<const uint64_t *>(data);
    data += sizeof(token);

    const bool parse_succeeded =
        json_shim::parse_json_pb(query.get(), token, data);

    if (!parse_succeeded) {
        handler->unparseable_query(token, &response,
                                   "Client is buggy (failed to deserialize query).");
    } else {
        boost::shared_ptr<http_conn_cache_t::http_conn_t> conn =
            http_conn_cache.find(conn_id);
        if (!conn) {
            handler->unparseable_query(token, &response,
                                       "This HTTP connection is not open.");
        } else {
            // Make sure the connection is not already running a query
            conn_acq_t conn_acq;

            if (!conn_acq.init(conn.get())) {
                *result = http_res_t(HTTP_BAD_REQUEST, "application/text",
                                     "Session is already running a query");
                return;
            }

            // Check for noreply, which we don't support here, as it causes
            // problems with interruption
            ql::datum_t noreply = static_optarg("noreply", query);
            bool response_needed = !(noreply.has() &&
                                     noreply.get_type() == ql::datum_t::type_t::R_BOOL &&
                                     noreply.as_bool());

            if (!response_needed) {
                *result = http_res_t(HTTP_BAD_REQUEST, "application/text",
                                     "Noreply writes unsupported over HTTP\n");
                return;
            }

            client_context_t *client_ctx = conn->get_ctx();
            signal_t *conn_interruptor = conn->get_interruptor();
            wait_any_t true_interruptor(interruptor, conn_interruptor);
            response_needed = handler->run_query(
                query, &response, &true_interruptor, client_ctx, req.peer);
            rassert(response_needed);
        }
    }

    uint32_t size;
    std::string str;

    json_shim::write_json_pb(response, &str);
    size = str.size();

    char header_buffer[sizeof(token) + sizeof(size)];
    memcpy(&header_buffer[0], &token, sizeof(token));
    memcpy(&header_buffer[sizeof(token)], &size, sizeof(size));

    std::string body_data;
    body_data.reserve(sizeof(header_buffer) + str.length());
    body_data.append(&header_buffer[0], sizeof(header_buffer));
    body_data.append(str);
    result->set_body("application/octet-stream", body_data);
    result->code = HTTP_OK;
}
