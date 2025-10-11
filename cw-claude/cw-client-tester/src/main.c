#include "cw_client_tester.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

static app_context_t g_app_ctx;
static volatile int g_shutdown_requested = 0;

void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        CW_LOG_INFO("Shutdown signal received");
        g_shutdown_requested = 1;
        g_app_ctx.running = 0;
    }
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -c <config_file>  Configuration file path\n");
    printf("  -h                Show this help message\n");
}

int main(int argc, char *argv[]) {
    const char *config_file = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "c:h")) != -1) {
        switch (opt) {
            case 'c':
                config_file = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (config_file == NULL) {
        config_file = "config/client_tester.conf";
    }

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize application */
    CW_LOG_INFO("Starting CAPWAP Client Tester");
    CW_LOG_INFO("Configuration file: %s", config_file);

    if (app_init(&g_app_ctx, config_file) < 0) {
        CW_LOG_ERROR("Failed to initialize application");
        return 1;
    }

    CW_LOG_INFO("CAPWAP Client Tester initialized successfully");
    CW_LOG_INFO("Listening on %s:%d", g_app_ctx.config.listen_addr,
                g_app_ctx.config.listen_port);

    /* Run main event loop */
    int ret = app_run(&g_app_ctx);

    /* Cleanup */
    CW_LOG_INFO("Shutting down CAPWAP Client Tester");
    app_cleanup(&g_app_ctx);

    CW_LOG_INFO("CAPWAP Client Tester stopped");
    return ret;
}

int app_init(app_context_t *ctx, const char *config_file) {
    if (ctx == NULL) {
        return -1;
    }

    memset(ctx, 0, sizeof(app_context_t));

    /* Initialize logging */
    int log_ret = cw_log_init_from_file(config_file);
    if (log_ret != CW_LOG_SUCCESS) {
        /* Fall back to default config */
        fprintf(stderr, "Warning: Failed to load log config: %s (%d). Using defaults.\n",
                cw_log_error_string(log_ret), log_ret);
        cw_log_config_t log_config = {
            .log_file_path = "/tmp/cw_client_tester.log",
            .log_level = CW_LOG_LEVEL_INFO,
            .log_format = CW_LOG_FORMAT_TEXT,
            .max_file_size = 10 * 1024 * 1024,
            .max_backup_files = 5,
            .console_output = 1
        };
        int init_ret = cw_log_init(&log_config);
        if (init_ret != CW_LOG_SUCCESS) {
            fprintf(stderr, "Error: Failed to initialize logging: %s (%d)\n",
                    cw_log_error_string(init_ret), init_ret);
        }
    }

    /* Load configuration - using defaults for now */
    strcpy(ctx->config.listen_addr, "0.0.0.0");
    ctx->config.listen_port = 5246;  /* CAPWAP control port */
    strcpy(ctx->config.ac_name, "TestAC");
    ctx->config.use_dtls = 0;  /* Disable DTLS for basic testing */
    ctx->config.max_clients = MAX_CLIENTS;
    ctx->config.echo_interval = 30;

    /* Create UDP socket */
    ctx->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sockfd < 0) {
        CW_LOG_ERROR("Failed to create socket: %s", strerror(errno));
        return -1;
    }

    /* Bind socket */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ctx->config.listen_addr);
    addr.sin_port = htons(ctx->config.listen_port);

    if (bind(ctx->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        CW_LOG_ERROR("Failed to bind socket: %s", strerror(errno));
        close(ctx->sockfd);
        return -1;
    }

    /* Create epoll instance */
    ctx->epoll_fd = epoll_create1(0);
    if (ctx->epoll_fd < 0) {
        CW_LOG_ERROR("Failed to create epoll: %s", strerror(errno));
        close(ctx->sockfd);
        return -1;
    }

    /* Add socket to epoll */
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = ctx->sockfd;
    if (epoll_ctl(ctx->epoll_fd, EPOLL_CTL_ADD, ctx->sockfd, &ev) < 0) {
        CW_LOG_ERROR("Failed to add socket to epoll: %s", strerror(errno));
        close(ctx->epoll_fd);
        close(ctx->sockfd);
        return -1;
    }

    ctx->running = 1;
    return 0;
}

void app_cleanup(app_context_t *ctx) {
    if (ctx == NULL) {
        return;
    }

    /* Close all client sessions */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ctx->clients[i].active) {
            if (ctx->clients[i].dtls_ctx != NULL) {
                cw_dtls_destroy_context(ctx->clients[i].dtls_ctx);
            }
            ctx->clients[i].active = 0;
        }
    }

    /* Close sockets */
    if (ctx->sockfd >= 0) {
        close(ctx->sockfd);
        ctx->sockfd = -1;
    }

    if (ctx->epoll_fd >= 0) {
        close(ctx->epoll_fd);
        ctx->epoll_fd = -1;
    }

    /* Cleanup logging */
    cw_log_cleanup();
}

