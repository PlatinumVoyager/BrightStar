#ifndef MODULE_HTTPU_MAIN_H
#define MODULE_HTTPU_MAIN_H

#include <wchar.h>
#include <errno.h>
#include <locale.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

int httpu_discovery_main_module(char *msa, char *port, char *st, char *mx);
int run_httpu_module_main(char *port, char *msa, char *sockt_opt, char *ssdpmx, char *st, char *pmx);

int transform_msg_into_utf_8(int sockt);
int execute_httpu_module_main(char *ssdp_msa_addr, char *ssdp_msa_port, char msg[], size_t msg_sz, int sockt_opt, char *packet_mx);

void httpu_module_sig_handler(int sigint);
int setup_multicast_scope_opts(void);
int setup_client_socket_opts(int sock_opt, char msg[], size_t msg_sz, int packet_mx);
int send_ssdp_discovery_msg(char msg[], size_t msg_sz, int packet_mx);

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


#endif

// WireEye 2023 - BRIGHTSTAR SSDP analyzer/manipulation Framework