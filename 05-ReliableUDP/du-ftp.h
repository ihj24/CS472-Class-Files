#pragma once

#define PROG_MD_CLI 0
#define PROG_MD_SVR 1
#define DEF_PORT_NO 2080
#define FNAME_SZ 150
#define PROG_DEF_FNAME "test.c"
#define PROG_DEF_SVR_ADDR "127.0.0.1"

// status values
#define FTP_ST_REQUEST 0 // ready to send
#define FTP_ST_READY 1   // ready to receive
#define FTP_ST_IN_PROGRESS 2
#define FTP_ST_COMPLETE 3
#define FTP_ST_ERROR 4
#define FTP_ST_CLOSE 5
#define FTP_ST_CLOSE_ACK 6

// error codes
#define FTP_ERR_NONE 0      // no error
#define FTP_ERR_NOT_FOUND 1 // file not found
#define FTP_ERR_PERM 2      // permission denied
#define FTP_ERR_DISK 3      // server write error
#define FTP_ERR_PROTO 4     // protocol violation

typedef struct prog_config
{
    int prog_mode;
    int port_number;
    char svr_ip_addr[16];
    char file_name[128];
} prog_config;

typedef struct ftp_pdu
{
    int status;
    int err_code;
    int payload_sz;
    char file_name[FNAME_SZ];
} ftp_pdu;