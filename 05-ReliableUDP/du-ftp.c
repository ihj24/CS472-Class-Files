#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>

#include "du-ftp.h"
#include "du-proto.h"

static char full_file_path[FNAME_SZ];

/*
 *  Helper function that processes the command line arguements.  Highlights
 *  how to use a very useful utility called getopt, where you pass it a
 *  format string and it does all of the hard work for you.  The arg
 *  string basically states this program accepts a -p or -c flag, the
 *  -p flag is for a "pong message", in other words the server echos
 *  back what the client sends, and a -c message, the -c option takes
 *  a course id, and the server looks up the course id and responds
 *  with an appropriate message.
 */
static int initParams(int argc, char *argv[], prog_config *cfg)
{
    int option;
    // setup defaults if no arguements are passed
    static char cmdBuffer[64] = {0};

    // setup defaults if no arguements are passed
    cfg->prog_mode = PROG_MD_CLI;
    cfg->port_number = DEF_PORT_NO;
    strcpy(cfg->file_name, PROG_DEF_FNAME);
    strcpy(cfg->svr_ip_addr, PROG_DEF_SVR_ADDR);

    while ((option = getopt(argc, argv, ":p:f:a:csh")) != -1)
    {
        switch (option)
        {
        case 'p':
            strncpy(cmdBuffer, optarg, sizeof(cmdBuffer));
            cfg->port_number = atoi(cmdBuffer);
            break;
        case 'f':
            strncpy(cfg->file_name, optarg, sizeof(cfg->file_name));
            break;
        case 'a':
            strncpy(cfg->svr_ip_addr, optarg, sizeof(cfg->svr_ip_addr));
            break;
        case 'c':
            cfg->prog_mode = PROG_MD_CLI;
            break;
        case 's':
            cfg->prog_mode = PROG_MD_SVR;
            break;
        case 'h':
            printf("USAGE: %s [-p port] [-f fname] [-a svr_addr] [-s] [-c] [-h]\n", argv[0]);
            printf("WHERE:\n\t[-c] runs in client mode, [-s] runs in server mode; DEFAULT= client_mode\n");
            printf("\t[-a svr_addr] specifies the servers IP address as a string; DEFAULT = %s\n", cfg->svr_ip_addr);
            printf("\t[-p portnum] specifies the port number; DEFAULT = %d\n", cfg->port_number);
            printf("\t[-f fname] specifies the filename to send or recv; DEFAULT = %s\n", cfg->file_name);
            printf("\t[-p] displays what you are looking at now - the help\n\n");
            exit(0);
        case ':':
            perror("Option missing value");
            exit(-1);
        default:
        case '?':
            perror("Unknown option");
            exit(-1);
        }
    }
    return cfg->prog_mode;
}

int server_loop(dp_connp dpc, void *sBuff, void *rBuff, int sbuff_sz, int rbuff_sz)
{
    int rcvSz;

    FILE *f = fopen(full_file_path, "wb+");
    if (f == NULL)
    {
        printf("ERROR:  Cannot open file %s\n", full_file_path);
        exit(-1);
    }
    if (dpc->isConnected == false)
    {
        perror("Expecting the protocol to be in connect state, but its not");
        exit(-1);
    }
    // Loop until a disconnect is received, or error hapens
    while (1)
    {

        // receive request from client
        rcvSz = dprecv(dpc, rBuff, rbuff_sz);
        if (rcvSz == DP_CONNECTION_CLOSED)
        {
            fclose(f);
            printf("Client closed connection\n");
            return DP_CONNECTION_CLOSED;
        }
        fwrite(rBuff, 1, rcvSz, f);
        rcvSz = rcvSz > 50 ? 50 : rcvSz; // Just print the first 50 characters max

        printf("========================> \n%.*s\n========================> \n",
               rcvSz, (char *)rBuff);
    }
}

