#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <iostream>
#include "helpers.h"
#include "requests.h"
#include "json.hpp"

using namespace nlohmann;
using namespace std;

#define ADDRESS "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"
#define REGISTER "/api/v1/tema/auth/register"
#define JSON "application/json"
#define LOGIN "/api/v1/tema/auth/login"
#define ACCESS "/api/v1/tema/library/access"
#define BOOKS "/api/v1/tema/library/books"
#define LOGOUT "/api/v1/tema/auth/logout"


char *message;
char *response;
char key[50];
string JWTtoken;
cookie cookies[10];
struct addrinfo hints, *result;
struct sockaddr_in* addr;
int main_server;
int nr_of_cookies = 0;
int ret, i, err;
bool is_logged_in = false;
bool gotJWT = false;

void dns_lookup() {
    hints.ai_family = AF_INET;
	hints.ai_socktype = 0;
	hints.ai_protocol = 0;
    hints.ai_flags = 0;

    err = getaddrinfo(ADDRESS, "http", &hints, &result);
    if (err != 0)
        printf("err: %d %s\n", err, gai_strerror(err));

    addr = (struct sockaddr_in*)(result->ai_addr);
}

bool is_error(const char* response) {
    return (response[9] != '2');
}

bool is_number(string s) {
    if (s.empty()) {
        return false;
    }

    char *p;
    strtol(s.c_str(), &p, 10);

    return (*p == 0);
}

json get_login_info() {
    json auth;
    string username, password;
    
    cout << "username=";
    getline(cin, username);

    cout << "password=";
    getline(cin, password);

    auth["username"] = username;
    auth["password"] = password;

    return auth;
}

int get_id() {
    int id;
    cout << "id=";
    cin >> id;
    return id;
}

json get_book_info() {
    json book;
    string title, author, genre, page_count, publisher;
    bool is_nr = false;

    cout << "title=";
    getline(cin, title);
    book["title"] = title;

    cout << "author=";
    getline(cin, author);
    book["author"] = author;

    cout << "genre=";
    getline(cin, genre);
    book["genre"] = genre;

    while (!is_nr) {
        cout << "page_count=";
        getline(cin, page_count);
        is_nr = is_number(page_count);
        if (!is_nr) {
            cout << "Please enter a valid number\n";
        }
    }
    book["page_count"] = page_count;

    cout << "publisher=";
    getline(cin, publisher);
    book["publisher"] = publisher;

    return book;
}

void extract_JWT(string response) {
    if (!is_error(((char*)response.c_str()))) {
        int pos = response.find("{");

        if (pos != -1) {
            json raspuns = json::parse(response.substr(pos, response.size()));
            JWTtoken = raspuns["token"];
            gotJWT = true;
        }
    }
}

string book_addr(int id) {
    return string(BOOKS) + "/" + to_string(id);
}

int open_server_connection() {
    return open_connection(inet_ntoa(addr->sin_addr), 8080, AF_INET, SOCK_STREAM, 0);
}

void print_error(const char* server_response) {
    string response_string(server_response);
    string error;
    json raspuns;
    int pos = response_string.find("{");

    if (is_error(server_response)) {
        cout << "Error! ";

        if (pos != -1) {
            raspuns = json::parse(response_string.substr(pos, response_string.size()));
            error = raspuns["error"];
            cout << error << "\n";
        } else {
            cout << "\n";
        }
    }
}

void print_json(string response) {
    if (is_error(((char*)response.c_str()))) {
        print_error(((char*)response.c_str()));
        return;
    }

    int pos = response.find("[");
    if (pos == -1) { // daca nu e lista de json
        pos = response.find("{");
        if (pos == -1) { // daca nu avem json deloc
            return;
        }
    }

    string substr = response.substr(pos, response.size());

    cout << "Json:\n";
    json res = json::parse(substr);
    cout << res.dump(3) << "\n";
}

void register_acc() {
    json auth = get_login_info();
    
    main_server = open_server_connection();

    message = compute_post_request(ADDRESS, REGISTER, JSON, auth, JWTtoken,
                                   false, cookies, nr_of_cookies);

    send_to_server(main_server, message);
    printf("sent: %s\n", message);

    response = receive_from_server(main_server);
    printf("recieved:\n%s\n\n", response);

    print_json(string(response));

    close(main_server);
}

