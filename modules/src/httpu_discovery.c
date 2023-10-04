/*
    2023 WireEye. Copyleft. No rights reserved.
    httpu_discovery.c - This file contains the primary execution stage/phase
    of BRIGHTSTAR's SSDP HTTPU Discovery module.
*/

#define _DEFAULT_SOURCE

#include "../../include/imports.h"
#include "../../include/fort.h"

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// take 'char *input'
// convert 'char *input' into 'wchar_t *input'
// convert 'wchar_t *input' to multibyte string (utf-8)

int httpu_m_search_setup(void);
int httpu_host_setup(char *hostname, char *udp_port);
int httpu_st_setup(char *search_target);
int httpu_mx_setup(char *ssdp_mx);
int httpu_man_setup(void);
int httpu_fmt_data_setup(void);
int set_ssdp_char_msg_elements(void);
int start_wchar_sz_conversion(void);
int start_wchar_allocation_opts(void);
int start_wchar_merge_main(void);
int set_ssdp_wchar_msg_elements(void);
int return_final_msg_len(void);
int transform_msg_into_utf_8(void);
int set_packet_sockt_opts(char *packetmx, char *socktopt);

int execute_httpu_module_main(char *ssdp_msa_addr, char *ssdp_msa_port, char msg[], size_t msg_sz, char *sockt_opt, char *packet_mx);
void httpu_module_sig_handler(int sigint);
int setup_multicast_scope_opts(void);
int setup_client_socket_opts(void);
int send_ssdp_discovery_msg(char msg[], size_t msg_sz);


static int UNICAST_RESPONSES = 0;
static int RESPONSES_WARRANTED = 0;

char *ssdp_char_msg[6];
wchar_t *ssdp_wchar_msg[6];

#define FMT_DATA "\r\n"
#define MSEARCH_STRING "M-SEARCH * HTTP/1.1"
#define MANDATORY_EXT "ssdp:discover"

#define PERFORM_CLEANUP_OPERATIONS free(m_search_post);\
                    free(host_post);\
                    free(st_post);\
                    free(mx_post);\
                    free(man_post);\
                    free(fmt_data_post);\
                    \
                    free(wc_msearch);\
                    free(wc_host);\
                    free(wc_st);\
                    free(wc_mx);\
                    free(wc_man);\
                    free(wc_fmt_data);\
                    free(packet_mx);\
                    free(sockt_opt);\

// +++++++++++++++++ START SIZE ALLOCATION DEFINITIONS +++++++++++++++++
static size_t fmt_sz = 0;
static size_t m_search_sz = 0;
static size_t host_sz = 0;
static size_t st_sz = 0;
static size_t mx_sz = 0;
static size_t man_sz = 0;

// final msg len
size_t len = 0;
size_t ssdp_wc_sz = 0;

static size_t wc_msearch_sz, wc_host_sz, wc_st_sz, wc_mx_sz, wc_man_sz, wc_fmt_data_sz;
// +++++++++++++++++ END SIZE ALLOCATION DEFINITIONS +++++++++++++++++

// +++++++++++++++++ START GENERIC CHAR DEFINITIONS +++++++++++++++++
static char *host = "";
static char *port = "";
static char *st = "";
static char *mx = "";

static char *separator = "";
static char *host_ext = "";
static char *st_ext = "";
static char *mx_ext = "";

static char *packet_mx, *sockt_opt;

static char *m_search_post, *host_post, *st_post, *mx_post, *man_post, *fmt_data_post;
static wchar_t *wc_msearch, *wc_host, *wc_st, *wc_mx, *wc_man, *wc_fmt_data;
// +++++++++++++++++ END GENERIC CHAR DEFINITIONS +++++++++++++++++

// client (root control point) socket
int client_multicast_sock = 0;

// integral variable used for limiting the amount of packets received on the
// interface in which the socket is used to send/recieve data

// set to 0 by default
int REPLY_COUNT_LIMIT = 0;

