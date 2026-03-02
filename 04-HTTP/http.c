#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include "http.h"

//---------------------------------------------------------------------------------
// TODO:  Documentation
//
// Note that this module includes a number of helper functions to support this
// assignment.  YOU DO NOT NEED TO MODIFY ANY OF THIS CODE.  What you need to do
// is to appropriately document the socket_connect(), get_http_header_len(), and
// get_http_content_len() functions.
//
// NOTE:  I am not looking for a line-by-line set of comments.  I am looking for
//        a comment block at the top of each function that clearly highlights you
//        understanding about how the function works and that you researched the
//        function calls that I used.  You may (and likely should) add additional
//        comments within the function body itself highlighting key aspects of
//        what is going on.
//
// There is also an optional extra credit activity at the end of this function. If
// you partake, you need to rewrite the body of this function with a more optimal
// implementation. See the directions for this if you want to take on the extra
// credit.
//--------------------------------------------------------------------------------

char *strcasestr(const char *s, const char *find)
{
    char c, sc;
    size_t len;

    if ((c = *find++) != 0)
    {
        c = tolower((unsigned char)c);
        len = strlen(find);
        do
        {
            do
            {
                if ((sc = *s++) == 0)
                    return (NULL);
            } while ((char)tolower((unsigned char)sc) != c);
        } while (strncasecmp(s, find, len) != 0);
        s--;
    }
    return ((char *)s);
}

char *strnstr(const char *s, const char *find, size_t slen)
{
    char c, sc;
    size_t len;

    if ((c = *find++) != '\0')
    {
        len = strlen(find);
        do
        {
            do
            {
                if ((sc = *s++) == '\0' || slen-- < 1)
                    return (NULL);
            } while (sc != c);
            if (len > slen)
                return (NULL);
        } while (strncmp(s, find, len) != 0);
        s--;
    }
    return ((char *)s);
}

/*
 * socket_connect() - takes a hostname and establishes a TCP connection.
 *
 * Uses gethostbyname() to resolve the host, then creates a SOCK_STREAM
 * socket via socket(), and connects it to the server using connect().
 * The port is converted to network byte order with htons().
 *
 * Returns the socket file descriptor on success, or -1/-2 on failure.
 */

int socket_connect(const char *host, uint16_t port)
{
    struct hostent *hp;
    struct sockaddr_in addr;
    int sock;

    if ((hp = gethostbyname(host)) == NULL)
    {
        herror("gethostbyname");
        return -2;
    }

    bcopy(hp->h_addr_list[0], &addr.sin_addr, hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    sock = socket(PF_INET, SOCK_STREAM, 0);

    if (sock == -1)
    {
        perror("socket");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("connect");
        close(sock);
        return -1;
    }

    return sock;
}

/*
 * get_http_header_len() - returns the length of the HTTP response header.
 *
 * HTTP headers end with the sequence "\r\n\r\n". This function uses strnstr()
 * to find that sequence within the buffer, then calculates  header length
 * as the offset to that marker plus the length of the marker itself.
 *
 * Returns the header length in bytes, or -1 if the end marker isn't found.
 */

int get_http_header_len(char *http_buff, int http_buff_len)
{
    char *end_ptr;
    int header_len = 0;
    end_ptr = strnstr(http_buff, HTTP_HEADER_END, http_buff_len);

    if (end_ptr == NULL)
    {
        fprintf(stderr, "Could not find the end of the HTTP header\n");
        return -1;
    }

    header_len = (end_ptr - http_buff) + strlen(HTTP_HEADER_END);

    return header_len;
}

/*
 * get_http_content_len() - parses the HTTP header to find the Content-Length value.
 *
 * Iterates through the header buffer using sscanf() to extract
 * each line. Uses strcasestr() for a case insensitive search for the
 * Content-Length header, then finds the : delimiter with strchr() and
 * converts the value to an integer with atoi().
 *
 * Returns the content length, or 0 if no Content-Length header is found.
 */

int get_http_content_len(char *http_buff, int http_header_len)
{
    char header_line[MAX_HEADER_LINE];

    char *next_header_line = http_buff;
    char *end_header_buff = http_buff + http_header_len;

    while (next_header_line < end_header_buff)
    {
        bzero(header_line, sizeof(header_line));
        sscanf(next_header_line, "%[^\r\n]s", header_line);

        char *isCLHeader = strcasestr(header_line, CL_HEADER);
        if (isCLHeader != NULL)
        {
            char *header_value_start = strchr(header_line, HTTP_HEADER_DELIM);
            if (header_value_start != NULL)
            {
                char *header_value = header_value_start + 1;
                int content_len = atoi(header_value);
                return content_len;
            }
        }
        next_header_line += strlen(header_line) + strlen(HTTP_HEADER_EOL);
    }
    fprintf(stderr, "Did not find content length\n");
    return 0;
}

// This function just prints the header, it might be helpful for your debugging
// You dont need to document this or do anything with it, its self explanitory. :-)
void print_header(char *http_buff, int http_header_len)
{
    fprintf(stdout, "%.*s\n", http_header_len, http_buff);
}

//--------------------------------------------------------------------------------------
// EXTRA CREDIT - 10 pts - READ BELOW
//
// Implement a function that processes the header in one pass to figure out BOTH the
// header length and the content length.  I provided an implementation below just to
// highlight what I DONT WANT, in that we are making 2 passes over the buffer to determine
// the header and content length.
//
// To get extra credit, you must process the buffer ONCE getting both the header and content
// length.  Note that you are also free to change the function signature, or use the one I have
// that is passing both of the values back via pointers.  If you change the interface dont forget
// to change the signature in the http.h header file :-).  You also need to update client-ka.c to
// use this function to get full extra credit.
//--------------------------------------------------------------------------------------
int process_http_header(char *http_buff, int http_buff_len, int *header_len, int *content_len)
{
    *header_len = 0;
    *content_len = 0;

    char *next_line = http_buff;
    char *end_buff = http_buff + http_buff_len;
    char header_line[MAX_HEADER_LINE];

    while (next_line < end_buff)
    {
        bzero(header_line, sizeof(header_line));
        sscanf(next_line, "%[^\r\n]s", header_line);

        // check if blank line that ends the header ("\r\n\r\n")
        if (strlen(header_line) == 0)
        {
            // header ends here. account for final \r\n
            *header_len = (next_line - http_buff) + strlen(HTTP_HEADER_EOL);
            return 0;
        }

        // check if line is Content-Length header
        char *isCLHeader = strcasestr(header_line, CL_HEADER);
        if (isCLHeader != NULL)
        {
            char *delim = strchr(header_line, HTTP_HEADER_DELIM);
            if (delim != NULL)
            {
                *content_len = atoi(delim + 1);
            }
        }

        // advance to the next line
        next_line += strlen(header_line) + strlen(HTTP_HEADER_EOL);
    }

    // fail
    fprintf(stderr, "Could not find the end of the HTTP header\n");
    *header_len = 0;
    *content_len = 0;
    return -1;
}