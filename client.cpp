#include "json.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>

#include "helpers.h"
#include "requests.h"

#define PORT 8080
#define IP "34.246.184.49"
#define CONTENT_TYPE "application/json"
#define AUTH_ROUTE "/api/v1/tema/auth/"
#define LIBRARY_ACCESS_ROUTE "/api/v1/tema/library/access"
#define LIBRARY_BOOKS_ROUTE "/api/v1/tema/library/books"
#define LOGOUT_ROUTE "/api/v1/tema/auth/logout"

using namespace std;
using namespace nlohmann;

// Status codes for client commands
enum Command
{
    REGISTER,
    LOGIN,
    ENTER_LIBRARY,
    GET_BOOKS,
    GET_BOOK,
    ADD_BOOK,
    DELETE_BOOK,
    LOGOUT,
    EXIT,
    INVALID
};

// Check if a string is a number
bool is_number(const string &s)
{
    for (int i = 0; i < (int)s.size(); i++)
    {
        if (!isdigit(s[i]))
        {
            return false;
        }
    }
    return true;
}

// Function to free cookies
void free_cookies(char **cookies, int &cookies_size)
{
    for (int i = 0; i < cookies_size; i++)
    {
        free(cookies[i]);
        cookies[i] = NULL;
    }
    cookies_size = 0;
}

// Function to choose the command
char *choose_command(const string &host, int port, Command command, const char **cookies, int &cookies_size, map<string, any> data, bool &exit)
{
    string route = "";
    string payload = "";
    string content_type = CONTENT_TYPE;
    char *request = NULL;
    // Check the client command
    if (command == REGISTER || command == LOGIN)
    {
        // Create the payload for the request
        json j;
        j["username"] = any_cast<string>(data["username"]);
        j["password"] = any_cast<string>(data["password"]);
        payload = j.dump();
        // Set the route
        route = AUTH_ROUTE;
        route += (command == REGISTER) ? "register" : "login";
        // Compute the request
        request = compute_post_request(host.c_str(), route.c_str(), content_type.c_str(), payload.c_str(), payload.size(), NULL, 0);
    }
    else if (command == ENTER_LIBRARY)
    {
        // Set the route and compute the request
        route = LIBRARY_ACCESS_ROUTE;
        request = compute_get_request(host.c_str(), route.c_str(), NULL, cookies, cookies_size);
    }
    else if (command == GET_BOOKS || command == GET_BOOK)
    {
        // The same logic as above
        route = LIBRARY_BOOKS_ROUTE;
        // Add the book id to the route if it is needed
        route += (command == GET_BOOK) ? "/" + to_string(any_cast<int>(data["bookId"])) : "";
        request = compute_get_request(host.c_str(), route.c_str(), NULL, cookies, cookies_size);
    }
    else if (command == ADD_BOOK)
    {
        route = LIBRARY_BOOKS_ROUTE;
        json j;
        // Set the book data in the payload
        j["title"] = any_cast<string>(data["title"]);
        j["author"] = any_cast<string>(data["author"]);
        j["genre"] = any_cast<string>(data["genre"]);
        j["publisher"] = any_cast<string>(data["publisher"]);
        j["page_count"] = any_cast<int>(data["page_count"]);
        payload = j.dump();
        request = compute_post_request(host.c_str(), route.c_str(), content_type.c_str(), payload.c_str(), payload.size(), cookies, cookies_size);
    }
    else if (command == DELETE_BOOK)
    {
        route = LIBRARY_BOOKS_ROUTE;
        route += "/" + to_string(any_cast<int>(data["bookId"]));
        // Compute the delete request
        request = compute_delete_request(host.c_str(), route.c_str(), content_type.c_str(), cookies, cookies_size);
    }
    else if (command == LOGOUT)
    {
        route = LOGOUT_ROUTE;
        // Compute the logout request
        request = compute_get_request(host.c_str(), route.c_str(), NULL, cookies, cookies_size);
    }
    else if (command == EXIT)
    {
        // Exit
        exit = true;
    }
    return request;
}