int app_run(app_context_t *ctx) {
    if (ctx == NULL) {
        return -1;
    }

    struct epoll_event events[EPOLL_MAX_EVENTS];
    uint8_t recv_buffer[RECV_BUFFER_SIZE];

    CW_LOG_INFO("Entering main event loop");

    while (ctx->running && !g_shutdown_requested) {
        int nfds = epoll_wait(ctx->epoll_fd, events, EPOLL_MAX_EVENTS, 1000);

        if (nfds < 0) {
            if (errno == EINTR) {
                continue;
            }
            CW_LOG_ERROR("epoll_wait failed: %s", strerror(errno));
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == ctx->sockfd) {
                /* Handle incoming packet */
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);

                ssize_t recv_len = recvfrom(ctx->sockfd, recv_buffer, sizeof(recv_buffer),
                                            0, (struct sockaddr *)&client_addr, &addr_len);

                if (recv_len > 0) {
                    CW_LOG_DEBUG("Received %zd bytes from %s:%d",
                                recv_len, inet_ntoa(client_addr.sin_addr),
                                ntohs(client_addr.sin_port));

                    /* Find or create session */
                    client_session_t *session = find_or_create_session(ctx, &client_addr);
                    if (session != NULL) {
                        handle_packet(ctx, session, recv_buffer, recv_len);
                    }
                }
            }
        }

        /* Check for timeouts and send echo requests */
        time_t now = time(NULL);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (ctx->clients[i].active) {
                if (now - ctx->clients[i].last_activity > ctx->config.echo_interval) {
                    /* Send echo request */
                    send_echo_request(ctx, &ctx->clients[i]);
                    ctx->clients[i].last_activity = now;
                }
            }
        }
    }

    CW_LOG_INFO("Exiting main event loop");
    return 0;
}

client_session_t* find_or_create_session(app_context_t *ctx, struct sockaddr_in *addr) {
    if (ctx == NULL || addr == NULL) {
        return NULL;
    }

    /* Find existing session */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (ctx->clients[i].active &&
            ctx->clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            ctx->clients[i].addr.sin_port == addr->sin_port) {
            return &ctx->clients[i];
        }
    }

    /* Create new session */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!ctx->clients[i].active) {
            memset(&ctx->clients[i], 0, sizeof(client_session_t));
            ctx->clients[i].sockfd = ctx->sockfd;
            memcpy(&ctx->clients[i].addr, addr, sizeof(struct sockaddr_in));
            ctx->clients[i].state = CW_STATE_IDLE;
            ctx->clients[i].active = 1;
            ctx->clients[i].last_activity = time(NULL);
            ctx->client_count++;

            CW_LOG_INFO("New client session created: %s:%d",
                       inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));

            return &ctx->clients[i];
        }
    }

    CW_LOG_WARN("Maximum number of clients reached");
    return NULL;
}

int handle_packet(app_context_t *ctx, client_session_t *session,
                  const uint8_t *buffer, size_t len) {
    if (ctx == NULL || session == NULL || buffer == NULL || len == 0) {
        return -1;
    }

    /* Parse CAPWAP message */
    cw_control_msg_t msg;
    int parse_len = cw_parse_control_msg(buffer, len, &msg);

    if (parse_len < 0) {
        CW_LOG_ERROR("Failed to parse CAPWAP message: %s (%d)",
                     cw_msg_error_string(parse_len), parse_len);
        return -1;
    }

    CW_LOG_INFO("Received %s (seq=%d) from %s:%d",
               cw_msg_type_name(msg.ctrl_header.message_type),
               msg.ctrl_header.seq_num,
               inet_ntoa(session->addr.sin_addr),
               ntohs(session->addr.sin_port));

    /* Handle based on message type */
    switch (msg.ctrl_header.message_type) {
        case CW_MSG_TYPE_DISCOVERY_REQUEST:
            return handle_discovery_request(ctx, session, &msg);

        case CW_MSG_TYPE_JOIN_REQUEST:
            return handle_join_request(ctx, session, &msg);

        case CW_MSG_TYPE_CONFIGURATION_STATUS_REQUEST:
            return handle_config_status_request(ctx, session, &msg);

        case CW_MSG_TYPE_ECHO_REQUEST:
            return handle_echo_request(ctx, session, &msg);

        default:
            CW_LOG_WARN("Unhandled message type: %d", msg.ctrl_header.message_type);
            return 0;
    }
}

