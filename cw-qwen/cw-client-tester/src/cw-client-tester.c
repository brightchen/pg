#include "cw-client-tester.h"

// Global application context
app_context_t g_app_ctx = {0};

// Function to find or create client context
client_context_t* find_or_create_client(struct sockaddr_in *client_addr) {
    client_context_t *current = g_app_ctx.clients;
    
    // Look for existing client
    while (current) {
        if (current->client_addr.sin_addr.s_addr == client_addr->sin_addr.s_addr &&
            current->client_addr.sin_port == client_addr->sin_port) {
            // Update last activity time
            current->last_activity = time(NULL);
            return current;
        }
        current = current->next;
    }
    
    // Create new client context
    client_context_t *new_client = malloc(sizeof(client_context_t));
    if (!new_client) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to allocate client context");
        return NULL;
    }
    
    new_client->client_addr = *client_addr;
    new_client->state = CAPWAP_STATE_IDLE;
    new_client->session_id = 0;
    new_client->last_activity = time(NULL);
    new_client->next = g_app_ctx.clients;
    
    g_app_ctx.clients = new_client;
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
              "New client registered: %s:%d", 
              inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    
    return new_client;
}

// Function to update client state
void update_client_state(client_context_t *client, capwap_state_t new_state) {
    if (client) {
        logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                  "Client state transition: %d -> %d", client->state, new_state);
        client->state = new_state;
    }
}

// Log level strings
static const char* log_level_strings[] = {
    "TRACE",
    "DEBUG", 
    "INFO",
    "WARN",
    "ERROR"
};

// Log format strings for different formats
static FILE *log_file = NULL;
static logger_config_t current_log_config = {0};
static size_t current_log_size = 0;

int logger_init(logger_config_t *config) {
    // Copy configuration
    current_log_config = *config;
    
    if (config->console_output) {
        // Log to console
        return 0;
    }
    
    if (config->file_path) {
        log_file = fopen(config->file_path, "a");
        if (!log_file) {
            fprintf(stderr, "Failed to open log file: %s\n", config->file_path);
            return -1;
        }
        
        // Get current file size
        fseek(log_file, 0, SEEK_END);
        current_log_size = ftell(log_file);
        fseek(log_file, 0, SEEK_SET);
        
        return 0;
    }
    
    return -1;
}

void logger_log(log_level_t level, const char *file, int line, const char *func, const char *format, ...) {
    if (level < current_log_config.level) {
        return; // Skip logging if below configured level
    }
    
    va_list args;
    va_start(args, format);
    
    char time_str[64];
    time_t rawtime;
    struct tm *timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    // Get the filename only (remove path)
    const char *filename = strrchr(file, '/');
    if (filename) {
        filename++;
    } else {
        filename = file;
    }
    
    // Prepare log message
    char log_msg[2048];
    int len = snprintf(log_msg, sizeof(log_msg), "[%s] [%s] [%s:%d] [%s] ", 
                      time_str, log_level_strings[level], filename, line, func);
    
    if (len > 0 && len < sizeof(log_msg)) {
        vsnprintf(log_msg + len, sizeof(log_msg) - len, format, args);
    }
    
    // Add newline if not present
    size_t msg_len = strlen(log_msg);
    if (msg_len > 0 && log_msg[msg_len-1] != '\n') {
        strcat(log_msg, "\n");
    }
    
    // Output based on format
    switch (current_log_config.format) {
        case LOG_FORMAT_TEXT:
            if (current_log_config.console_output) {
                fprintf(stdout, "%s", log_msg);
                fflush(stdout);
            }
            if (log_file) {
                fprintf(log_file, "%s", log_msg);
                fflush(log_file);
                
                // Update file size and check for rollover
                current_log_size += strlen(log_msg);
                if (current_log_config.max_file_size > 0 && 
                    current_log_size >= current_log_config.max_file_size) {
                    // Rotate log file
                    fclose(log_file);
                    
                    // Create backup filename
                    char backup_path[512];
                    snprintf(backup_path, sizeof(backup_path), "%s.old", current_log_config.file_path);
                    
                    // Rename current log file
                    rename(current_log_config.file_path, backup_path);
                    
                    // Open new log file
                    log_file = fopen(current_log_config.file_path, "w");
                    current_log_size = 0;
                }
            }
            break;
            
        case LOG_FORMAT_CSV:
            if (current_log_config.console_output) {
                fprintf(stdout, "\"%s\",\"%s\",\"%s\",\"%d\",\"%s\",\"%s\"\n", 
                       time_str, log_level_strings[level], filename, line, func, format);
                fflush(stdout);
            }
            if (log_file) {
                fprintf(log_file, "\"%s\",\"%s\",\"%s\",\"%d\",\"%s\",\"%s\"\n", 
                       time_str, log_level_strings[level], filename, line, func, format);
                fflush(log_file);
            }
            break;
            
        case LOG_FORMAT_JSON:
        {
            char json_msg[2048];
            snprintf(json_msg, sizeof(json_msg), 
                    "{\"timestamp\":\"%s\",\"level\":\"%s\",\"file\":\"%s\",\"line\":%d,\"function\":\"%s\",\"message\":\"%s\"}\n",
                    time_str, log_level_strings[level], filename, line, func, format);
            
            if (current_log_config.console_output) {
                fprintf(stdout, "%s", json_msg);
                fflush(stdout);
            }
            if (log_file) {
                fprintf(log_file, "%s", json_msg);
                fflush(log_file);
            }
        }
            break;
    }
    
    va_end(args);
}

