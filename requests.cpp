#include <stdlib.h> /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(const char *host, const char *url, const char *query_params, const char **cookies, int cookies_count)
{
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    char *line = (char *)calloc(LINELEN, sizeof(char));

    // Step 1: Write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL)
    {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    }
    else
    {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);

    // Step 2: Add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3: Add cookies if present
    if (cookies != NULL && cookies_count > 0)
    {
        strcpy(line, "Cookie: ");
        strcat(line, cookies[0]);
        compute_message(message, line);
    }

    // Add authorization header
    if (cookies != NULL && cookies_count > 1)
    {
        strcpy(line, "Authorization: Bearer ");
        strcat(line, cookies[1]);
        compute_message(message, line);
    }

    // Step 4: Add final new line
    strcat(message, "\r\n");

    free(line);
    return message;
}

char *compute_post_request(const char *host, const char *url, const char *content_type, const char *body_data,
                           int body_data_size, const char **cookies, int cookies_count)
{
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    char *line = (char *)calloc(LINELEN, sizeof(char));
    char *body_data_buffer = (char *)calloc(LINELEN, sizeof(char));

    // Step 1: Write the method name, URL, and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: Add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3: Add necessary headers
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    strcpy(body_data_buffer, body_data);
    body_data_buffer[body_data_size] = '\0';
    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);

    // Step 4: Add cookies if present
    if (cookies != NULL && cookies_count > 0)
    {
        strcpy(line, "Cookie: ");
        strcat(line, cookies[0]);
        compute_message(message, line);
    }

    // Add authorization header
    if (cookies != NULL && cookies_count > 1)
    {
        strcpy(line, "Authorization: Bearer ");
        strcat(line, cookies[1]);
        compute_message(message, line);
    }

    // Step 5: Add new line at end of header, then add payload data
    strcat(message, "\r\n");
    strcat(message, body_data_buffer);

    free(line);
    free(body_data_buffer);
    return message;
}

char *compute_delete_request(const char *host, const char *url, const char *content_type, const char **cookies, int cookies_count)
{
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    char *line = (char *)calloc(LINELEN, sizeof(char));

    // Step 1: Write the method name, URL, and protocol type
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: Add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Optional: Add Content-Type if necessary
    if (content_type != NULL && strlen(content_type) > 0)
    {
        sprintf(line, "Content-Type: %s", content_type);
        compute_message(message, line);
    }

    // Step 3: Add cookies if present
    if (cookies != NULL && cookies_count > 0)
    {
        strcpy(line, "Cookie: ");
        for (int i = 0; i < cookies_count; ++i)
        {
            strcat(line, cookies[i]);
            if (i < cookies_count - 1)
            {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    // Step 4: Add authorization header if present
    if (cookies != NULL && cookies_count > 1)
    {
        strcpy(line, "Authorization: Bearer ");
        strcat(line, cookies[1]); // Assuming cookies[1] is the JWT token
        compute_message(message, line);
    }

    // Step 5: Add a new line at the end of headers to indicate the start of the body (even if the body is empty)
    strcat(message, "\r\n");

    free(line);
    return message;
}
