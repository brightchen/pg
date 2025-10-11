#ifndef CW_DTLS_H
#define CW_DTLS_H

#include <stdint.h>
#include <stddef.h>
#include <openssl/ssl.h>

/* Error Codes */
#define CW_DTLS_SUCCESS                0
#define CW_DTLS_ERROR_INVALID_PARAM   -1
#define CW_DTLS_ERROR_SSL_INIT        -2
#define CW_DTLS_ERROR_CERT_LOAD       -3
#define CW_DTLS_ERROR_KEY_LOAD        -4
#define CW_DTLS_ERROR_CONNECTION      -5
#define CW_DTLS_ERROR_HANDSHAKE       -6
#define CW_DTLS_ERROR_READ            -7
#define CW_DTLS_ERROR_WRITE           -8
#define CW_DTLS_ERROR_MEMORY          -9

/* DTLS Role */
#define CW_DTLS_ROLE_CLIENT 0
#define CW_DTLS_ROLE_SERVER 1

/* Certificate Paths */
#define CW_DTLS_MAX_PATH_LEN 256

/* DTLS Configuration */
typedef struct {
    char cert_file[CW_DTLS_MAX_PATH_LEN];
    char key_file[CW_DTLS_MAX_PATH_LEN];
    char ca_file[CW_DTLS_MAX_PATH_LEN];
    int role;  /* CW_DTLS_ROLE_CLIENT or CW_DTLS_ROLE_SERVER */
    int verify_peer;  /* 1 to verify peer certificate, 0 otherwise */
} cw_dtls_config_t;

/* DTLS Context */
typedef struct {
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    BIO *bio;
    cw_dtls_config_t config;
    int initialized;
} cw_dtls_ctx_t;

/**
 * Initialize DTLS library (call once at program start)
 * @return CW_DTLS_SUCCESS on success, error code otherwise
 */
int cw_dtls_library_init(void);

/**
 * Cleanup DTLS library (call once at program end)
 */
void cw_dtls_library_cleanup(void);

/**
 * Create DTLS context with configuration
 * @param config Pointer to configuration
 * @return Pointer to DTLS context, or NULL on error
 */
cw_dtls_ctx_t* cw_dtls_create_context(const cw_dtls_config_t *config);

/**
 * Destroy DTLS context
 * @param ctx Pointer to DTLS context
 */
void cw_dtls_destroy_context(cw_dtls_ctx_t *ctx);

/**
 * Set socket file descriptor for DTLS context
 * @param ctx Pointer to DTLS context
 * @param sockfd Socket file descriptor
 * @return CW_DTLS_SUCCESS on success, error code otherwise
 */
int cw_dtls_set_socket(cw_dtls_ctx_t *ctx, int sockfd);

/**
 * Perform DTLS handshake
 * @param ctx Pointer to DTLS context
 * @return CW_DTLS_SUCCESS on success, error code otherwise
 */
int cw_dtls_handshake(cw_dtls_ctx_t *ctx);

/**
 * Read encrypted data and decrypt
 * @param ctx Pointer to DTLS context
 * @param buffer Output buffer for decrypted data
 * @param length Buffer size
 * @return Number of bytes read, or negative error code
 */
int cw_dtls_read(cw_dtls_ctx_t *ctx, uint8_t *buffer, size_t length);

/**
 * Encrypt and write data
 * @param ctx Pointer to DTLS context
 * @param buffer Input buffer with data to encrypt
 * @param length Data length
 * @return Number of bytes written, or negative error code
 */
int cw_dtls_write(cw_dtls_ctx_t *ctx, const uint8_t *buffer, size_t length);

/**
 * Check if DTLS handshake is complete
 * @param ctx Pointer to DTLS context
 * @return 1 if handshake complete, 0 otherwise
 */
int cw_dtls_is_handshake_complete(cw_dtls_ctx_t *ctx);

/**
 * Get last SSL error string
 * @return Error string
 */
const char* cw_dtls_get_error_string(void);

/**
 * Get error code string
 * @param error_code Error code value
 * @return String representation of error code
 */
const char* cw_dtls_error_code_string(int error_code);

#endif /* CW_DTLS_H */
