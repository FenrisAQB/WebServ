#ifndef STATUSCODE_HPP
#define STATUSCODE_HPP

#define OK 200
#define CREATED 201
#define NO_CONTENT 204
#define AUTOINDEX 242

#define MOVED_PERMANENTLY 301
#define FOUND 302
#define TEMPORARY_REDIRECT 307
#define PERMANENT_REDIRECT 308

#define BAD_REQUEST 400
#define FORBIDDEN 403
#define NOT_FOUND 404
#define METHOD_NOT_ALLOWED 405
#define REQUEST_TIMEOUT 408
#define CONFLICT 409
#define LENGTH_REQUIRED 411
#define CONTENT_TOO_LARGE 413
#define IM_A_TEAPOT 418

#define INTERNAL_SERVER_ERROR 500
#define NOT_IMPLEMENTED 501
#define SERVICE_UNAVAILABLE 503
#define HTTP_VERSION_NOT_SUPPORTED 505
#define LOOP_DETECTED 508

#define MSG_200 "OK"
#define MSG_201 "Created"
#define MSG_204 "No Content"
#define MSG_242 "Autoindex"

#define MSG_301 "Moved Permanently"
#define MSG_302 "Found"
#define MSG_307 "Temporary Redirect"
#define MSG_308 "Permanent Redirect"

#define MSG_400 "Bad Request"
#define MSG_403 "Forbidden"
#define MSG_404 "Not Found"
#define MSG_405 "Method Not Allowed"
#define MSG_408 "Request Timeout"
#define MSG_409 "Conflict"
#define MSG_411 "Length Required"
#define MSG_413 "Payload Too Large"
#define MSG_418	"I'm A Teapot"

#define MSG_500 "Internal Server Error"
#define MSG_501 "Not Implemeted"
#define MSG_503 "Service Unavailable"
#define MSG_505 "HTTP Version Not Supported"
#define MSG_508 "Loop Detected"

#endif
