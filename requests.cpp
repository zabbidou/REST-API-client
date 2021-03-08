#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <string>
#include <iostream>

#include "helpers.h"
#include "requests.h"
#include "json.hpp"

using namespace std;
using namespace nlohmann;

char *compute_get_request(string host_string, string url_string, char *query_params, 
                          string JWT, bool isJWT, cookie *cookies, 
                          int cookies_count) {
    char* host = (char*)host_string.c_str();
    char* url = (char*)url_string.c_str();

    char *message = (char*)calloc(BUFLEN, sizeof(char));
    char *line = (char*)calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookies_count != 0) {
       for (int i = 0; i < cookies_count; i++) {
           sprintf(line, "Cookie: %s", print_cookie(cookies[i]));
           compute_message(message, line);
       }
    }

    if (isJWT) {
        sprintf(line, "Authorization: Bearer %s", JWT.c_str());
        compute_message(message, line);
    }

    // Step 4: add final new line
    compute_message(message, "");
    return message;
}

char *compute_post_request(string host_string, string url_string, 
                           string content_type_string, json body_data, 
                           string JWT, bool isJWT, 
                           cookie *cookies, int cookies_count) {
    char* host = (char*)host_string.c_str();
    char* url = (char*)url_string.c_str();
    char* content_type = (char*)content_type_string.c_str();

    char *message = (char*)calloc(BUFLEN, sizeof(char));
    char *line = (char*)calloc(LINELEN, sizeof(char));
    char *body_data_buffer = (char*)calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    int size = body_data.dump().length();

    sprintf(line, "Content-Length: %d", size);
    compute_message(message, line);

    // Step 4 (optional): add cookies
    if (cookies != NULL) {
       for (int i = 0; i < cookies_count; i++) {
           sprintf(line, "Cookie: %s", print_cookie(cookies[i]));
           compute_message(message, line);
       }
    }

    if (isJWT) {
        sprintf(line, "Authorization: Bearer %s", JWT.c_str());
        compute_message(message, line);
    }

    // Step 5: add new line at end of header
    compute_message(message, "");

    // Step 6: add the actual payload data
    strcat(message, body_data.dump().c_str());

    free(line);
    return message;
}

char *compute_delete_request(string host_string, string url_string, char *query_params,
                             string JWT, bool isJWT, cookie *cookies,
                             int cookies_count) {
    char* host = (char*)host_string.c_str();
    char* url = (char*)url_string.c_str();

    char *message = (char*)calloc(BUFLEN, sizeof(char));
    char *line = (char*)calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    
    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookies_count != 0) {
       for (int i = 0; i < cookies_count; i++) {
           sprintf(line, "Cookie: %s", print_cookie(cookies[i]));
           compute_message(message, line);
       }
    }

    if (isJWT) {
        sprintf(line, "Authorization: Bearer %s", JWT.c_str());
        compute_message(message, line);
    }

    // Step 4: add final new line
    compute_message(message, "");
    return message;
}

int extract_cookie(char* msg, cookie* c) {
    bool found = 0;
    char* line = strtok(msg, "\n");
    while (line != NULL) {
        if (strstr(line, "Set-Cookie")) {
            found = 1;
            break;
        }
        line = strtok(NULL, "\n");
    }

    if (!found) {
        return -1;
    }

    line = line + 12;
    char* field = strtok(line, ";");
    char field_name[50];
    char field_value[100];
    int counter;
    int i;

    for (counter = 0; field[counter] != '='; counter++) { // parcurgem pana la =, adica numele field-ului
        (*c).name_field[counter] = field[counter];
    }

    (*c).name_field[counter] = '\0';
    counter++; // ignoram egalul

    for (i = 0; field[counter] != '\0'; counter++, i++) { // pana la finalul field-ului, aka valoarea (sper)
        (*c).name_value[i] = field[counter];
    }

    (*c).name_value[i] = '\0';

    field = strtok(NULL, ";");
    while (field != NULL) { // cat timp mai avem field-uri
        field++; // scoatem spatiul ramas de la strtok

        if (strchr(field, '=') != NULL) {
            for (counter = 0; field[counter] != '='; counter++) { // parcurgem pana la =, adica numele field-ului
                field_name[counter] = field[counter];
            }

            field_name[counter] = '\0';            
            counter++; // ignoram egalul
                                                  // fmm carriage return
            for (i = 0; field[counter] != '\0' && field[counter] != 13; counter++, i++) { // pana la finalul field-ului, aka valoarea (sper)
                field_value[i] = field[counter];
            }

            field_value[i] = '\0';

            if (!strcmp(field_name, "Path")) {
                strcpy((*c).path, field_value);
            }

        } else {
            for (int j = 0; field[j] != '\0'; j++) {
                if (field[j] == 13) { // fmm carriage return
                    field[j] = '\0';
                }
            }

            if (!strcmp(field, "HttpOnly")) {
                (*c).httponly = true;
            } else {
                (*c).httponly = false;
            }

            if (!strcmp(field, "Secure")) {
                (*c).secure = true;
            } else {
                (*c).secure = false;
            }
        }
        
        field = strtok(NULL, ";");
    }

    return 0;
}

char* print_cookie(cookie c) {
    char* line = (char*)calloc(1000, sizeof(char));

    sprintf(line, "%s=%s", c.name_field, c.name_value);

    if (strlen(c.path) != 0) {
        strcat(line, "; Path=");
        strcat(line, c.path);
    }

    if (c.httponly) {
        strcat(line, "; HttpOnly");
    }

    if (c.secure) {
        strcat(line, "; Secure");
        //printf("; Secure");
    }

    return line;
}
