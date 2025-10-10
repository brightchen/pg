#include "cw-client-tester.h"

// Simple configuration parser (for demonstration)
int parse_config_file(const char *config_path, logger_config_t *log_config) {
    FILE *file = fopen(config_path, "r");
    if (!file) {
        logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, "Could not open config file: %s", config_path);
        return -1;
    }
    
    #include "cw-client-tester.h"

char line[MAX_CONFIG_LINE_LENGTH];
    char section[MAX_SECTION_NAME_LENGTH] = "";
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        
        // Check if it's a section header [section]
        if (line[0] == '[' && line[strlen(line)-1] == ']') {
            line[strlen(line)-1] = '\0';
            strncpy(section, line + 1, sizeof(section) - 1);
            section[sizeof(section) - 1] = '\0';
            continue;
        }
        
        // Parse key=value pairs
        char *eq_pos = strchr(line, '=');
        if (eq_pos) {
            *eq_pos = '\0';
            char *key = line;
            char *value = eq_pos + 1;
            
            // Trim whitespace
            while (*key == ' ' || *key == '\t') key++;
            while (*value == ' ' || *value == '\t') value++;
            
            // Process logging configuration
            if (strcmp(section, "logging") == 0) {
                if (strcmp(key, "level") == 0) {
                    if (strcmp(value, "TRACE") == 0) log_config->level = LOG_LEVEL_TRACE;
                    else if (strcmp(value, "DEBUG") == 0) log_config->level = LOG_LEVEL_DEBUG;
                    else if (strcmp(value, "INFO") == 0) log_config->level = LOG_LEVEL_INFO;
                    else if (strcmp(value, "WARN") == 0) log_config->level = LOG_LEVEL_WARN;
                    else if (strcmp(value, "ERROR") == 0) log_config->level = LOG_LEVEL_ERROR;
                }
                else if (strcmp(key, "format") == 0) {
                    if (strcmp(value, "TEXT") == 0) log_config->format = LOG_FORMAT_TEXT;
                    else if (strcmp(value, "CSV") == 0) log_config->format = LOG_FORMAT_CSV;
                    else if (strcmp(value, "JSON") == 0) log_config->format = LOG_FORMAT_JSON;
                }
                else if (strcmp(key, "file") == 0) {
                    if (log_config->file_path) free(log_config->file_path);
                    log_config->file_path = strdup(value);
                }
                else if (strcmp(key, "max_file_size") == 0) {
                    log_config->max_file_size = atoi(value);
                }
                else if (strcmp(key, "console_output") == 0) {
                    log_config->console_output = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
                }
            }
        }
    }
    
    fclose(file);
    return 0;
}