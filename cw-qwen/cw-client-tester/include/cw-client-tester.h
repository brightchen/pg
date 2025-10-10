#ifndef CW_CLIENT_TESTER_H
#define CW_CLIENT_TESTER_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "capwap.h"

// Application states
typedef enum {
    APP_STATE_INITIALIZING,
    APP_STATE_RUNNING,
    APP_STATE_SHUTTING_DOWN,
    APP_STATE_STOPPED
} app_state_t;

// CAPWAP protocol states
typedef enum {
    CAPWAP_STATE_IDLE,
    CAPWAP_STATE_DISCOVERY,
    CAPWAP_STATE_JOIN,
    CAPWAP_STATE_CONFIG_STATUS,
    CAPWAP_STATE_RUN,
    CAPWAP_STATE_RESET
} capwap_state_t;

// Constant definitions
#define DEFAULT_AC_NAME "CAPWAP-Test-Server"
#define DEFAULT_DISCOVERY_INTERVAL 60    // 60 seconds
#define DEFAULT_ECHO_INTERVAL 30         // 30 seconds
#define DEFAULT_MAX_ECHO_INTERVAL 180    // 180 seconds
#define DEFAULT_DISCOVERY_COUNT 3        // Discovery attempts
#define DEFAULT_SESSION_ID_LENGTH 16
#define MAX_CLIENT_BUFFER_SIZE 4096
#define MAX_RESPONSE_BUFFER_SIZE 1024
#define MAX_SHORT_RESPONSE_BUFFER_SIZE 512
#define SERVER_SOCKET_TIMEOUT_SEC 1
#define AC_DESCRIPTOR_BUFFER_SIZE 100
#define LOG_BUFFER_SIZE 2048
#define MAX_CONFIG_LINE_LENGTH 512
#define MAX_SECTION_NAME_LENGTH 64
#define DEFAULT_LOG_FILE_SIZE_LIMIT (10 * 1024 * 1024) // 10MB
#define TEST_TIMEOUT_SECONDS 300         // 5 minutes
#define CLIENT_ACTIVITY_TIMEOUT 300      // 5 minutes (in seconds)

// Log levels
typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} log_level_t;

// Log format types
typedef enum {
    LOG_FORMAT_TEXT,
    LOG_FORMAT_CSV,
    LOG_FORMAT_JSON
} log_format_t;

// Forward declarations
typedef struct test_case test_case_t;
typedef struct workflow_result workflow_result_t;

// Workflow event structure
typedef struct {
    capwap_msg_type_t msg_type;
    capwap_state_t current_state;
    void *data;
    size_t data_len;
} workflow_event_t;

// Workflow result structure
typedef struct workflow_result {
    bool success;
    long execution_time_ms;
    char *error_message;  // NULL if successful
    capwap_state_t final_state;
} workflow_result_t;

// Test case structure
struct test_case {
    char *name;
    char *description;
    capwap_state_t initial_state;
    workflow_result_t* (*workflow)(test_case_t *tc, void *event_data);
    test_case_t *next;
};



// Logger configuration
typedef struct {
    log_level_t level;
    log_format_t format;
    char *file_path;
    size_t max_file_size;
    bool console_output;
} logger_config_t;

// Client state tracking structure
typedef struct client_context {
    struct sockaddr_in client_addr;
    capwap_state_t state;
    uint32_t session_id;
    time_t last_activity;
    struct client_context *next;
} client_context_t;

// Main application context
typedef struct {
    app_state_t state;
    pthread_t server_thread;
    int server_socket;
    struct sockaddr_in server_addr;
    test_case_t *test_cases;
    logger_config_t log_config;
    pthread_mutex_t state_mutex;
    pthread_cond_t state_cond;
    bool shutdown_requested;
    bool force_shutdown;
    client_context_t *clients;  // List of connected clients
} app_context_t;



// Global application context
extern app_context_t g_app_ctx;

// Function declarations

// Main application functions
int app_init(int argc, char *argv[]);
void app_run(void);
void app_shutdown(bool force);
void app_cleanup(void);

// CAPWAP server functions
int capwap_server_init(void);
int capwap_server_run(void);
void capwap_server_shutdown(void);
void* capwap_server_thread(void *arg);

// Message handling functions
int handle_discovery_request(uint8_t *buffer, size_t len, struct sockaddr_in *client_addr);
int handle_join_request(uint8_t *buffer, size_t len, struct sockaddr_in *client_addr);
int handle_echo_request(uint8_t *buffer, size_t len, struct sockaddr_in *client_addr);
int handle_config_status_request(uint8_t *buffer, size_t len, struct sockaddr_in *client_addr);
int handle_change_state_event_request(uint8_t *buffer, size_t len, struct sockaddr_in *client_addr);

// Response functions
int send_discovery_response(struct sockaddr_in *client_addr);
int send_join_response(struct sockaddr_in *client_addr, capwap_result_code_t result_code);
int send_echo_response(struct sockaddr_in *client_addr, capwap_result_code_t result_code);
int send_config_status_response(struct sockaddr_in *client_addr, capwap_result_code_t result_code);
int send_change_state_event_response(struct sockaddr_in *client_addr, capwap_result_code_t result_code);

// Logging functions
int logger_init(logger_config_t *config);
void logger_log(log_level_t level, const char *file, int line, const char *func, const char *format, ...);
void logger_cleanup(void);
void logger_set_level(log_level_t level);

// Test case functions
test_case_t* create_test_case(const char *name, const char *desc, capwap_state_t initial_state, 
                             workflow_result_t* (*workflow)(test_case_t *tc, void *event_data));
void register_test_case(test_case_t *tc);
void run_test_cases(void);
void init_test_cases(void);
void log_workflow_summary(test_case_t *tc, workflow_result_t *result);
void free_workflow_result(workflow_result_t *result);

// Utility functions
char* get_current_time_str(void);
void signal_handler(int sig);
int parse_config_file(const char *config_path, logger_config_t *log_config);

#endif // CW_CLIENT_TESTER_H