void logger_cleanup(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

void logger_set_level(log_level_t level) {
    current_log_config.level = level;
}

test_case_t* create_test_case(const char *name, const char *desc, capwap_state_t initial_state, 
                             workflow_result_t* (*workflow)(test_case_t *tc, void *event_data)) {
    test_case_t *tc = malloc(sizeof(test_case_t));
    if (!tc) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to allocate memory for test case");
        return NULL;
    }
    
    tc->name = strdup(name);
    tc->description = strdup(desc);
    tc->initial_state = initial_state;
    tc->workflow = workflow;
    tc->next = NULL;
    
    if (!tc->name || !tc->description) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to allocate memory for test case strings");
        if (tc->name) free(tc->name);
        if (tc->description) free(tc->description);
        free(tc);
        return NULL;
    }
    
    return tc;
}

void register_test_case(test_case_t *tc) {
    if (!tc) return;
    
    if (!g_app_ctx.test_cases) {
        g_app_ctx.test_cases = tc;
    } else {
        test_case_t *current = g_app_ctx.test_cases;
        while (current->next) {
            current = current->next;
        }
        current->next = tc;
    }
    
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Registered test case: %s", tc->name);
}

void run_test_cases(void) {
    test_case_t *current = g_app_ctx.test_cases;
    while (current) {
        logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Running test case: %s - %s", 
                   current->name, current->description);
        
        // Initialize with the initial state
        capwap_state_t state = current->initial_state;
        
        // Process events based on the state - in a real implementation, this would
        // receive actual events from the CAPWAP client
        logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, "Initial state for test case: %d", state);
        
        // If the test case has a custom workflow, execute it
        if (current->workflow) {
            workflow_event_t dummy_event = {0};
            dummy_event.msg_type = CAPWAP_DISCOVERY_REQUEST; // Example event
            dummy_event.current_state = state;
            
            // Cast the workflow function to the new signature
            workflow_result_t* (*workflow_func)(test_case_t*, void*) = 
                (workflow_result_t* (*)(test_case_t*, void*))current->workflow;
                
            workflow_result_t *result = workflow_func(current, &dummy_event);
            
            if (result) {
                // Log the summary of the workflow execution
                log_workflow_summary(current, result);
                
                // Free the result
                free_workflow_result(result);
            } else {
                logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, 
                          "Workflow for test case '%s' returned NULL result", current->name);
            }
        } else {
            logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, 
                      "Test case '%s' has no workflow function", current->name);
        }
        
        logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Test case %s completed", current->name);
        current = current->next;
    }
}