void start_client(dp_connp dpc, const char *file_name)
{
    // Step 1 — send REQUEST PDU with filename
    ftp_pdu req;
    req.status = FTP_ST_REQUEST;
    req.err_code = FTP_ERR_NONE;
    req.payload_sz = 0;
    memset(req.file_name, 0, FNAME_SZ);
    strncpy(req.file_name, file_name, FNAME_SZ - 1);

    printf("sending PDU, status=%d size=%lu\n", req.status, sizeof(ftp_pdu));

    int rc = dpsend(dpc, &req, sizeof(ftp_pdu));
    if (rc < 0)
    {
        printf("[CLIENT] ERROR: failed to send request\n");
        exit(-1);
    }
    printf("[CLIENT] Sent REQUEST for file: %s\n", file_name);

    // Step 2 — wait for server response
    ftp_pdu resp;
    rc = dprecv(dpc, &resp, sizeof(ftp_pdu));
    if (rc < 0)
    {
        printf("[CLIENT] ERROR: failed to receive server response\n");
        exit(-1);
    }
    if (resp.status == FTP_ST_ERROR)
    {
        printf("[CLIENT] Server returned error code %d — aborting\n", resp.err_code);
        exit(-1);
    }
    if (resp.status != FTP_ST_READY)
    {
        printf("[CLIENT] Unexpected server status %d — aborting\n", resp.status);
        exit(-1);
    }
    printf("[CLIENT] Server READY — starting transfer\n");

    // Step 3 — open file and send data blocks
    static char data_buf[4096];
    static char send_buf[sizeof(ftp_pdu) + 4096];
    char full_path[FNAME_SZ + 16];
    snprintf(full_path, sizeof(full_path), "./outfile/%s", file_name);

    FILE *f = fopen(full_path, "rb");
    if (f == NULL)
    {
        printf("[CLIENT] ERROR: cannot open %s\n", full_path);
        exit(-1);
    }

    int bytes_read;
    while ((bytes_read = fread(data_buf, 1, sizeof(data_buf), f)) > 0)
    {
        int is_last = feof(f);

        ftp_pdu *blk = (ftp_pdu *)send_buf;
        blk->status = is_last ? FTP_ST_COMPLETE : FTP_ST_IN_PROGRESS;
        blk->err_code = FTP_ERR_NONE;
        blk->payload_sz = bytes_read;
        memset(blk->file_name, 0, FNAME_SZ);

        memcpy(send_buf + sizeof(ftp_pdu), data_buf, bytes_read);

        rc = dpsend(dpc, send_buf, sizeof(ftp_pdu) + bytes_read);
        if (rc < 0)
        {
            printf("[CLIENT] ERROR: failed to send data block\n");
            fclose(f);
            exit(-1);
        }
        printf("[CLIENT] Sent block: %d bytes\n", bytes_read);
    }
    fclose(f);

    // Step 4 — graceful close
    ftp_pdu close_pdu;
    close_pdu.status = FTP_ST_CLOSE;
    close_pdu.err_code = FTP_ERR_NONE;
    close_pdu.payload_sz = 0;
    memset(close_pdu.file_name, 0, FNAME_SZ);

    rc = dpsend(dpc, &close_pdu, sizeof(ftp_pdu));
    if (rc < 0)
    {
        printf("[CLIENT] ERROR: failed to send close\n");
        exit(-1);
    }
    printf("[CLIENT] Sent CLOSE\n");

    // wait for CLOSE_ACK
    ftp_pdu ack;
    rc = dprecv(dpc, &ack, sizeof(ftp_pdu));
    if (rc < 0)
    {
        printf("[CLIENT] ERROR: failed to receive close ack\n");
        exit(-1);
    }
    if (ack.status == FTP_ST_CLOSE_ACK)
        printf("[CLIENT] Received CLOSE_ACK — transfer complete\n");
    else
        printf("[CLIENT] WARNING: expected CLOSE_ACK, got status %d\n", ack.status);

    dpdisconnect(dpc);
}