// remote structure for MSA
struct sockaddr_in scope_multicast_addr_opts;

int multicast_scope_port;
char *multicast_scope_addr;


// setup M-SEARCH string
int httpu_m_search_setup(void)
{
    m_search_post = (char *) malloc(m_search_sz * sizeof(char));
    
    if (m_search_post == NULL)
    {
        printf("failed to allocate (char) space for: m-search\n");

        return -1;
    }

    int m_search_ret = 0;

    if ((m_search_ret = snprintf(m_search_post, m_search_sz, "%s%s", MSEARCH_STRING, FMT_DATA)) < 0)
    {
        printf("failed to convert the M-SEARCH string\n");
        free(m_search_post);

        return -1;
    }

    return 0;
}


// setup Host string
int httpu_host_setup(char *hostname, char *udp_port)
{
    host = hostname;
    port = udp_port;

    host_ext = "Host:";
    separator = ":";

    host_sz = strlen(host_ext) + strlen(host) + strlen(separator) + strlen(port) + fmt_sz + 1; // "Host:<host>:<port>\r\n" + '\0'
    host_post = (char *) malloc(host_sz * sizeof(char));

    if (host_post == NULL)
    {
        printf("failed to allocate (char) space for: %s\n", host);

        return -1;
    }

    int host_ret = 0;

    if ((host_ret = snprintf(host_post, host_sz, "%s%s%s%s%s", host_ext, hostname, separator, udp_port, FMT_DATA)) < 0)
    {
        printf("failed to convert the Host string\n");
        free(host_post);

        return -1;
    }

    return 0;
}


// setup ST string
int httpu_st_setup(char *search_target)
{
    st = search_target; // search target
    st_ext = "ST:";

    st_sz = strlen(st_ext) + strlen(st) + fmt_sz + 1; // "ST:<st>\r\n" + '\0'
    st_post = (char *) malloc(st_sz * sizeof(char));

    if (st_post == NULL)
    {
        printf("failed to allocate (char) space for: %s\n", st);

        return -1;
    }

    int st_ret = 0;

    if ((st_ret = snprintf(st_post, st_sz, "%s%s%s", st_ext, st, FMT_DATA)) < 0)
    {
        printf("failed to convert the ST string\n");
        free(st_post);

        return -1;
    }

    return 0;
}


// setup MX string
int httpu_mx_setup(char *ssdp_mx)
{
    // going to assume this is used to prevent flooding the root control point
    // since preferably all devices listening on the multicast domain SHALL respond
    // with the appropriate HTTP response (mx = delay before responding to HTTPU (HTTP UDP) M-SEARCH@MSA:1900)
    mx = ssdp_mx;
    mx_ext = "MX:";

    mx_sz = strlen(mx_ext) + strlen(mx) + strlen(FMT_DATA) + 1; // "MX:<mx>\r\n" + '\0'

    mx_post = (char *) malloc(mx_sz * sizeof(char));

    if (mx_post == NULL)
    {
        printf("failed to allocate (char) space for: %s\n", mx);

        return -1;
    }

    int mx_ret = 0;

    if ((mx_ret = snprintf(mx_post, mx_sz, "%s%s%s", mx_ext, mx, FMT_DATA)) < 0)
    {
        printf("failed to convert MX target\n");
        free(mx_post);

        return -1;
    }

    return 0;
}


// setup MAN string
int httpu_man_setup(void)
{
    char *man = MANDATORY_EXT; // mandatory extension
    char *man_ext = "MAN:";
    char *esc = "\"";

    man_sz = strlen(man_ext) + strlen(esc) + strlen(man) + strlen(esc) + strlen(FMT_DATA) + 1; // "MAN:"<man>"\r\n" + '\0'

    man_post = (char *) malloc(man_sz * sizeof(char));

    if (man_post == NULL)
    {
        printf("failed to allocate (char) space for: %s\n", man);

        return -1;
    }

    int man_ret = 0;

    if ((man_ret = snprintf(man_post, man_sz, "%s%s%s%s%s", man_ext, esc, man, esc, FMT_DATA)) < 0)
    {
        printf("failed to convert MAN string\n");
        free(man_post);

        return -1;
    }

    return 0;
}