int capwap_server_init(void) {
    // Create UDP socket for CAPWAP discovery (port 5246)
    g_app_ctx.server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_app_ctx.server_socket == -1) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to create socket: %s", strerror(errno));
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(g_app_ctx.server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to set socket options: %s", strerror(errno));
        close(g_app_ctx.server_socket);
        return -1;
    }
    
    // Setup server address
    memset(&g_app_ctx.server_addr, 0, sizeof(g_app_ctx.server_addr));
    g_app_ctx.server_addr.sin_family = AF_INET;
    g_app_ctx.server_addr.sin_port = htons(CAPWAP_PORT_CONTROL); // 5246
    g_app_ctx.server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    
    // Bind socket
    if (bind(g_app_ctx.server_socket, (struct sockaddr*)&g_app_ctx.server_addr, sizeof(g_app_ctx.server_addr)) < 0) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Bind failed: %s", strerror(errno));
        close(g_app_ctx.server_socket);
        return -1;
    }
    
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "CAPWAP server initialized on port %d", CAPWAP_PORT_CONTROL);
    return 0;
}

int capwap_server_run(void) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Starting CAPWAP server loop");
    
    while (!g_app_ctx.shutdown_requested) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(g_app_ctx.server_socket, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = SERVER_SOCKET_TIMEOUT_SEC;  // Server socket timeout in seconds
        timeout.tv_usec = 0;
        
        int activity = select(g_app_ctx.server_socket + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR) {
            logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Select error: %s", strerror(errno));
            break;
        }
        
        if (activity > 0 && FD_ISSET(g_app_ctx.server_socket, &readfds)) {
            // Receive data from client
            uint8_t buffer[MAX_CLIENT_BUFFER_SIZE];
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            
            ssize_t bytes_received = recvfrom(g_app_ctx.server_socket, buffer, sizeof(buffer)-1, 0,
                                            (struct sockaddr*)&client_addr, &addr_len);
            if (bytes_received < 0) {
                logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Receive error: %s", strerror(errno));
                continue;
            }
            
            logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                      "Received %zd bytes from %s:%d", bytes_received, 
                      inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            
            // Get or create client context
            client_context_t *client_ctx = find_or_create_client(&client_addr);
            if (!client_ctx) {
                logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, 
                          "Failed to get client context, dropping message");
                continue;
            }
            
            logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                      "Client state before processing: %d", client_ctx->state);
            
            // Parse the complete CAPWAP message based on message type
            capwap_header_t header;
            if (capwap_parse_header(buffer, bytes_received, &header) == CAPWAP_SUCCESS) {
                logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                          "Parsed CAPWAP header: type=%d, wtp_ac_id=%d", header.type, header.wtp_ac_id);
                
                // Handle based on message type - Control messages
                if (header.type == 1) { // Control message
                    switch (header.wtp_ac_id) {
                        case CAPWAP_DISCOVERY_REQUEST:
                            logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
                                      "Discovery request received from client in state: %d", client_ctx->state);
                            handle_discovery_request(buffer, bytes_received, &client_addr);
                            // After discovery request, client should be in discovery state
                            update_client_state(client_ctx, CAPWAP_STATE_DISCOVERY);
                            break;
                        case CAPWAP_JOIN_REQUEST:
                            logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
                                      "Join request received from client in state: %d", client_ctx->state);
                            handle_join_request(buffer, bytes_received, &client_addr);
                            // After successful join, client should be in join state
                            update_client_state(client_ctx, CAPWAP_STATE_JOIN);
                            break;
                        case CAPWAP_ECHO_REQUEST:
                            logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
                                      "Echo request received from client in state: %d", client_ctx->state);
                            handle_echo_request(buffer, bytes_received, &client_addr);
                            // Echo can happen in any state, keep the same state
                            break;
                        case CAPWAP_CONFIG_STATUS_REQUEST:
                            logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
                                      "Config status request received from client in state: %d", client_ctx->state);
                            handle_config_status_request(buffer, bytes_received, &client_addr);
                            // Update state to config status after processing
                            update_client_state(client_ctx, CAPWAP_STATE_CONFIG_STATUS);
                            break;
                        case CAPWAP_CHANGE_STATE_EVENT_REQUEST:
                            logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
                                      "Change state event request received from client in state: %d", client_ctx->state);
                            handle_change_state_event_request(buffer, bytes_received, &client_addr);
                            // Update state to run after processing event
                            update_client_state(client_ctx, CAPWAP_STATE_RUN);
                            break;
                        default:
                            logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, 
                                      "Unknown control message type %d received from client in state: %d", 
                                      header.wtp_ac_id, client_ctx->state);
                            break;
                    }
                } else {
                    // Data message - not currently handled in this server
                    logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                              "Data message received from client (not handled directly)");
                }
            } else {
                logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to parse CAPWAP header");
            }
            
            logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                      "Client state after processing: %d", client_ctx->state);
        }
    }
    
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "CAPWAP server loop ended");
    return 0;
}