// Function to print the response
void print_response(Command command, char *response, string &body_str, map<string, any> &data, char **cookies, int &cookies_size)
{
    json j;
    // Check for response
    if (command == REGISTER)
    {
        cout << "Utilizatorul a fost inregistrat cu succes!" << endl;
    }
    else if (command == LOGIN)
    {
        // Save the login token
        cout << "Utilizatorul s-a logat cu succes!" << endl;
        char *cookie = (char *)calloc(LINELEN, sizeof(char));
        char *cookie_line = strstr(response, "Set-Cookie: ");
        cookie_line += 12;
        cookie_line = strtok(cookie_line, "\r\n");
        strncpy(cookie, cookie_line, strlen(cookie_line));
        cookies[0] = cookie;
        cookies_size++;
    }
    else if (command == ENTER_LIBRARY)
    {
        j = json::parse(body_str);
        cout << "Utilizatorul are acces la biblioteca!" << std::endl;
        // Save the library token
        char *cookie = (char *)calloc(LINELEN, sizeof(char));
        string token = j["token"].dump();
        strncpy(cookie, token.c_str() + 1, token.size() - 2);
        cookie[strlen(cookie)] = '\0';
        cookies[1] = cookie;
        cookies_size++;
    }
    else if (command == GET_BOOKS || command == GET_BOOK)
    {
        json j = json::parse(body_str);
        cout << "\n"
             << j.dump(1) << endl;
    }
    else if (command == ADD_BOOK)
    {
        cout << "Cartea " << any_cast<string>(data["title"]) << " a fost adaugata cu succes!" << endl;
    }
    else if (command == DELETE_BOOK)
    {
        cout << "Cartea cu id " << any_cast<int>(data["bookId"]) << " a fost stearsa cu succes!" << endl;
    }
    else if (command == LOGOUT)
    {
        cout << "Utilizatorul s-a delogat cu succes!" << endl;
        free_cookies((char **)cookies, cookies_size);
    }
}

// Function to check for errors
bool check_for_errors(const string &code, const string &body)
{
    // Check if code is 4xx or 5xx
    if (code[0] == '4' || code[0] == '5')
    {
        json j = json::parse(body);
        string error = j["error"];
        cout << "Error. " << error << endl;
        return true;
    }
    return false;
}

// Close the connection and freE all ocuppied memory
void close_connection_and_free_data(int sockfd, char *request, char *response)
{
    // Close the connection and free the data
    free(request);
    free(response);
    close_connection(sockfd);
}

// Function to send a command to the server
void send_command(const string &host, int port, Command command, const char **cookies, int &cookies_size, map<string, any> data)

{
    // Open a connection to the server
    int sockfd = open_connection(const_cast<char *>(host.c_str()), port, AF_INET, SOCK_STREAM, 0);
    char *request = NULL;
    // Choose the command and create the request
    bool exit = false;
    request = choose_command(host, port, command, cookies, cookies_size, data, exit);
    if (exit)
    {
        close_connection(sockfd);
        return;
    }

    // Send request to server
    send_to_server(sockfd, request);
    char *response = receive_from_server(sockfd);
    char *body = basic_extract_payload(response);
    // Get the status code and the body of the response
    string code(get_status_code(response));
    string body_str(body);

    // Check for errors
    if (check_for_errors(code, body_str))
    {
        close_connection_and_free_data(sockfd, request, response);
        return;
    }
    else
        cout << "Succes. ";

    // Get the response
    print_response(command, response, body_str, data, (char **)cookies, cookies_size);

    // Clean up
    close_connection_and_free_data(sockfd, request, response);
}

// Function to check if the credentials are valid
bool invalid_credentials(const string &username, const string &password)
{
    return username.find(" ") != string::npos || password.find(" ") != string::npos;
}

// Read the credentials from the user
bool read_credentials(string &username, string &password)
{
    cout << "username=";
    getline(cin, username);
    cout << "password=";
    getline(cin, password);
    // If the credentials are empty or have spaces return false
    if (username.empty() || password.empty())
    {
        cout << "Error. Toate câmpurile sunt obligatorii!" << endl;
        return false;
    }
    if (invalid_credentials(username, password))
    {
        cout << "Error. Username si parola nu trebuie sa contina spatii!" << endl;
        return false;
    }
    return true;
}

// Read the book id from the user
int read_book_id()
{
    string bookId_str;
    cout << "id=";
    getline(cin, bookId_str);

    // Check if the book id is empty
    if (bookId_str.empty())
    {
        cout << "Error. Id-ul cărții este obligatoriu!" << endl;
        return -1;
    }

    // Check if the book id is a number
    bool is_num = is_number(bookId_str);
    if (is_num)
    {
        return stoi(bookId_str);
    }
    else
    {
        cout << "Error. Id-ul cărții trebuie să fie un număr!" << endl;
        return -1;
    }
}

// Read the book data from the user
bool read_book_data(string &title, string &author, string &genre, string &publisher, int &page_num)
{
    cout << "title=";
    getline(cin, title);
    cout << "author=";
    getline(cin, author);
    cout << "genre=";
    getline(cin, genre);
    cout << "publisher=";
    getline(cin, publisher);
    cout << "page_count=";
    string page_num_str;
    getline(cin, page_num_str);

    // Check if the fields are empty
    if (title.empty() || author.empty() || genre.empty() || publisher.empty() || page_num_str.empty())
    {
        cout << "Error. Toate câmpurile sunt obligatorii!" << endl;
        return false;
    }

    // Check if the page count is a number
    bool is_num = is_number(page_num_str);
    if (is_num)
    {
        page_num = stoi(page_num_str);
        // Pages must be positive
        if (page_num <= 0)
        {
            cout << "Error. Numărul de pagini trebuie să fie un număr pozitiv!" << endl;
            return false;
        }
    }
    else
    {
        cout << "Error. Numărul de pagini trebuie să fie un număr!" << endl;
        return false;
    }
    return true;
}

