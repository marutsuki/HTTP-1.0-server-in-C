#include "../header/socket_worker_adaptor_http.h"
void *socket_worker_response_func_adaptor_http(long *response_len, char *req) {
    http_response *response = http_get(req);
    if (!response) {
        return NULL;
    }

    // Convert to char array
    char *raw_response = http_response_to_char_array(response, response_len);
    http_response_destructor(response);
    return raw_response;
}
char *http_response_to_char_array(http_response *response, long *response_len) {
    char *raw_response;
    long offset = 0;
    
    if (!response) {
        return NULL;
    }
    http_response_header *header = response->headers;

    /**
     * @brief Length of response 
     *  = length of all (headers + CRLF) + length of (status line + CRLF) + end of header indicator + body length
     */
    *response_len = http_response_length(response);
    
    raw_response = malloc(*response_len);
    if (!raw_response) {
        fprintf(stderr, "http.c - http_response_to_char_array() - malloc failed for raw_response\n");
        exit(ERROR_STATUS_CODE);
    }
    /**
     * @brief Get http status line
     */
    memcpy_offset_strlen_helper(&offset, raw_response, response->status_line);
    memcpy_offset_strlen_helper(&offset, raw_response, HTTP_HEADER_SEPARATOR);

    /**
     * @brief Get headers
     */
    header = response->headers;
    while (header != NULL) {
        memcpy_offset_strlen_helper(&offset, raw_response, header->name);
        memcpy_offset_strlen_helper(&offset, raw_response, HTTP_HEADER_KEY_VALUE_SEPARATOR);
        memcpy_offset_strlen_helper(&offset, raw_response, header->value);
        memcpy_offset_strlen_helper(&offset, raw_response, HTTP_HEADER_SEPARATOR);
        header = header->next;
    }
    memcpy(raw_response + offset, HTTP_HEADER_SEPARATOR, strlen(HTTP_HEADER_SEPARATOR));
    offset += strlen(HTTP_HEADER_SEPARATOR);
    if (response->body_len > 0) {
        memcpy(raw_response + offset, response->body, response->body_len);
    }
    return raw_response;
}
char *socket_worker_read_func_adaptor_http(int connfd) {
    long nbytes, bytes_read = 0, current_size = READ_BUFFER_SIZE;
    char *message = malloc(READ_BUFFER_SIZE);
    char buffer[READ_BUFFER_SIZE];
    char *end_of_header;

    if (!message) {
        fprintf(stderr, "http.c - http_read_adaptor() - malloc failed for char *message\n");
        exit(ERROR_STATUS_CODE);
    }
    while ((nbytes = recv(connfd, buffer, sizeof(buffer), 0)) != -1) {
        // If EOF, then stop reading
        if (nbytes == 0) {
            break;
        }
        if (bytes_read + nbytes > current_size) {
            message = realloc(message, current_size + READ_BUFFER_SIZE);

            if (!message) {
                fprintf(stderr, "http.c - http_read_adaptor() - realloc failed for *message\n");
                exit(ERROR_STATUS_CODE);
            }
            current_size += READ_BUFFER_SIZE;
        }
        // We use memcpy as strcpy depends on the '\0' character which may not be read
        memcpy(message + bytes_read, buffer, nbytes);
        bytes_read += nbytes;
        // Realloc memory in the case of the edge case where there is no additional space for the null terminator
        if (bytes_read == current_size) {
            message = realloc(message, current_size + 1);
        }
        // Note assigning this null terminator is important to prevent undefined behaviour when using strstr function
        message[bytes_read] = '\0';
        // Check for the end of header indicator as specified in RFC 2616
        if ((end_of_header = strstr(message, "\r\n\r\n")) != NULL || (end_of_header = strstr(message, "\n\n")) != NULL) {
            break;
        }
    }
    // If no bytes has been read, we need to add a null terminator
    if (bytes_read == 0) {
        message[0] = '\0';
    }
    return message;
}