void capwap_server_shutdown(void) {
    if (g_app_ctx.server_socket >= 0) {
        close(g_app_ctx.server_socket);
        g_app_ctx.server_socket = -1;
        logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "CAPWAP server socket closed");
    }
}

void* capwap_server_thread(void *arg) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "CAPWAP server thread started");
    capwap_server_run();
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "CAPWAP server thread ended");
    return NULL;
}

int handle_discovery_request(uint8_t *buffer, size_t len, struct sockaddr_in *client_addr) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Handling discovery request");
    
    capwap_discovery_req_t discovery_req = {0};
    int result = capwap_parse_discovery_request(buffer, len, &discovery_req);
    
    if (result == CAPWAP_SUCCESS) {
        logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                  "Successfully parsed discovery request, discovery type: %d", discovery_req.discovery_type);
        
        // Send discovery response
        send_discovery_response(client_addr);
    } else {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to parse discovery request: %d", result);
    }
    
    return result;
}

int handle_join_request(uint8_t *buffer, size_t len, struct sockaddr_in *client_addr) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Handling join request");
    
    capwap_join_req_t join_req = {0};
    int result = capwap_parse_join_request(buffer, len, &join_req);
    
    if (result == CAPWAP_SUCCESS) {
        logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                  "Successfully parsed join request");
        
        // Send join response (success for testing purposes)
        send_join_response(client_addr, CAPWAP_RESULT_SUCCESS);
    } else {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to parse join request: %d", result);
        // Send join response with failure
        send_join_response(client_addr, CAPWAP_RESULT_FAILURE_RESOURCE_DEPLETION);
    }
    
    return result;
}

int handle_echo_request(uint8_t *buffer, size_t len, struct sockaddr_in *client_addr) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Handling echo request");
    
    capwap_echo_req_t echo_req = {0};
    int result = capwap_parse_echo_request(buffer, len, &echo_req);
    
    if (result == CAPWAP_SUCCESS) {
        logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                  "Successfully parsed echo request");
        
        // Send echo response
        send_echo_response(client_addr, CAPWAP_RESULT_SUCCESS);
    } else {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to parse echo request: %d", result);
        // Send echo response with failure
        send_echo_response(client_addr, CAPWAP_RESULT_FAILURE_UNSPECIFIED);
    }
    
    return result;
}

int handle_config_status_request(uint8_t *buffer, size_t len, struct sockaddr_in *client_addr) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Handling config status request");
    
    capwap_config_status_req_t config_req = {0};
    int result = capwap_parse_config_status_request(buffer, len, &config_req);
    
    if (result == CAPWAP_SUCCESS) {
        logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                  "Successfully parsed config status request");
        
        // Send config status response (success for testing purposes)
        send_config_status_response(client_addr, CAPWAP_RESULT_SUCCESS);
    } else {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to parse config status request: %d", result);
        // Send config status response with failure
        send_config_status_response(client_addr, CAPWAP_RESULT_FAILURE_UNSPECIFIED);
    }
    
    return result;
}

