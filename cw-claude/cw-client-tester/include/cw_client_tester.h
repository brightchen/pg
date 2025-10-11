#ifndef CW_CLIENT_TESTER_H
#define CW_CLIENT_TESTER_H

#include <stdint.h>
#include <time.h>
#include <netinet/in.h>
#include "cw_msg.h"
#include "cw_dtls.h"
#include "cw_log.h"

/* Application Constants */
#define MAX_CLIENTS 256
#define MAX_TEST_NAME_LEN 128
#define MAX_TEST_DESC_LEN 512
#define EPOLL_MAX_EVENTS 128
#define RECV_BUFFER_SIZE 4096
#define SEND_BUFFER_SIZE 4096

/* CAPWAP State Machine States (RFC 5415) */
#define CW_STATE_IDLE              0
#define CW_STATE_DISCOVERY         1
#define CW_STATE_DTLS_SETUP        2
#define CW_STATE_JOIN              3
#define CW_STATE_CONFIGURE         4
#define CW_STATE_DATA_CHECK        5
#define CW_STATE_RUN               6
#define CW_STATE_RESET             7
#define CW_STATE_DEAD              8

/* Test Result Codes */
#define TEST_RESULT_PASS           0
#define TEST_RESULT_FAIL           1
#define TEST_RESULT_TIMEOUT        2
#define TEST_RESULT_ERROR          3

/* Client Session Information */
typedef struct {
    int sockfd;
    struct sockaddr_in addr;
    cw_dtls_ctx_t *dtls_ctx;
    int state;
    uint8_t session_id[16];
    uint8_t session_id_len;
    uint8_t seq_num;
    time_t last_activity;
    int active;
} client_session_t;

/* Test Case Definition */
typedef struct {
    char name[MAX_TEST_NAME_LEN];
    char description[MAX_TEST_DESC_LEN];
    int (*test_func)(client_session_t *session);
    int expected_state;
    int timeout_seconds;
} test_case_t;

/* Application Configuration */
typedef struct {
    char listen_addr[64];
    uint16_t listen_port;
    char ac_name[256];
    int use_dtls;
    char cert_file[256];
    char key_file[256];
    char ca_file[256];
    int max_clients;
    int echo_interval;
    char log_config_file[256];
} app_config_t;

/* Application Context */
typedef struct {
    app_config_t config;
    int sockfd;
    int epoll_fd;
    client_session_t clients[MAX_CLIENTS];
    int client_count;
    int running;
    test_case_t *current_test;
} app_context_t;

/* Function Declarations */

/**
 * Initialize application context
 */
int app_init(app_context_t *ctx, const char *config_file);

/**
 * Cleanup application context
 */
void app_cleanup(app_context_t *ctx);

/**
 * Main event loop
 */
int app_run(app_context_t *ctx);

/**
 * Handle incoming packet
 */
int handle_packet(app_context_t *ctx, client_session_t *session,
                  const uint8_t *buffer, size_t len);

/**
 * Handle Discovery Request
 */
int handle_discovery_request(app_context_t *ctx, client_session_t *session,
                              const cw_control_msg_t *msg);

/**
 * Handle Join Request
 */
int handle_join_request(app_context_t *ctx, client_session_t *session,
                         const cw_control_msg_t *msg);

/**
 * Handle Configuration Status Request
 */
int handle_config_status_request(app_context_t *ctx, client_session_t *session,
                                  const cw_control_msg_t *msg);

/**
 * Handle Echo Request
 */
int handle_echo_request(app_context_t *ctx, client_session_t *session,
                         const cw_control_msg_t *msg);

/**
 * Send Discovery Response
 */
int send_discovery_response(app_context_t *ctx, client_session_t *session, uint8_t seq_num);

/**
 * Send Join Response
 */
int send_join_response(app_context_t *ctx, client_session_t *session, uint8_t seq_num);

/**
 * Send Configuration Status Response
 */
int send_config_status_response(app_context_t *ctx, client_session_t *session, uint8_t seq_num);

/**
 * Send Echo Response
 */
int send_echo_response(app_context_t *ctx, client_session_t *session, uint8_t seq_num);

/**
 * Send Echo Request
 */
int send_echo_request(app_context_t *ctx, client_session_t *session);

/**
 * Find or create client session
 */
client_session_t* find_or_create_session(app_context_t *ctx, struct sockaddr_in *addr);

/**
 * Run test case
 */
int run_test_case(app_context_t *ctx, test_case_t *test);

#endif /* CW_CLIENT_TESTER_H */
