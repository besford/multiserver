//This file contains common HTTP response headers
//and HTML pages. Specifically those for success and error codes.

const char* ok =
  "HTTP/1.1 200 OK\n";

const char* bad_req = 
  "HTTP/1.1 400 Bad Request\n"
  "Content-type: text/html\n"
  "\n"
  "<html>\n"
  " <body>\n"
  "  <h1>Bad Request</h1>\n"
  "  <p>This server did not understand your request.</p>\n"
  " </body>\n"
  "</html>\n";

const char* not_found = 
  "HTTP/1.1 404 Not Found\n"
  "Content-type: text/html\n"
  "\n"
  "<html>\n"
  " <body>\n"
  "  <h1>Not Found</h1>\n"
  "  <p>The requested URL was not found on this server.</p>\n"
  " </body>\n"
  "</html>\n";

const char* bad_method = 
  "HTTP/1.1 501 Method Not Implemented\n"
  "Content-type: text/html\n"
  "\n"
  "<html>\n"
  " <body>\n"
  "  <h1>Method Not Implemented</h1>\n"
  "  <p>Mmethod is not implemented by this server.</p>\n"
  " </body>\n"
  "</html>\n";
  
const char* unsupported_media =
  "HTTP/1.1 415 Unsupported Media Type\n"
  "Content-type: text/html\n"
  "\n"
  "<html>\n"
  " <body>\n"
  "  <h1>Unsupported Media</h1>\n"
  "  <p>Media type unsupported by this server.</p>\n"
  " </body>\n"
  "</html>\n";
  
const char* forbidden =   
  "HTTP/1.1 403 Forbidden Access\n"
  "Content-type: text/html\n"
  "\n"
  "<html>\n"
  " <body>\n"
  "   <h1>Forbidden</h1>\n"
  "   <p>You don't have permission to access x on this server.</p>\n"
  " </body>\n"
  "</html>\n";