int handle_change_state_event_request(uint8_t *buffer, size_t len, struct sockaddr_in *client_addr) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Handling change state event request");
    
    capwap_change_state_event_req_t event_req = {0};
    int result = capwap_parse_change_state_event_request(buffer, len, &event_req);
    
    if (result == CAPWAP_SUCCESS) {
        logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                  "Successfully parsed change state event request");
        
        // Send change state event response (success for testing purposes)
        send_change_state_event_response(client_addr, CAPWAP_RESULT_SUCCESS);
    } else {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to parse change state event request: %d", result);
        // Send change state event response with failure
        send_change_state_event_response(client_addr, CAPWAP_RESULT_FAILURE_UNSPECIFIED);
    }
    
    return result;
}

int send_discovery_response(struct sockaddr_in *client_addr) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Sending discovery response");
    
    uint8_t response_buffer[MAX_RESPONSE_BUFFER_SIZE];
    size_t response_len = sizeof(response_buffer);
    
    // Prepare discovery response according to RFC 5415
    capwap_discovery_resp_t response = {0};
    response.result_code = CAPWAP_RESULT_SUCCESS;
    response.ac_name = DEFAULT_AC_NAME;
    response.discovery_interval = DEFAULT_DISCOVERY_INTERVAL;
    response.echo_interval = DEFAULT_ECHO_INTERVAL;
    response.max_echo_interval = DEFAULT_MAX_ECHO_INTERVAL;
    response.discovery_count = DEFAULT_DISCOVERY_COUNT;
    
    // Build the discovery response message
    int result = capwap_build_discovery_response(response_buffer, &response_len, sizeof(response_buffer), &response);
    
    if (result == CAPWAP_SUCCESS) {
        // Send the response back to the client
        ssize_t sent = sendto(g_app_ctx.server_socket, response_buffer, response_len, 0,
                             (struct sockaddr*)client_addr, sizeof(struct sockaddr_in));
        
        if (sent == -1) {
            logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to send discovery response: %s", strerror(errno));
            return -1;
        } else {
            logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                      "Sent %zd bytes of discovery response", sent);
        }
    } else {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, 
                  "Failed to build discovery response: %d", result);
        return -1;
    }
    
    return CAPWAP_SUCCESS;
}

int send_join_response(struct sockaddr_in *client_addr, capwap_result_code_t result_code) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Sending join response, result: %d", result_code);
    
    uint8_t response_buffer[MAX_RESPONSE_BUFFER_SIZE];
    size_t response_len = sizeof(response_buffer);
    
    // Prepare join response according to RFC 5415
    capwap_join_resp_t response = {0};
    response.result_code = result_code;
    response.wtp_mac_type = 0;  // Standard MAC type
    
    // For successful joins, include session ID and AC descriptor
    if (result_code == CAPWAP_RESULT_SUCCESS) {
        // Create a simple AC descriptor (this would be more complex in a real implementation)
        static uint8_t dummy_session_id[DEFAULT_SESSION_ID_LENGTH] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 
                                                                      0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
        response.ac_descriptor = malloc(AC_DESCRIPTOR_BUFFER_SIZE);
        if (response.ac_descriptor) {
            response.ac_descriptor_len = AC_DESCRIPTOR_BUFFER_SIZE;
            // Fill with dummy data
            memset(response.ac_descriptor, 0, response.ac_descriptor_len);
            response.ac_descriptor[0] = 0x01;  // Basic descriptor structure
        }
        response.session_id = dummy_session_id;
    }
    
    // Build the join response message
    int result = capwap_build_join_response(response_buffer, &response_len, sizeof(response_buffer), &response);
    
    if (result == CAPWAP_SUCCESS) {
        // Send the response back to the client
        ssize_t sent = sendto(g_app_ctx.server_socket, response_buffer, response_len, 0,
                             (struct sockaddr*)client_addr, sizeof(struct sockaddr_in));
        
        if (sent == -1) {
            logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to send join response: %s", strerror(errno));
            result = -1;
        } else {
            logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                      "Sent %zd bytes of join response", sent);
        }
    } else {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, 
                  "Failed to build join response: %d", result);
    }
    
    // Clean up allocated memory
    if (response.ac_descriptor) {
        free(response.ac_descriptor);
    }
    
    return result;
}

