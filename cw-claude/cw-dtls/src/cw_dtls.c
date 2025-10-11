#include "cw_dtls.h"
#include <string.h>
#include <openssl/err.h>
#include <openssl/dtls1.h>

static int g_dtls_library_initialized = 0;

int cw_dtls_library_init(void) {
    if (g_dtls_library_initialized) {
        return CW_DTLS_SUCCESS;
    }

    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    g_dtls_library_initialized = 1;
    return CW_DTLS_SUCCESS;
}

void cw_dtls_library_cleanup(void) {
    if (!g_dtls_library_initialized) {
        return;
    }

    EVP_cleanup();
    ERR_free_strings();

    g_dtls_library_initialized = 0;
}

cw_dtls_ctx_t* cw_dtls_create_context(const cw_dtls_config_t *config) {
    if (config == NULL) {
        return NULL;
    }

    if (!g_dtls_library_initialized) {
        cw_dtls_library_init();
    }

    cw_dtls_ctx_t *ctx = (cw_dtls_ctx_t *)malloc(sizeof(cw_dtls_ctx_t));
    if (ctx == NULL) {
        return NULL;
    }

    memset(ctx, 0, sizeof(cw_dtls_ctx_t));
    memcpy(&ctx->config, config, sizeof(cw_dtls_config_t));

    /* Create SSL context */
    const SSL_METHOD *method;
    if (config->role == CW_DTLS_ROLE_CLIENT) {
        method = DTLS_client_method();
    } else {
        method = DTLS_server_method();
    }

    ctx->ssl_ctx = SSL_CTX_new(method);
    if (ctx->ssl_ctx == NULL) {
        free(ctx);
        return NULL;
    }

    /* Set verification mode */
    if (config->verify_peer) {
        SSL_CTX_set_verify(ctx->ssl_ctx, SSL_VERIFY_PEER, NULL);
    } else {
        SSL_CTX_set_verify(ctx->ssl_ctx, SSL_VERIFY_NONE, NULL);
    }

    /* Load CA certificate if provided */
    if (config->ca_file[0] != '\0') {
        if (SSL_CTX_load_verify_locations(ctx->ssl_ctx, config->ca_file, NULL) != 1) {
            SSL_CTX_free(ctx->ssl_ctx);
            free(ctx);
            return NULL;
        }
    }

    /* Load certificate if provided */
    if (config->cert_file[0] != '\0') {
        if (SSL_CTX_use_certificate_file(ctx->ssl_ctx, config->cert_file,
                                         SSL_FILETYPE_PEM) != 1) {
            SSL_CTX_free(ctx->ssl_ctx);
            free(ctx);
            return NULL;
        }
    }

    /* Load private key if provided */
    if (config->key_file[0] != '\0') {
        if (SSL_CTX_use_PrivateKey_file(ctx->ssl_ctx, config->key_file,
                                        SSL_FILETYPE_PEM) != 1) {
            SSL_CTX_free(ctx->ssl_ctx);
            free(ctx);
            return NULL;
        }

        /* Verify private key */
        if (SSL_CTX_check_private_key(ctx->ssl_ctx) != 1) {
            SSL_CTX_free(ctx->ssl_ctx);
            free(ctx);
            return NULL;
        }
    }

    /* Set cipher list */
    SSL_CTX_set_cipher_list(ctx->ssl_ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

    ctx->initialized = 1;
    return ctx;
}

void cw_dtls_destroy_context(cw_dtls_ctx_t *ctx) {
    if (ctx == NULL) {
        return;
    }

    if (ctx->ssl != NULL) {
        SSL_shutdown(ctx->ssl);
        SSL_free(ctx->ssl);
        ctx->ssl = NULL;
    }

    if (ctx->ssl_ctx != NULL) {
        SSL_CTX_free(ctx->ssl_ctx);
        ctx->ssl_ctx = NULL;
    }

    ctx->initialized = 0;
    free(ctx);
}

int cw_dtls_set_socket(cw_dtls_ctx_t *ctx, int sockfd) {
    if (ctx == NULL || !ctx->initialized || sockfd < 0) {
        return CW_DTLS_ERROR_INVALID_PARAM;
    }

    /* Create SSL object */
    ctx->ssl = SSL_new(ctx->ssl_ctx);
    if (ctx->ssl == NULL) {
        return CW_DTLS_ERROR_SSL_INIT;
    }

    /* Set socket file descriptor */
    if (SSL_set_fd(ctx->ssl, sockfd) != 1) {
        SSL_free(ctx->ssl);
        ctx->ssl = NULL;
        return CW_DTLS_ERROR_CONNECTION;
    }

    /* Set connect or accept state */
    if (ctx->config.role == CW_DTLS_ROLE_CLIENT) {
        SSL_set_connect_state(ctx->ssl);
    } else {
        SSL_set_accept_state(ctx->ssl);
    }

    return CW_DTLS_SUCCESS;
}

int cw_dtls_handshake(cw_dtls_ctx_t *ctx) {
    if (ctx == NULL || ctx->ssl == NULL) {
        return CW_DTLS_ERROR_INVALID_PARAM;
    }

    int ret;
    if (ctx->config.role == CW_DTLS_ROLE_CLIENT) {
        ret = SSL_connect(ctx->ssl);
    } else {
        ret = SSL_accept(ctx->ssl);
    }

    if (ret == 1) {
        return CW_DTLS_SUCCESS;
    }

    int error = SSL_get_error(ctx->ssl, ret);
    if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
        /* Handshake in progress, need more data */
        return CW_DTLS_ERROR_HANDSHAKE;
    }

    return CW_DTLS_ERROR_HANDSHAKE;
}

