# Digital Library REST Client (C++)

Developed a C++ client application that communicates with a **RESTful API** for a digital library system. The client:
1. Accepts **commands from the console** (e.g., `register`, `login`, `enter_library`, etc.).
2. Sends **HTTP requests** (GET, POST, DELETE) to a remote **server**.
3. Parses responses (JSON format), handles cookies, and displays **appropriate messages** to the user.

It demonstrates **core HTTP operations** (GET/POST/DELETE), usage of **JSON** (via [nlohmann/json.hpp](https://github.com/nlohmann/json)), and **session handling** (cookies, access tokens).

---

## Table of Contents
1. [Overview](#overview)  
2. [Features](#features)  
3. [Implementation Details](#implementation-details)  
4. [Commands](#commands)  

---

## Overview
This client:
- Connects to a **specific server** (IP and port configured in constants).
- Maintains **two tokens** in the form of cookies:
  1. **Session Cookie** – identifies a logged-in user.
  2. **Library Access Token** – grants access to book-related endpoints once inside the library.
- Reads **commands** from **stdin**, constructs an **HTTP request**, sends it, and interprets the response.

**Core technologies**:
- **C/C++** with sockets.
- **HTTP** request/response structure.
- **JSON** for request payloads and server responses.
- **Cookie** (string) management for session and library token.

---

## Features

1. **User Registration** (`register`)
   - Collects username and password from the user.
   - Submits a POST request to `/register`.
   - On success, prints a **success message**.  
   - On failure (username taken, invalid data), shows an **error** from the server.

2. **User Login** (`login`)
   - Collects username and password.
   - Submits a POST request to `/login`.
   - Stores **session token** returned in the **Set-Cookie** header (if successful).
   - Prints relevant **error** on invalid credentials or success message on correct credentials.

3. **Enter Library** (`enter_library`)
   - Requires a valid **session token**.
   - Sends a GET request to `/api/v1/tema/library/access`.
   - Stores a **library access token** (returned in JSON) in a second cookie field.

4. **Get All Books** (`get_books`)
   - Requires both **session token** and **library token**.
   - Retrieves the entire books list in JSON from the server.

5. **Get Book by ID** (`get_book`)
   - Prompts the user for a numeric book ID.
   - Sends a GET request to `.../books/<ID>`.
   - Prints the **book details** or an **error** if not found.

6. **Add Book** (`add_book`)
   - Prompts for book details: title, author, genre, publisher, page count.
   - Sends these in JSON via a POST request.
   - Prints a **success** message if the server adds the book, or **error** otherwise.

7. **Delete Book** (`delete_book`)
   - Requires library token.
   - Sends a DELETE request to `.../books/<ID>`.
   - Prints a **success** message on successful deletion or **error** otherwise.

8. **Logout** (`logout`)
   - Terminates session on the server by issuing a GET request to `/logout`.
   - Clears all stored cookies (both session & library token).

9. **Exit** (`exit`)
   - Frees resources and **closes** the client application.

---

## Implementation Details

1. **HTTP Requests**  
   - Built with helper functions (`compute_get_request`, `compute_post_request`, `compute_delete_request`) that format headers correctly.
   - Cookies are passed if available (session, library token).
   - **JSON** serialization uses `nlohmann::json`.

2. **Cookies & Session Management**  
   - Session cookie is set upon **login** in the `Set-Cookie` header and stored in `cookies[0]`.
   - Library access token is parsed from JSON response in **`enter_library`** and stored in `cookies[1]`.

3. **Error Handling**  
   - Checks **HTTP status code** (4xx or 5xx) from the server.
   - Extracts **`{"error": "..."} `** from the JSON body and displays it.

4. **Validation**  
   - Ensures **username** and **password** are non-empty and do not contain spaces.
   - Checks **book ID** is numeric.
   - Ensures **page_count** is a positive integer.

---

## Commands

Below are the **console commands** you can type:
- **`register`**: Prompt for credentials, register a new user.
- **`login`**: Prompt for credentials, log in existing user.
- **`enter_library`**: Obtain a library token if logged in.
- **`get_books`**: List all books (requires valid tokens).
- **`get_book`**: Prompt for a book ID and retrieve details.
- **`add_book`**: Prompt for book data (title, author, etc.) and add a new book.
- **`delete_book`**: Prompt for a book ID and remove that book.
- **`logout`**: Invalidate session and clear cookies.
- **`exit`**: Terminate the client application.