int send_echo_response(struct sockaddr_in *client_addr, capwap_result_code_t result_code) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Sending echo response, result: %d", result_code);
    
    uint8_t response_buffer[MAX_SHORT_RESPONSE_BUFFER_SIZE];
    size_t response_len = sizeof(response_buffer);
    
    // Prepare echo response according to RFC 5415
    capwap_echo_resp_t response = {0};
    response.result_code = result_code;
    
    // Build the echo response message
    int result = capwap_build_echo_response(response_buffer, &response_len, sizeof(response_buffer), &response);
    
    if (result == CAPWAP_SUCCESS) {
        // Send the response back to the client
        ssize_t sent = sendto(g_app_ctx.server_socket, response_buffer, response_len, 0,
                             (struct sockaddr*)client_addr, sizeof(struct sockaddr_in));
        
        if (sent == -1) {
            logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to send echo response: %s", strerror(errno));
            result = -1;
        } else {
            logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                      "Sent %zd bytes of echo response", sent);
        }
    } else {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, 
                  "Failed to build echo response: %d", result);
    }
    
    return result;
}

int send_config_status_response(struct sockaddr_in *client_addr, capwap_result_code_t result_code) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Sending config status response, result: %d", result_code);
    
    uint8_t response_buffer[MAX_SHORT_RESPONSE_BUFFER_SIZE];
    size_t response_len = sizeof(response_buffer);
    
    // Prepare config status response according to RFC 5415
    capwap_config_status_resp_t response = {0};
    response.result_code = result_code;
    
    // Build the config status response message
    int result = capwap_build_config_status_response(response_buffer, &response_len, sizeof(response_buffer), &response);
    
    if (result == CAPWAP_SUCCESS) {
        // Send the response back to the client
        ssize_t sent = sendto(g_app_ctx.server_socket, response_buffer, response_len, 0,
                             (struct sockaddr*)client_addr, sizeof(struct sockaddr_in));
        
        if (sent == -1) {
            logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to send config status response: %s", strerror(errno));
            result = -1;
        } else {
            logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                      "Sent %zd bytes of config status response", sent);
        }
    } else {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, 
                  "Failed to build config status response: %d", result);
    }
    
    return result;
}

int send_change_state_event_response(struct sockaddr_in *client_addr, capwap_result_code_t result_code) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Sending change state event response, result: %d", result_code);
    
    uint8_t response_buffer[MAX_SHORT_RESPONSE_BUFFER_SIZE];
    size_t response_len = sizeof(response_buffer);
    
    // Prepare change state event response according to RFC 5415
    capwap_change_state_event_resp_t response = {0};
    response.result_code = result_code;
    
    // Build the change state event response message
    int result = capwap_build_change_state_event_response(response_buffer, &response_len, sizeof(response_buffer), &response);
    
    if (result == CAPWAP_SUCCESS) {
        // Send the response back to the client
        ssize_t sent = sendto(g_app_ctx.server_socket, response_buffer, response_len, 0,
                             (struct sockaddr*)client_addr, sizeof(struct sockaddr_in));
        
        if (sent == -1) {
            logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to send change state event response: %s", strerror(errno));
            result = -1;
        } else {
            logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                      "Sent %zd bytes of change state event response", sent);
        }
    } else {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, 
                  "Failed to build change state event response: %d", result);
    }
    
    return result;
}