int handle_discovery_request(app_context_t *ctx, client_session_t *session,
                              const cw_control_msg_t *msg) {
    CW_LOG_DEBUG("Handling Discovery Request");

    session->state = CW_STATE_DISCOVERY;
    return send_discovery_response(ctx, session, msg->ctrl_header.seq_num);
}

int handle_join_request(app_context_t *ctx, client_session_t *session,
                         const cw_control_msg_t *msg) {
    CW_LOG_DEBUG("Handling Join Request");

    /* Generate session ID */
    for (int i = 0; i < 16; i++) {
        session->session_id[i] = rand() % 256;
    }
    session->session_id_len = 16;

    session->state = CW_STATE_JOIN;
    return send_join_response(ctx, session, msg->ctrl_header.seq_num);
}

int handle_config_status_request(app_context_t *ctx, client_session_t *session,
                                  const cw_control_msg_t *msg) {
    CW_LOG_DEBUG("Handling Configuration Status Request");

    session->state = CW_STATE_RUN;
    return send_config_status_response(ctx, session, msg->ctrl_header.seq_num);
}

int handle_echo_request(app_context_t *ctx, client_session_t *session,
                         const cw_control_msg_t *msg) {
    CW_LOG_DEBUG("Handling Echo Request");

    session->last_activity = time(NULL);
    return send_echo_response(ctx, session, msg->ctrl_header.seq_num);
}

int send_discovery_response(app_context_t *ctx, client_session_t *session, uint8_t seq_num) {
    uint8_t buffer[SEND_BUFFER_SIZE];

    /* Create Discovery Response */
    cw_control_msg_t resp;
    cw_init_control_msg(&resp, CW_MSG_TYPE_DISCOVERY_RESPONSE, seq_num);

    /* Add AC Descriptor */
    cw_ac_descriptor_t ac_desc = {
        .stations = 0,
        .limit = 1000,
        .active_wtps = 0,
        .max_wtps = 100,
        .security = 0x01,
        .rmac_field = 0x01,
        .reserved = 0,
        .dtls_policy = 0x01
    };

    uint8_t elem_buf[128];
    int elem_len = cw_generate_ac_descriptor(&ac_desc, elem_buf, sizeof(elem_buf));
    if (elem_len > 4) {
        cw_add_element(&resp, CW_ELEM_AC_DESCRIPTOR, &elem_buf[4], elem_len - 4);
    }

    /* Add AC Name */
    cw_add_element(&resp, CW_ELEM_AC_NAME,
                   (uint8_t *)ctx->config.ac_name, strlen(ctx->config.ac_name));

    /* Generate message */
    int msg_len = cw_generate_control_msg(&resp, buffer, sizeof(buffer));
    if (msg_len < 0) {
        CW_LOG_ERROR("Failed to generate Discovery Response: %s (%d)",
                     cw_msg_error_string(msg_len), msg_len);
        return -1;
    }

    /* Send response */
    ssize_t sent = sendto(session->sockfd, buffer, msg_len, 0,
                         (struct sockaddr *)&session->addr, sizeof(session->addr));

    if (sent < 0) {
        CW_LOG_ERROR("Failed to send Discovery Response: %s (errno=%d)",
                     strerror(errno), errno);
        return -1;
    }

    CW_LOG_INFO("Sent Discovery Response (seq=%d) to %s:%d",
               seq_num, inet_ntoa(session->addr.sin_addr),
               ntohs(session->addr.sin_port));

    return 0;
}

int send_join_response(app_context_t *ctx, client_session_t *session, uint8_t seq_num) {
    (void)ctx;  /* Unused parameter */
    uint8_t buffer[SEND_BUFFER_SIZE];

    /* Create Join Response */
    cw_control_msg_t resp;
    cw_init_control_msg(&resp, CW_MSG_TYPE_JOIN_RESPONSE, seq_num);

    /* Add Result Code */
    uint8_t result_buf[16];
    int result_len = cw_generate_result_code(CW_RESULT_SUCCESS, result_buf, sizeof(result_buf));
    if (result_len > 4) {
        cw_add_element(&resp, CW_ELEM_RESULT_CODE, &result_buf[4], result_len - 4);
    }

    /* Add Session ID */
    cw_session_id_t sid;
    memcpy(sid.session_id, session->session_id, 16);
    sid.length = 16;

    uint8_t sid_buf[32];
    int sid_len = cw_generate_session_id(&sid, sid_buf, sizeof(sid_buf));
    if (sid_len > 4) {
        cw_add_element(&resp, CW_ELEM_SESSION_ID, &sid_buf[4], sid_len - 4);
    }

    /* Generate message */
    int msg_len = cw_generate_control_msg(&resp, buffer, sizeof(buffer));
    if (msg_len < 0) {
        CW_LOG_ERROR("Failed to generate Join Response: %s (%d)",
                     cw_msg_error_string(msg_len), msg_len);
        return -1;
    }

    /* Send response */
    ssize_t sent = sendto(session->sockfd, buffer, msg_len, 0,
                         (struct sockaddr *)&session->addr, sizeof(session->addr));

    if (sent < 0) {
        CW_LOG_ERROR("Failed to send Join Response: %s (errno=%d)",
                     strerror(errno), errno);
        return -1;
    }

    CW_LOG_INFO("Sent Join Response (seq=%d) to %s:%d",
               seq_num, inet_ntoa(session->addr.sin_addr),
               ntohs(session->addr.sin_port));

    return 0;
}