// setup fmt_data string
int httpu_fmt_data_setup(void)
{
    fmt_data_post = (char *) malloc(fmt_sz * sizeof(char));

    if (fmt_data_post == NULL)
    {
        printf("failed to allocate (char) space for: \\r\\n\n");

        return -1;
    }

    int fmt_data_ret = 0;

    if ((fmt_data_ret = snprintf(fmt_data_post, fmt_sz, "%s", FMT_DATA)) < 0)
    {
        printf("failed to convert: \"\\r\\n\" targets\n");
        free(fmt_data_post);

        return -1;
    }

    return 0;
}


// copy generic char targets to char array 'ssdp_char_msg'
int set_ssdp_char_msg_elements(void)
{
    size_t i = 0;

    goto STRDUP_MSEARCH;

    STRDUP_MSEARCH:
    {
        ssdp_char_msg[i] = strdup(m_search_post);
        i++;
    }

    goto STRDUP_HOST;

    STRDUP_HOST:
    {
        ssdp_char_msg[i] = strdup(host_post);
        i++;
    }

    goto STRDUP_ST;

    STRDUP_ST:
    {
        ssdp_char_msg[i] = strdup(st_post);
        i++;
    }

    goto STRDUP_MX;

    STRDUP_MX:
    {
        ssdp_char_msg[i] = strdup(mx_post);
        i++;
    }

    goto STRDUP_MAN;

    STRDUP_MAN:
    {
        ssdp_char_msg[i] = strdup(man_post);
        i++;
    }

    goto STRDUP_FMT_DATA;

    STRDUP_FMT_DATA:
    {
        ssdp_char_msg[i] = strdup(fmt_data_post);
    }

    return 0;
}


// allocate enough space for each char string to hold wide char string properly on conversion
int start_wchar_sz_conversion(void)
{   
    wc_msearch_sz = mbstowcs(NULL, m_search_post, 0);

    if (wc_msearch_sz == (size_t)-1)
    {
        printf("failed to obtain (wide char) size for: %s\n", m_search_post);
        free(m_search_post);

        return -1;
    }

    wc_host_sz = mbstowcs(NULL, host_post, 0);

    if (wc_host_sz == (size_t)-1)
    {
        printf("failed to obtain (wide char) size for: %s\n", host_post);
        free(host_post);

        return -1;
    }

    wc_st_sz = mbstowcs(NULL, st_post, 0);

    if (wc_st_sz == (size_t)-1)
    {
        printf("failed to obtain (wide char) size for: %s\n", st_post);
        free(st_post);

        return -1;
    }

    wc_mx_sz = mbstowcs(NULL, mx_post, 0);

    if (wc_mx_sz == (size_t)-1)
    {
        printf("failed to obtain (wide char) size for: %s\n", mx_post);
        free(mx_post);

        return -1;
    }

    wc_man_sz = mbstowcs(NULL, man_post, 0);

    if (wc_man_sz == (size_t)-1)
    {
        printf("failed to obtain (wide char) size for: %s\n", man_post);
        free(man_post);

        return -1;
    }

    wc_fmt_data_sz = mbstowcs(NULL, fmt_data_post, 0);

    if (wc_fmt_data_sz == (size_t)-1)
    {
        printf("failed to obtain (wide char) size for: %s\n", fmt_data_post);
        free(fmt_data_post);

        return -1;
    }

    return 0;
}