// Function to register a user
void register_user(const string &host, int port, char **cookies, int &cookies_size)
{
    string username, password;
    if (read_credentials(username, password))
        send_command(host, port, REGISTER, (const char **)cookies, cookies_size, {{"username", username}, {"password", password}});
}

// Function to login a user
void login_user(const string &host, int port, char **cookies, int &cookies_size)
{
    if (cookies[0] != NULL)
        cout << "Error. Utilizatorul este deja logat!" << endl;
    else
    {
        string username, password;
        if (read_credentials(username, password))
            send_command(host, port, LOGIN, (const char **)cookies, cookies_size, {{"username", username}, {"password", password}});
    }
}

// Function to enter the library
void enter_library(const string &host, int port, char **cookies, int &cookies_size)
{
    if (cookies[0] == NULL)
        cout << "Error. Utilizatorul nu este logat!" << endl;
    else
        send_command(host, port, ENTER_LIBRARY, (const char **)cookies, cookies_size, {});
}

// Get all the books from the library
void get_books(const string &host, int port, char **cookies, int &cookies_size)
{
    if (cookies[1] == NULL || cookies[0] == NULL)
        cout << "Error. Trebuie sa va logati si sa aveti acces la biblioteca pentru a vedea informatii despre carti!" << endl;
    else
        send_command(host, port, GET_BOOKS, (const char **)cookies, cookies_size, {});
}

// Get a book from the library by id
void get_book(const string &host, int port, char **cookies, int &cookies_size)
{
    if (cookies[1] == NULL || cookies[0] == NULL)
        cout << "Error. Trebuie sa va logati si sa aveti acces la biblioteca pentru a obtine informatie despre o carte!" << endl;
    else
    {
        int bookId = read_book_id();
        if (bookId != -1)
            send_command(host, port, GET_BOOK, (const char **)cookies, cookies_size, {{"bookId", bookId}});
    }
}

// Add a book to the library
void add_book(const string &host, int port, char **cookies, int &cookies_size)
{
    if (cookies[1] == NULL || cookies[0] == NULL)
    {
        cout << "Error. Trebuie sa va logati si sa aveti acces la biblioteca pentru a adauga o carte!" << endl;
    }
    else
    {
        int page_count = 0;
        string title, author, genre, publisher;
        if (read_book_data(title, author, genre, publisher, page_count))
        {
            map<string, any> data = {{"title", title}, {"author", author}, {"genre", genre}, {"publisher", publisher}, {"page_count", page_count}};
            if (page_count > 0)
                send_command(host, port, ADD_BOOK, (const char **)cookies, cookies_size, data);
        }
    }
}

// Delete a book from the library
void delete_book(const string &host, int port, char **cookies, int &cookies_size)
{
    if (cookies[1] == NULL || cookies[0] == NULL)
    {
        cout << "Error. Trebuie sa va logati si sa aveti acces la biblioteca pentru a sterge o carte!" << endl;
    }
    else
    {
        int bookId = read_book_id();
        if (bookId != -1)
            send_command(host, port, DELETE_BOOK, (const char **)cookies, cookies_size, {{"bookId", bookId}});
    }
}

// Logout the user
void logout_user(const string &host, int port, char **cookies, int &cookies_size)
{
    if (cookies[0] == NULL)
    {
        cout << "Error. Utilizatorul nu este logat!" << endl;
    }
    else
    {
        send_command(host, port, LOGOUT, (const char **)cookies, cookies_size, {});
        free_cookies(cookies, cookies_size);
    }
}

int main(int argc, char *argv[])
{
    // Host and port
    string host = IP;
    int port = PORT;
    // Cookies
    char **cookies = (char **)calloc(5, sizeof(char *));
    int cookies_count = 0;
    // Main loop
    while (true)
    {
        string command;
        // Get the command
        getline(cin, command);
        if (command == "register")
            register_user(host, port, cookies, cookies_count);
        else if (command == "login")
            login_user(host, port, cookies, cookies_count);
        else if (command == "enter_library")
            enter_library(host, port, cookies, cookies_count);
        else if (command == "get_books")
            get_books(host, port, cookies, cookies_count);
        else if (command == "get_book")
            get_book(host, port, cookies, cookies_count);
        else if (command == "add_book")
            add_book(host, port, cookies, cookies_count);
        else if (command == "delete_book")
            delete_book(host, port, cookies, cookies_count);
        else if (command == "logout")
            logout_user(host, port, cookies, cookies_count);
        else if (command == "exit")
        {
            free_cookies(cookies, cookies_count);
            free(cookies);
            return 0;
        }
        else
            cout << "Comanda invalida!" << endl;
    }
}