int cw_dtls_read(cw_dtls_ctx_t *ctx, uint8_t *buffer, size_t length) {
    if (ctx == NULL || ctx->ssl == NULL || buffer == NULL) {
        return CW_DTLS_ERROR_INVALID_PARAM;
    }

    int ret = SSL_read(ctx->ssl, buffer, (int)length);
    if (ret > 0) {
        return ret;
    }

    int error = SSL_get_error(ctx->ssl, ret);
    if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
        return 0; /* No data available yet */
    }

    return CW_DTLS_ERROR_READ;
}

int cw_dtls_write(cw_dtls_ctx_t *ctx, const uint8_t *buffer, size_t length) {
    if (ctx == NULL || ctx->ssl == NULL || buffer == NULL) {
        return CW_DTLS_ERROR_INVALID_PARAM;
    }

    int ret = SSL_write(ctx->ssl, buffer, (int)length);
    if (ret > 0) {
        return ret;
    }

    int error = SSL_get_error(ctx->ssl, ret);
    if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
        return 0; /* Try again later */
    }

    return CW_DTLS_ERROR_WRITE;
}

int cw_dtls_is_handshake_complete(cw_dtls_ctx_t *ctx) {
    if (ctx == NULL || ctx->ssl == NULL) {
        return 0;
    }

    return SSL_is_init_finished(ctx->ssl);
}

const char* cw_dtls_get_error_string(void) {
    unsigned long err = ERR_get_error();
    if (err == 0) {
        return "No error";
    }
    return ERR_error_string(err, NULL);
}

const char* cw_dtls_error_code_string(int error_code) {
    switch (error_code) {
        case CW_DTLS_SUCCESS: return "Success";
        case CW_DTLS_ERROR_INVALID_PARAM: return "Invalid parameter";
        case CW_DTLS_ERROR_SSL_INIT: return "SSL initialization error";
        case CW_DTLS_ERROR_CERT_LOAD: return "Certificate load error";
        case CW_DTLS_ERROR_KEY_LOAD: return "Private key load error";
        case CW_DTLS_ERROR_CONNECTION: return "Connection error";
        case CW_DTLS_ERROR_HANDSHAKE: return "Handshake error";
        case CW_DTLS_ERROR_READ: return "Read error";
        case CW_DTLS_ERROR_WRITE: return "Write error";
        case CW_DTLS_ERROR_MEMORY: return "Memory allocation error";
        default: return "Unknown error";
    }
}