// actually allocate data (wchar_t (unsigned int)) to a block of memory internally when
// converting to wchar_t
int start_wchar_allocation_opts(void)
{
    wc_msearch = (wchar_t *) malloc((wc_msearch_sz + 1) * sizeof(wchar_t));

    if (wc_msearch == NULL)
    {
        printf("failed to allocate (wide char) space for: %s\n", m_search_post);
        free(m_search_post);

        return -1;
    }

    wc_host = (wchar_t *) malloc((wc_host_sz + 1) * sizeof(wchar_t));

    if (wc_host == NULL)
    {
        printf("failed to allocate (wide char) space for: %s\n", host_post);
        free(host_post);

        return -1;
    }

    wc_st = (wchar_t *) malloc((wc_st_sz + 1) * sizeof(wchar_t));

    if (wc_st == NULL)
    {
        printf("failed to allocate (wide char) space for: %s\n", st_post);
        free(st_post);

        return -1;
    }

    wc_mx = (wchar_t *) malloc((wc_mx_sz + 1) * sizeof(wchar_t));

    if (wc_mx == NULL)
    {
        printf("failed to allocate (wide char) space for: %s\n", mx_post);
        free(mx_post);

        return -1;
    }

    wc_man = (wchar_t *) malloc((wc_man_sz + 1) * sizeof(wchar_t));

    if (wc_man == NULL)
    {
        printf("failed to allocate (wide char) space for: %s\n", man_post);
        free(man_post);

        return -1;
    }

    wc_fmt_data = (wchar_t *) malloc((wc_fmt_data_sz + 1) * sizeof(wchar_t));

    if (wc_fmt_data == NULL)
    {
        printf("failed to allocate (wide char) space for: %s\n", fmt_data_post);
        free(fmt_data_post);

        return -1;
    }

    return 0;
}


// convert char *string to wchar_t *string
int start_wchar_merge_main(void)
{
    // M-SEARCH
    if (mbstowcs(wc_msearch, m_search_post, wc_msearch_sz) == (size_t)-1)
    {
        printf("failed to convert (char) to wide char: %s\n", m_search_post);

        free(m_search_post);
        free(wc_msearch);

        return -1;
    }

    // HOST
    if (mbstowcs(wc_host, host_post, wc_host_sz) == (size_t)-1)
    {
        printf("failed to convert (char) to wide char: %s\n", host_post);

        free(host_post);
        free(wc_host);

        return -1;
    }

    // SEARCH TARGET
    if (mbstowcs(wc_st, st_post, wc_st_sz) == (size_t)-1)
    {
        printf("failed to convert (char) to wide char: %s\n", st_post);

        free(st_post);
        free(wc_st);

        return -1;
    }

    // MX
    if (mbstowcs(wc_mx, mx_post, wc_mx_sz) == (size_t)-1)
    {
        printf("failed to convert (char) to wide char: %s\n", mx_post);

        free(mx_post);
        free(wc_mx);

        return -1;
    }

    // MAN
    if (mbstowcs(wc_man, man_post, wc_man_sz) == (size_t)-1)
    {
        printf("failed to convert (char) to wide char: %s\n", man_post);

        free(man_post);
        free(wc_man);

        return -1;
    }

    // /r/n
    if (mbstowcs(wc_fmt_data, fmt_data_post, wc_fmt_data_sz) == (size_t)-1)
    {
        printf("failed to convert (char) to wide char: %s\n", fmt_data_post);

        free(fmt_data_post);
        free(wc_fmt_data);

        return -1;
    }

    return 0;
}


// copy wchar_t strings to wchar_t array of type 'unsigned int'
int set_ssdp_wchar_msg_elements(void)
{
    size_t i = 0;

    // copy wchar_t strings to wchar_t array
    goto WCSCPY_MSEARCH; 

    WCSCPY_MSEARCH:
    {
        ssdp_wchar_msg[i] = wc_msearch;
        i++;
    }

    goto WCSCPY_HOST;

    WCSCPY_HOST:
    {
        ssdp_wchar_msg[i] = wc_host;
        i++;
    }

    goto WCSCPY_ST;

    WCSCPY_ST:
    {
        ssdp_wchar_msg[i] = wc_st;
        i++;
    }

    goto WCSCPY_MX;

    WCSCPY_MX:
    {
        ssdp_wchar_msg[i] = wc_mx;
        i++;
    }

    goto WCSCPY_MAN;

    WCSCPY_MAN:
    {
        ssdp_wchar_msg[i] = wc_man;
        i++;
    }

    goto WCSCPY_FMT_DATA;

    WCSCPY_FMT_DATA:
    {
        ssdp_wchar_msg[i] = wc_fmt_data;
    }

    return 0;
}


