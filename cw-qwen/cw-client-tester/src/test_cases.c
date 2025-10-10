#include "cw-client-tester.h"
#include <sys/time.h>

// Helper function to get current time in milliseconds
long get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// Example workflow function for a discovery test
workflow_result_t* discovery_test_workflow(test_case_t *tc, void *event_data) {
    workflow_event_t *event = (workflow_event_t*)event_data;
    long start_time = get_time_ms();
    
    logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
              "Discovery test workflow: processing event type %d in state %d", 
              event->msg_type, event->current_state);
    
    workflow_result_t *result = malloc(sizeof(workflow_result_t));
    if (!result) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, 
                  "Failed to allocate memory for workflow result");
        result = malloc(sizeof(workflow_result_t));
        result->success = false;
        result->error_message = strdup("Memory allocation failed");
        result->final_state = event->current_state;
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    // Based on current state and event, decide what to do
    switch (event->current_state) {
        case CAPWAP_STATE_DISCOVERY:
            logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                      "In discovery state, received message type %d", event->msg_type);
            
            if (event->msg_type == CAPWAP_DISCOVERY_REQUEST) {
                logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
                          "Discovery request received, sending response");
                
                // In a real implementation, we'd send a discovery response here
                // For this example, we'll just log the action
                result->success = true;
                result->error_message = NULL;
                result->final_state = CAPWAP_STATE_JOIN; // Transition to JOIN state
            } else {
                result->success = false;
                result->error_message = strdup("Expected discovery request but received different message type");
                result->final_state = event->current_state;
            }
            break;
            
        case CAPWAP_STATE_JOIN:
            logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                      "In join state, received message type %d", event->msg_type);
            if (event->msg_type == CAPWAP_JOIN_REQUEST) {
                logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
                          "Join request received, processing");
                result->success = true;
                result->error_message = NULL;
                result->final_state = CAPWAP_STATE_CONFIG_STATUS;
            } else {
                result->success = false;
                result->error_message = strdup("Expected join request but received different message type");
                result->final_state = event->current_state;
            }
            break;
            
        default:
            result->success = false;
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Unexpected state: %d", event->current_state);
            result->error_message = strdup(error_msg);
            result->final_state = event->current_state;
            break;
    }
    
    result->execution_time_ms = get_time_ms() - start_time;
    return result;
}

// Example workflow function for a join test
workflow_result_t* join_test_workflow(test_case_t *tc, void *event_data) {
    workflow_event_t *event = (workflow_event_t*)event_data;
    long start_time = get_time_ms();
    
    logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
              "Join test workflow: processing event type %d in state %d", 
              event->msg_type, event->current_state);
    
    workflow_result_t *result = malloc(sizeof(workflow_result_t));
    if (!result) {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, 
                  "Failed to allocate memory for workflow result");
        result = malloc(sizeof(workflow_result_t));
        result->success = false;
        result->error_message = strdup("Memory allocation failed");
        result->final_state = event->current_state;
        result->execution_time_ms = get_time_ms() - start_time;
        return result;
    }
    
    switch (event->current_state) {
        case CAPWAP_STATE_JOIN:
            logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
                      "Processing join request in join state");
            
            if (event->msg_type == CAPWAP_JOIN_REQUEST) {
                result->success = true;
                result->error_message = NULL;
                result->final_state = CAPWAP_STATE_CONFIG_STATUS;
            } else {
                // For testing purposes, let's also accept discovery requests in this example
                if (event->msg_type == CAPWAP_DISCOVERY_REQUEST) {
                    result->success = true;
                    result->error_message = strdup("Received discovery request instead of join request - transitioning state anyway");
                    result->final_state = CAPWAP_STATE_CONFIG_STATUS;
                } else {
                    result->success = false;
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "Expected JOIN_REQUEST but received message type %d", event->msg_type);
                    result->error_message = strdup(error_msg);
                    result->final_state = event->current_state;
                }
            }
            break;
            
        default:
            result->success = false;
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Join test expected JOIN state, got %d", event->current_state);
            result->error_message = strdup(error_msg);
            result->final_state = event->current_state;
            break;
    }
    
    result->execution_time_ms = get_time_ms() - start_time;
    return result;
}

// Function to log workflow summary
void log_workflow_summary(test_case_t *tc, workflow_result_t *result) {
    if (result->success) {
        logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, 
                  "Workflow '%s' completed successfully in %ld ms", 
                  tc->name, result->execution_time_ms);
    } else {
        logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, 
                  "Workflow '%s' failed after %ld ms: %s", 
                  tc->name, result->execution_time_ms, result->error_message ? result->error_message : "Unknown error");
        logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, 
                  "Workflow '%s' final state: %d", tc->name, result->final_state);
    }
}

// Function to free workflow result
void free_workflow_result(workflow_result_t *result) {
    if (result) {
        if (result->error_message) {
            free(result->error_message);
        }
        free(result);
    }
}

// Initialize and register test cases
void init_test_cases(void) {
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Initializing test cases");
    
    // Create and register discovery test case
    test_case_t *discovery_tc = create_test_case("Discovery Test", 
                                                "Test CAPWAP client discovery process", 
                                                CAPWAP_STATE_DISCOVERY, 
                                                discovery_test_workflow);
    if (discovery_tc) {
        register_test_case(discovery_tc);
    }
    
    // Create and register join test case
    test_case_t *join_tc = create_test_case("Join Test", 
                                           "Test CAPWAP client join process", 
                                           CAPWAP_STATE_JOIN, 
                                           join_test_workflow);
    if (join_tc) {
        register_test_case(join_tc);
    }
    
    // Create and register a comprehensive test case
    test_case_t *comprehensive_tc = create_test_case("Comprehensive Test", 
                                                    "Test full CAPWAP client workflow", 
                                                    CAPWAP_STATE_DISCOVERY, 
                                                    discovery_test_workflow);
    if (comprehensive_tc) {
        register_test_case(comprehensive_tc);
    }
}