int send_config_status_response(app_context_t *ctx, client_session_t *session, uint8_t seq_num) {
    (void)ctx;  /* Unused parameter */
    uint8_t buffer[SEND_BUFFER_SIZE];

    /* Create Configuration Status Response */
    cw_control_msg_t resp;
    cw_init_control_msg(&resp, CW_MSG_TYPE_CONFIGURATION_STATUS_RESPONSE, seq_num);

    /* Add Result Code */
    uint8_t result_buf[16];
    int result_len = cw_generate_result_code(CW_RESULT_SUCCESS, result_buf, sizeof(result_buf));
    if (result_len > 4) {
        cw_add_element(&resp, CW_ELEM_RESULT_CODE, &result_buf[4], result_len - 4);
    }

    /* Generate message */
    int msg_len = cw_generate_control_msg(&resp, buffer, sizeof(buffer));
    if (msg_len < 0) {
        CW_LOG_ERROR("Failed to generate Configuration Status Response: %s (%d)",
                     cw_msg_error_string(msg_len), msg_len);
        return -1;
    }

    /* Send response */
    ssize_t sent = sendto(session->sockfd, buffer, msg_len, 0,
                         (struct sockaddr *)&session->addr, sizeof(session->addr));

    if (sent < 0) {
        CW_LOG_ERROR("Failed to send Configuration Status Response: %s (errno=%d)",
                     strerror(errno), errno);
        return -1;
    }

    CW_LOG_INFO("Sent Configuration Status Response (seq=%d) to %s:%d",
               seq_num, inet_ntoa(session->addr.sin_addr),
               ntohs(session->addr.sin_port));

    return 0;
}

int send_echo_response(app_context_t *ctx, client_session_t *session, uint8_t seq_num) {
    (void)ctx;  /* Unused parameter */
    uint8_t buffer[SEND_BUFFER_SIZE];

    /* Create Echo Response */
    cw_control_msg_t resp;
    cw_init_control_msg(&resp, CW_MSG_TYPE_ECHO_RESPONSE, seq_num);

    /* Generate message */
    int msg_len = cw_generate_control_msg(&resp, buffer, sizeof(buffer));
    if (msg_len < 0) {
        CW_LOG_ERROR("Failed to generate Echo Response: %s (%d)",
                     cw_msg_error_string(msg_len), msg_len);
        return -1;
    }

    /* Send response */
    ssize_t sent = sendto(session->sockfd, buffer, msg_len, 0,
                         (struct sockaddr *)&session->addr, sizeof(session->addr));

    if (sent < 0) {
        CW_LOG_ERROR("Failed to send Echo Response: %s (errno=%d)",
                     strerror(errno), errno);
        return -1;
    }

    CW_LOG_DEBUG("Sent Echo Response (seq=%d) to %s:%d",
                seq_num, inet_ntoa(session->addr.sin_addr),
                ntohs(session->addr.sin_port));

    return 0;
}

int send_echo_request(app_context_t *ctx, client_session_t *session) {
    (void)ctx;  /* Unused parameter */
    uint8_t buffer[SEND_BUFFER_SIZE];

    /* Create Echo Request */
    cw_control_msg_t req;
    cw_init_control_msg(&req, CW_MSG_TYPE_ECHO_REQUEST, session->seq_num++);

    /* Generate message */
    int msg_len = cw_generate_control_msg(&req, buffer, sizeof(buffer));
    if (msg_len < 0) {
        return -1;
    }

    /* Send request */
    ssize_t sent = sendto(session->sockfd, buffer, msg_len, 0,
                         (struct sockaddr *)&session->addr, sizeof(session->addr));

    if (sent < 0) {
        return -1;
    }

    CW_LOG_DEBUG("Sent Echo Request (seq=%d) to %s:%d",
                session->seq_num - 1, inet_ntoa(session->addr.sin_addr),
                ntohs(session->addr.sin_port));

    return 0;
}