// take each element of the recently allocated/filled wchar_t array
// and get its equivalent length
int return_final_msg_len(void)
{
    len = 0;
    ssdp_wc_sz = sizeof(ssdp_wchar_msg) / sizeof(ssdp_wchar_msg[0]);

    // we need to obtain the rough multibyte length from the whar_t array
    // this is how we do it
    for (size_t i = 0; i < ssdp_wc_sz; i++)
    {
        len += wcslen(ssdp_wchar_msg[i]);
    }

    return 0;
}


int transform_msg_into_utf_8(void)
{
    // target utf-8 encoded message to send to the multicast scope address
    char msg[len + 1];
    msg[0] = '\0';

    size_t ret_val, msg_sz;
    ret_val = msg_sz = 0;

    for (size_t i = 0; i < ssdp_wc_sz; i++)
    {
        if ((ret_val = wcstombs(msg + strlen(msg), ssdp_wchar_msg[i], len)) == (size_t)-1)
        {
            printf("failed to convert wchar_t to multibyte string: wcstombs\n");

            // PERFORM_CLEANUP_OPERATIONS

            return -1;
        }
    }

    msg_sz = (sizeof(msg) / sizeof(msg[0]));
    msg[msg_sz] = '\0';

    // pass msg, msg_sz to function that send it to the multicast scope address
    // possibly set SOCK_T, and PMX to extern
    execute_httpu_module_main(host, port, msg, msg_sz, packet_mx, sockt_opt);

    return 0;
}
// -------------------------------------------------------------------------------------------


int set_packet_sockt_opts(char *packetmx, char *socktopt)
{
    packet_mx = strdup(packetmx);
    sockt_opt = strdup(socktopt);

    if (packet_mx == NULL || sockt_opt == NULL)
    {
        printf("Failed to set basic socket options: packet_mx or sockt_opt\n");

        return -1;
    }

    return 0;
}


// setup client multicast TCP socket structure options
int setup_multicast_scope_opts(void)
{
    scope_multicast_addr_opts.sin_family = AF_INET;
    scope_multicast_addr_opts.sin_port = htons(multicast_scope_port);
    scope_multicast_addr_opts.sin_addr.s_addr = inet_addr(multicast_scope_addr);

    return 0;
}