int app_init(int argc, char *argv[]) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Initializing CAPWAP client tester");
    
    // Initialize application context
    memset(&g_app_ctx, 0, sizeof(g_app_ctx));
    g_app_ctx.state = APP_STATE_INITIALIZING;
    g_app_ctx.server_socket = -1;
    g_app_ctx.shutdown_requested = false;
    
    // Initialize mutex and condition variable
    if (pthread_mutex_init(&g_app_ctx.state_mutex, NULL) != 0) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to initialize mutex");
        return -1;
    }
    
    if (pthread_cond_init(&g_app_ctx.state_cond, NULL) != 0) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to initialize condition variable");
        pthread_mutex_destroy(&g_app_ctx.state_mutex);
        return -1;
    }
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize logging from config file
    logger_config_t log_config = {0};
    log_config.level = LOG_LEVEL_DEBUG;
    log_config.format = LOG_FORMAT_TEXT;
    log_config.file_path = strdup("cw-client-tester.log");
    log_config.max_file_size = DEFAULT_LOG_FILE_SIZE_LIMIT; // 10 MB
    log_config.console_output = true;
    
    // Try to read configuration from file
    int config_result = parse_config_file("config/cw-client-tester.conf", &log_config);
    if (config_result != 0) {
        logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, 
                  "Could not read config file, using defaults");
    } else {
        logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
                  "Configuration loaded from file");
    }
    
    if (logger_init(&log_config) != 0) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to initialize logger");
        return -1;
    }
    
    // Initialize CAPWAP server
    if (capwap_server_init() != 0) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to initialize CAPWAP server");
        return -1;
    }
    
    // Initialize test cases
    init_test_cases();
    
    g_app_ctx.state = APP_STATE_RUNNING;
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Application initialized successfully");
    
    return 0;
}

void app_run(void) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Starting application main loop");
    
    // Create server thread
    int rc = pthread_create(&g_app_ctx.server_thread, NULL, capwap_server_thread, NULL);
    if (rc != 0) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, "Failed to create server thread: %d", rc);
        return;
    }
    
    // Run test cases (this is where workflow summaries will be logged)
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Running registered test cases...");
    run_test_cases();
    
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Application running, waiting for shutdown signal...");
    
    // Wait for shutdown signal
    pthread_mutex_lock(&g_app_ctx.state_mutex);
    while (g_app_ctx.state == APP_STATE_RUNNING && !g_app_ctx.shutdown_requested) {
        pthread_cond_wait(&g_app_ctx.state_cond, &g_app_ctx.state_mutex);
    }
    pthread_mutex_unlock(&g_app_ctx.state_mutex);
    
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Shutdown signal received");
}

void app_shutdown(bool force) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Initiating shutdown (force: %s)", force ? "true" : "false");
    
    g_app_ctx.shutdown_requested = true;
    g_app_ctx.force_shutdown = force;
    
    if (g_app_ctx.state == APP_STATE_RUNNING) {
        // Wake up the main thread
        pthread_mutex_lock(&g_app_ctx.state_mutex);
        pthread_cond_signal(&g_app_ctx.state_cond);
        pthread_mutex_unlock(&g_app_ctx.state_mutex);
    }
    
    // If forced shutdown, cancel the server thread
    if (force && g_app_ctx.server_thread) {
        pthread_cancel(g_app_ctx.server_thread);
    }
    
    // Wait for server thread to finish (with timeout)
    if (g_app_ctx.server_thread) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 5; // 5 second timeout
        
        int rc = pthread_timedjoin_np(g_app_ctx.server_thread, NULL, &timeout);
        if (rc != 0) {
            logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, "Server thread did not finish in time, canceling");
            pthread_cancel(g_app_ctx.server_thread);
            pthread_join(g_app_ctx.server_thread, NULL);
        }
    }
    
    g_app_ctx.state = APP_STATE_STOPPED;
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Shutdown completed");
}

void app_cleanup(void) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Cleaning up application resources");
    
    // Clean up CAPWAP server
    capwap_server_shutdown();
    
    // Free test cases
    test_case_t *current = g_app_ctx.test_cases;
    while (current) {
        test_case_t *next = current->next;
        if (current->name) free(current->name);
        if (current->description) free(current->description);
        free(current);
        current = next;
    }
    
    // Free client contexts
    client_context_t *client = g_app_ctx.clients;
    while (client) {
        client_context_t *next = client->next;
        free(client);
        client = next;
    }
    
    // Destroy mutex and condition variable
    pthread_mutex_destroy(&g_app_ctx.state_mutex);
    pthread_cond_destroy(&g_app_ctx.state_cond);
    
    // Cleanup logger
    logger_cleanup();
    
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Application cleanup completed");
}

void signal_handler(int sig) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Received signal %d", sig);
    
    switch (sig) {
        case SIGINT:
        case SIGTERM:
            app_shutdown(false); // Graceful shutdown
            break;
        default:
            break;
    }
}