void start_server(dp_connp dpc)
{
    // Step 1 — receive REQUEST PDU and extract filename
    ftp_pdu req;
    int rc = dprecv(dpc, &req, sizeof(ftp_pdu));
    if (rc < 0)
    {
        printf("[SERVER] ERROR: failed to receive request\n");
        return;
    }
    if (req.status != FTP_ST_REQUEST)
    {
        printf("[SERVER] ERROR: expected REQUEST, got status %d\n", req.status);
        return;
    }
    req.file_name[FNAME_SZ - 1] = '\0';
    printf("[SERVER] Client requests file: %s\n", req.file_name);

    // Step 2 — try to open file for writing, send READY or ERROR
    char full_path[FNAME_SZ + 16];
    snprintf(full_path, sizeof(full_path), "./infile/%s", req.file_name);

    FILE *f = fopen(full_path, "wb");
    if (f == NULL)
    {
        printf("[SERVER] ERROR: cannot open %s for writing\n", full_path);
        ftp_pdu err_pdu;
        err_pdu.status = FTP_ST_ERROR;
        err_pdu.err_code = FTP_ERR_DISK;
        err_pdu.payload_sz = 0;
        memset(err_pdu.file_name, 0, FNAME_SZ);
        rc = dpsend(dpc, &err_pdu, sizeof(ftp_pdu));
        if (rc < 0)
            printf("[SERVER] ERROR: failed to send error PDU\n");
        return;
    }

    // send READY
    ftp_pdu ready_pdu;
    ready_pdu.status = FTP_ST_READY;
    ready_pdu.err_code = FTP_ERR_NONE;
    ready_pdu.payload_sz = 0;
    memset(ready_pdu.file_name, 0, FNAME_SZ);
    strncpy(ready_pdu.file_name, req.file_name, FNAME_SZ - 1);

    rc = dpsend(dpc, &ready_pdu, sizeof(ftp_pdu));
    if (rc < 0)
    {
        printf("[SERVER] ERROR: failed to send READY\n");
        fclose(f);
        return;
    }
    printf("[SERVER] Sent READY — waiting for data\n");

    // Step 3 — receive data blocks and write to file
    static char recv_buf[sizeof(ftp_pdu) + 4096];
    long total_bytes = 0;

    while (1)
    {
        rc = dprecv(dpc, recv_buf, sizeof(recv_buf));
        printf("[SERVER] dprecv returned %d bytes, first 4 bytes: %d\n",
               rc, *(int *)recv_buf);
        if (rc == DP_CONNECTION_CLOSED)
        {
            printf("[SERVER] ERROR: connection closed unexpectedly\n");
            fclose(f);
            return;
        }
        if (rc < (int)sizeof(ftp_pdu))
        {
            printf("[SERVER] ERROR: received truncated PDU\n");
            fclose(f);
            return;
        }

        ftp_pdu *blk = (ftp_pdu *)recv_buf;

        // Step 4 — handle CLOSE
        if (blk->status == FTP_ST_CLOSE)
        {
            printf("[SERVER] Received CLOSE\n");
            fclose(f);

            // send CLOSE_ACK
            ftp_pdu close_ack;
            close_ack.status = FTP_ST_CLOSE_ACK;
            close_ack.err_code = FTP_ERR_NONE;
            close_ack.payload_sz = 0;
            memset(close_ack.file_name, 0, FNAME_SZ);

            rc = dpsend(dpc, &close_ack, sizeof(ftp_pdu));
            if (rc < 0)
                printf("[SERVER] ERROR: failed to send CLOSE_ACK\n");
            else
                printf("[SERVER] Sent CLOSE_ACK — total bytes written: %ld\n",
                       total_bytes);

            return;
        }

        // handle IN_PROGRESS and COMPLETE
        if (blk->status == FTP_ST_IN_PROGRESS || blk->status == FTP_ST_COMPLETE)
        {
            char *payload = recv_buf + sizeof(ftp_pdu);
            int written = fwrite(payload, 1, blk->payload_sz, f);
            if (written != blk->payload_sz)
            {
                printf("[SERVER] ERROR: disk write failed\n");
                fclose(f);
                return;
            }
            total_bytes += written;
            printf("[SERVER] Received block: %d bytes (total: %ld)\n",
                   blk->payload_sz, total_bytes);

            if (blk->status == FTP_ST_COMPLETE)
                printf("[SERVER] All data received — waiting for CLOSE\n");
        }
        else
        {
            printf("[SERVER] ERROR: unexpected status %d\n", blk->status);
            fclose(f);
            return;
        }
    }
}

int main(int argc, char *argv[])
{
    prog_config cfg;
    int cmd;
    dp_connp dpc;
    int rc;

    // Process the parameters and init the header - look at the helpers
    // in the cs472-pproto.c file
    cmd = initParams(argc, argv, &cfg);

    printf("MODE %d\n", cfg.prog_mode);
    printf("PORT %d\n", cfg.port_number);
    printf("FILE NAME: %s\n", cfg.file_name);
    printf("sizeof ftp_pdu = %lu\n", sizeof(ftp_pdu));

    switch (cmd)
    {
    case PROG_MD_CLI:
        // by default client will look for files in the ./outfile directory
        snprintf(full_file_path, sizeof(full_file_path), "./outfile/%s", cfg.file_name);
        dpc = dpClientInit(cfg.svr_ip_addr, cfg.port_number);
        rc = dpconnect(dpc);
        if (rc < 0)
        {
            perror("Error establishing connection");
            exit(-1);
        }

        start_client(dpc, cfg.file_name);
        exit(0);
        break;

    case PROG_MD_SVR:
        // by default server will look for files in the ./infile directory
        snprintf(full_file_path, sizeof(full_file_path), "./infile/%s", cfg.file_name);
        dpc = dpServerInit(cfg.port_number);
        rc = dplisten(dpc);
        if (rc < 0)
        {
            perror("Error establishing connection");
            exit(-1);
        }

        start_server(dpc);
        break;
    default:
        printf("ERROR: Unknown Program Mode.  Mode set is %d\n", cmd);
        break;
    }
}