int setup_client_socket_opts(void)
{
    if ((client_multicast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        fprintf(stderr, "%s ERROR: Setting up UDP socket for multicast UDP discovery request!\n", RED_ERR);

        return -1;
    }

    // set socket timeout opts
    struct timeval timeout;

    timeout.tv_sec = atoi(sockt_opt);
    timeout.tv_usec = 0;

    // set data no receive socket timeout opt
    if (setsockopt(client_multicast_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
    {
        fprintf(stderr, "%s ERROR: Failed to set timeout setsockopt operation on the targeted socket!\n", RED_ERR);

        return -1;
    }

    return 0;
}


int send_ssdp_discovery_msg(char msg[], size_t msg_sz)
{
    printf("%s Sending target message:\n\"\n", BLUE_OK);
    printf("%s", msg);
    printf("\"\nto scope multicast address \033[0;4m%s:%d\033[0;m\n", multicast_scope_addr, multicast_scope_port);

    printf("\nPress \033[90;3m<ENTER>\033[0;m to send device traffic relay || Press \033[90;3m<CTRL + c>\033[0;m to abort...\n");
    getchar();

    // send SSDP discovery message
    for (int i = 0; i < atoi(packet_mx);)
    {
        // use the locally bound UDP socket, pass the message buffer, along with the length, no flags given, send the data
        // to the remote target, along with the size of the struct
        if (sendto(client_multicast_sock, msg, msg_sz, 0, (struct sockaddr*)&scope_multicast_addr_opts, sizeof(scope_multicast_addr_opts)) == -1)
        {
            fprintf(stderr, "%s ERROR: Failed to send data to scope multicast address for SSDP discovery operations...\n", RED_ERR);

            return -1;
        }

        printf("%s SENT \033[0;3mHTTPU SSDP M-SEARCH\033[0;m MESSAGE TO MULTICAST SCOPE ADDRESS #%d\n", GREEN_OK, i + 1);
        i++;
    }

    char resp_buffer[8000];

    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);

    // obtain data from the network interface card through the locally bound UDP socket
    // include the allocated space via a buffer to hold the retrieved data
    // no flags, pass a structure 'sender_addr', along with the size
    while (1)
    {
        int bytes_recv = recvfrom(client_multicast_sock, resp_buffer, sizeof(resp_buffer), 0, (struct sockaddr *)&sender_addr, &sender_addr_len);

        if (bytes_recv == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("\n%s Got setsockopt set timeout interval reached...\n", BLUE_OK);

                break;
            }

            else 
            {
                fprintf(stderr, "%s ERROR: Failed to obtain data from unicast network traffic.\n", RED_ERR);
            }
        }

        else 
        {
            // buffer to store converted internet protocol address 'sender_addr.sin_addr'
            char response_host[INET_ADDRSTRLEN];

            // convert the structural address type (binary) to a presentational format 
            inet_ntop(AF_INET, &(sender_addr.sin_addr), response_host, INET_ADDRSTRLEN);

            resp_buffer[bytes_recv] = '\0'; // null terminate received data
            size_t count_sz = snprintf(NULL, 0, "HTTPU-REPLY #\033[0;32m%d\033[0;m", sender_addr.sin_port);

            char *count = (char *) malloc(count_sz * sizeof(count));

            if (count == NULL)
            {
                printf("Failed to allocate buffer to store count!\n");

                return -1;
            }

            size_t port_sz = snprintf(NULL, 0, "%d", sender_addr.sin_port);

            char *port = (char *) malloc(port_sz * sizeof(port));

            if (port == NULL)
            {
                printf("Failed to allocate buffer to store port!\n");

                free(count);

                return -1;
            }

            // get individual messages from the buffer
            char *endline = strtok(resp_buffer, FMT_DATA);

            // make option to limit amount of responses recieved 
            // when it matches UNICAST_RESPONSES stop. 0 = unlimited

            sprintf(count, "HTTPU-REPLY #%d", UNICAST_RESPONSES + 1);
            sprintf(port, "%d", sender_addr.sin_port);

            size_t host_sz = snprintf(NULL, 0, "%s:%s", response_host, port);

            char *sender_host = (char *) malloc(host_sz * sizeof(sender_host));

            if (sender_host == NULL)
            {
                printf("Failed to allocate buffer to store sender address and port!\n");

                free(port);
                free(count);

                return -1;
            }

            sprintf(sender_host, "%s:%s", response_host, port);

            size_t table_header_sz = snprintf(NULL, 0, "%s => %s", count, sender_host);

            char *table_header = (char *) malloc(table_header_sz * sizeof(table_header));

            if (table_header == NULL)
            {
                printf("Failed to allocate buffer to store formatted table header!\n");

                return -1;
            }

            sprintf(table_header, "%s => %s", count, sender_host);

            // setup PACKET table
            ft_table_t *table = ft_create_table();

            ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
            ft_write_ln(table, table_header);

            while (endline != NULL)
            {
                // add endline to buffer 'char buffer[2000];'
                // find a method to split on ": " to grab HTTP header name/values
                // without having to print the entire line
                // this data will be stored in a database at a later date
                ft_write_ln(table, endline);
                endline = strtok(NULL, FMT_DATA);
            }

            ft_set_border_style(table, FT_BOLD_STYLE);

            printf("\n%s", ft_to_string(table));
            ft_destroy_table(table);

            UNICAST_RESPONSES++;

            if (UNICAST_RESPONSES == RESPONSES_WARRANTED)
            {
                printf("\n%s Reached RECVMAX interval for UDP unicast discover responses.\n", GREEN_OK);
                break;
            }
        }
    }

    close(client_multicast_sock);

    printf("%s %d Unicast discover responses confirmed.\n", GREEN_OK, UNICAST_RESPONSES);
    printf("\n%s Socket closed successfully.\n", BLUE_OK);

    return 0;
}


void httpu_module_sig_handler(int sigint)
{
    // find method to close socket
    printf("\n%s Got interrupt (SIG_INT)\n", BLUE_OK);
    printf("%s %d Unicast discover responses confirmed.\n", GREEN_OK, UNICAST_RESPONSES);

    exit(0);
}


int execute_httpu_module_main(char *ssdp_msa_addr, char *ssdp_msa_port, char msg[], size_t msg_sz, char *sockt_opt, char *packet_mx)
{
    if (signal(SIGINT, httpu_module_sig_handler) == SIG_ERR)
    {
        fprintf(stderr, "%s ERROR: Failed setting up main interrupt handler: main() => signal(SIGINT, httpu_module_sig_handler)!\n", RED_ERR);

        return -1;
    }

    multicast_scope_port = atoi(ssdp_msa_port);
    multicast_scope_addr = ssdp_msa_addr;

    int mcast_scope_ret, client_sock_ret;
    mcast_scope_ret = client_sock_ret = 0;

    // setup client struct options
    if ((mcast_scope_ret = setup_multicast_scope_opts()) != 0)
    {
        printf("failed to call function: setup_multicast_scope_opts()\n");

        return -1;
    }

    // setup client socket
    if ((client_sock_ret = setup_client_socket_opts()) != 0)
    {
        printf("failed to call function: setup_client_socket_opts()\n");

        return -1;
    }

    int ssdp_discover_ret, ssdp_reply_ret;
    ssdp_discover_ret = ssdp_reply_ret = 0;

    // send SSDP discovery request to the local administrative multicast scope domain
    if ((ssdp_discover_ret = send_ssdp_discovery_msg(msg, msg_sz)) != 0)
    {
        printf("failed to call function: send_ssdp_discovery_msg()\n");

        return -1;
    }

    return 0;
}

// -------------------------------------------------------------------------------------------


int main(int argc, char *argv[]) {
    if (argc < 7)
    {
        printf("BRIGHTSTAR::FATAL => could not obtain the proper arguments to pass to module \"httpu-discovery\"\n");

        return -1;
    }

    // set MAX responses to receive
    RESPONSES_WARRANTED = atoi(argv[7]);

    // setup global size options
    fmt_sz = strlen(FMT_DATA) + 1;
    m_search_sz = strlen(MSEARCH_STRING) + fmt_sz + 1; // "M-SEARCH * HTTP/1.1\r\n" + '\0'

    int m_search_ret, host_ret, st_ret, mx_ret, man_ret, fmt_data_ret;
    int ssdp_set_ret, start_wchar_ret;

    int wchar_alloc_ret, wchar_merge_ret, ssdp_wchar_ret;
    int msg_ret, setsock_ret, utf_8_transform_ret;

    m_search_ret = host_ret = st_ret = mx_ret = man_ret = fmt_data_ret = 0;
    ssdp_set_ret = start_wchar_ret = 0;

    wchar_alloc_ret = wchar_merge_ret = ssdp_wchar_ret = 0;
    msg_ret = setsock_ret = utf_8_transform_ret = 0;

    // +++++++++++++++++ M-SEARCH SETUP +++++++++++++++++
    if ((m_search_ret = httpu_m_search_setup()) != 0)
    {
        printf("failed to call: httpu_m_search_setup()\n");

        return -1;
    }

    // +++++++++++++++++ HOST SETUP +++++++++++++++++
    if ((host_ret = httpu_host_setup(argv[1], argv[2])) != 0)
    {
        printf("failed to call: httpu_host_setup()\n");

        return -1;
    }

    // +++++++++++++++++ SEARCH TARGET SETUP +++++++++++++++++
    if ((st_ret = httpu_st_setup(argv[3])) != 0)
    {
        printf("failed to call: httpu_set_setup()\n");

        return -1;
    }
    
    // +++++++++++++++++ MX SETUP +++++++++++++++++
    if ((mx_ret = httpu_mx_setup(argv[4])) != 0)
    {
        printf("failed to call: httpu_mx_setup()\n");

        return -1;
    }

    // +++++++++++++++++ MANEXT SETUP +++++++++++++++++
    if ((man_ret = httpu_man_setup()) != 0)
    {
        printf("failed to call: httpu_man_setup()\n");

        return -1;
    }
    
    // +++++++++++++++++ FMT_DATA SETUP +++++++++++++++++
    if ((fmt_data_ret = httpu_fmt_data_setup()) != 0)
    {
        printf("failed to call: httpu_fmt_data_setup()\n");

        return -1;
    }

    // copy char elements into char array
    if ((ssdp_set_ret = set_ssdp_char_msg_elements()) != 0)
    {
        printf("failed to call function: set_ssdp_char_msg_elements()\n");
        
        return -1;
    }

    // +++++++++++++++++ START WCS (Wide Character String) SIZE CONVERSION +++++++++++++++++
    if ((start_wchar_ret = start_wchar_sz_conversion()) != 0)
    {
        printf("failed to call function: start_wchar_sz_conversion()\n");

        return -1;
    }

    // +++++++++++++++++ START WCHAR_T ALLOCATION OPERATIONS +++++++++++++++++
    if ((wchar_alloc_ret = start_wchar_allocation_opts()) != 0)
    {
        printf("failed to call function: start_wchar_allocation_opts()\n");

        return -1;
    }

    // +++++++++++++++++ START WCHAR_T CONVERSION +++++++++++++++++
    if ((wchar_merge_ret = start_wchar_merge_main()) != 0)
    {
        printf("failed to call function: start_wchar_merge_main()\n");

        return -1;
    }

    // copy converted char to wchar_t elements into wchar_t array
    if ((ssdp_wchar_ret = set_ssdp_wchar_msg_elements()) != 0)
    {
        printf("failed to call function: set_ssdp_wchar_msg_elements()\n");

        return -1;
    }

    // obtain the final message length of each wchar_t string in the 'ssdp_wchar_msg'
    if ((msg_ret = return_final_msg_len()) != 0)
    {
        printf("failed to call function: return_final_msg_len()\n");

        return -1;
    }

    // set socket timeout and MAX packet count
    if ((setsock_ret = set_packet_sockt_opts(argv[5], argv[6])) != 0)
    {
        printf("failed to call function: set_packet_sockt_opts()\n");

        return -1;
    }

    // transform char buffer into adequate size from calling 'return_final_msg_len' to get the size of 'ssdp_wchar_msg'
    if ((utf_8_transform_ret = transform_msg_into_utf_8()) != 0)
    {
        printf("failed to call function: transform_msg_into_utf_8()\n");

        return 1;
    }

    // free ALL allocated char, wchar_t blocks in memory
    // i.e - release borrowed resources (from kernel space via user space system calls) that
    // have been allocated to the calling process (BRIGHTSTAR)
    PERFORM_CLEANUP_OPERATIONS

    // cleanup ssdp_char_msg
    for (size_t i = 0; i < (sizeof(ssdp_char_msg) / sizeof(ssdp_char_msg[0])); i++)
    {
        free(ssdp_char_msg[i]);
    }

    return 0;
}

// WireEye 2023 - BRIGHTSTAR SSDP analyzer/manipulation Framework