void auth_acc() {
    if (is_logged_in) {
        cout << "Log out first!\n";
        return;
    }

    json auth = get_login_info();
    
    main_server = open_server_connection();

    message = compute_post_request(ADDRESS, LOGIN, JSON, auth, JWTtoken, 
                                   false, cookies, nr_of_cookies);

    send_to_server(main_server, message);
    printf("sent: %s\n", message);

    response = receive_from_server(main_server);
    printf("recieved:\n%s\n\n", response);

    print_json(string(response));

    if (!is_error(response)) {
        is_logged_in = true;
    }

    ret = extract_cookie(response, &(cookies[nr_of_cookies]));
    if (ret >= 0) {
        nr_of_cookies++;
        cout << print_cookie(cookies[0]) << "\n";
    }

    close(main_server);
}

void enter_library() {
    main_server = open_server_connection();

    message = compute_get_request(ADDRESS, ACCESS, NULL, JWTtoken, false,
                                  cookies, nr_of_cookies);

    send_to_server(main_server, message);
    printf("sent: %s\n", message);

    response = receive_from_server(main_server);
    printf("recieved:\n%s\n\n", response);

    extract_JWT(string(response));

    print_json(string(response));

    close(main_server);
}

void get_books() {
    main_server = open_server_connection();

    message = compute_get_request(ADDRESS, BOOKS, NULL, JWTtoken, gotJWT, 
                                  cookies, nr_of_cookies);

    send_to_server(main_server, message);
    printf("sent: %s\n", message);

    response = receive_from_server(main_server);
    printf("recieved:\n%s\n\n", response);

    print_json(string(response));

    close(main_server);
}

void get_book() {
    main_server = open_server_connection();

    message = compute_get_request(ADDRESS, book_addr(get_id()), NULL, 
                                  JWTtoken, gotJWT, cookies, nr_of_cookies);

    send_to_server(main_server, message);
    printf("sent: %s\n", message);

    response = receive_from_server(main_server);
    printf("recieved:\n%s\n\n", response);

    print_json(string(response));

    close(main_server);
}

void add_book() {
    json book = get_book_info();
    
    main_server = open_server_connection();

    message = compute_post_request(ADDRESS, BOOKS, JSON, book, JWTtoken, 
                                   gotJWT, cookies, nr_of_cookies);

    send_to_server(main_server, message);
    printf("sent: %s\n", message);

    response = receive_from_server(main_server);
    printf("recieved:\n%s\n\n", response);

    print_json(string(response));

    close(main_server);
}

void delete_book() {
    main_server = open_server_connection();

    message = compute_delete_request(ADDRESS, book_addr(get_id()), NULL, 
                                     JWTtoken, gotJWT, cookies, nr_of_cookies);

    send_to_server(main_server, message);
    printf("sent: %s\n", message);

    response = receive_from_server(main_server);
    printf("recieved:\n%s\n\n", response);

    print_json(string(response));

    close(main_server);
}

void logout() {
    main_server = open_server_connection();

    message = compute_get_request(ADDRESS, LOGOUT, NULL, JWTtoken, gotJWT,
                                  cookies, nr_of_cookies);

    send_to_server(main_server, message);
    printf("sent: %s\n", message);

    response = receive_from_server(main_server);
    printf("recieved:\n%s\n\n", response);

    print_json(string(response));

    if (!is_error(response)) {
        is_logged_in = false;
        gotJWT = false;
        nr_of_cookies--;
    }

    close(main_server);
}

int main() {
    char buffer[50];

    dns_lookup();

    while (1) {
        cin >> buffer;
        cin.ignore();

        if (!strcmp(buffer, "exit")) {
            break;
        }

        if (!strcmp(buffer, "register")) {
            register_acc();
            continue;
        }

        if (!strcmp(buffer, "login")) {
            auth_acc();
            continue;
        }

        if (!strcmp(buffer, "enter_library")) {
            enter_library();
            continue;
        }

        if (!strcmp(buffer, "get_books")) {
            get_books();
            continue;
        }

        if (!strcmp(buffer, "get_book")) {
            get_book();
            continue;
        }

        if (!strcmp(buffer, "add_book")) {
            add_book();
            continue;
        }

        if (!strcmp(buffer, "delete_book")) {
            delete_book();
            continue;
        }

        if (!strcmp(buffer, "logout")) {
            logout();
            continue;
        }

        cout << "Unrecognized command! Try again and type it with more love.\n";
    }
    
    return 0;
}
