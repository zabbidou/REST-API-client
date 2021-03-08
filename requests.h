#ifndef _REQUESTS_
#define _REQUESTS_

#include "helpers.h"
#include <string>
#include "json.hpp"
using namespace std;

// computes and returns a GET request string (query_params
// and cookies can be set to NULL if not needed)
char *compute_get_request(string host, string url, char *query_params,
                          string JWT, bool isJWT,cookie *cookies, int cookies_count);

// computes and returns a POST request string (cookies can be NULL if not needed)
char *compute_post_request(string host, string url, string content_type, nlohmann::json body_data,
                           string JWT, bool isJWT, cookie* cookies, int cookies_count);

char *compute_delete_request(string host, string url, char *query_params,
                             string JWT, bool isJWT,cookie *cookies, int cookies_count);

int extract_cookie(char* msg, cookie* c);

char* print_cookie(cookie c);

#endif
