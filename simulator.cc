/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android_modem.h"
#include "sysdeps.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>

// Protobuff
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "sensors_packet.pb.h"

#define  DEFAULT_PORT  6703

static AModem  modem;
static SysChannel s_handler;

// XXX

google::protobuf::uint32 read_header(char *buf)
{
  google::protobuf::uint32 size;
  google::protobuf::io::ArrayInputStream ais(buf,4);
  google::protobuf::io::CodedInputStream coded_input(&ais);
  coded_input.ReadVarint32(&size);//Decode the HDR and get the size
  return size;
}

void read_body(int csock, google::protobuf::uint32 size)
{
    int bytecount;
    sensors_packet payload;
    char buffer [size];//size of the payload and hdr
    //Read the entire buffer including the hdr
    if((bytecount = recv(csock, (void *)buffer, size, MSG_WAITALL))== -1){
        fprintf(stderr, "Error receiving data %d\n", errno);
    }
    /*
    //Assign ArrayInputStream with enough memory
    google::protobuf::io::ArrayInputStream ais(buffer,size+4);
    google::protobuf::io::CodedInputStream coded_input(&ais);
    //Read an unsigned integer with Varint encoding, truncating to 32 bits.
    coded_input.ReadVarint32(&size);
    //After the message's length is read, PushLimit() is used to prevent the CodedInputStream
    //from reading beyond that length.Limits are used when parsing length-delimited
    //embedded messages
    google::protobuf::io::CodedInputStream::Limit msgLimit = coded_input.PushLimit(size);
    //De-Serialize
    payload.ParseFromCodedStream(&coded_input);
    //Once the embedded message has been parsed, PopLimit() is called to undo the limit
    coded_input.PopLimit(msgLimit);
    */
    payload.ParseFromArray(buffer, size);

    if (payload.has_gsm()) {
        switch (payload.gsm().action_type()) {
            case sensors_packet_GSMPayload_GSMActionType_RECEIVE_CALL:
                amodem_add_inbound_call(modem, payload.gsm().phone_number().c_str());
                break;
            case sensors_packet_GSMPayload_GSMActionType_ACCEPT_CALL:
            case sensors_packet_GSMPayload_GSMActionType_CANCEL_CALL:
            case sensors_packet_GSMPayload_GSMActionType_HOLD_CALL:
                amodem_disconnect_call(modem, payload.gsm().phone_number().c_str());
                break;
            case sensors_packet_GSMPayload_GSMActionType_RECEIVE_SMS:
                {
                    SmsAddressRec sender;
                    char* phone_number = (char*) payload.gsm().phone_number().c_str();
                    if (sms_address_from_str(&sender, phone_number, strlen(phone_number)) < 0) {
                        return;
                    }
                    char* sms_text = (char*) payload.gsm().sms_text().c_str();
                    int textlen = strlen(sms_text);
                    textlen = sms_utf8_from_message_str(sms_text, textlen, (unsigned char*) sms_text, textlen);
                    if (textlen < 0) {
                        return;
                    }

                    SmsPDU* pdus = smspdu_create_deliver_utf8((const unsigned char*) sms_text, textlen, &sender, NULL);
                    int nn;
                    for (nn = 0; pdus[nn] != NULL; nn++) {
                        amodem_receive_sms(modem, pdus[nn]);
                    }
                    smspdu_free_list(pdus);
                    break;
                }
            case sensors_packet_GSMPayload_GSMActionType_SET_SIGNAL:
                amodem_set_signal_strength(modem, payload.gsm().signal_strength());
                break;
            case sensors_packet_GSMPayload_GSMActionType_SET_NETWORK_TYPE:
                amodem_set_data_network_type(modem, android_parse_network_type(payload.gsm().network().c_str()));
                break;
            case sensors_packet_GSMPayload_GSMActionType_SET_NETWORK_REGISTRATION:
                ARegistrationState reg = A_REGISTRATION_HOME;
                char* type = (char*) payload.gsm().registration().c_str();
                if (!strcmp("home", type))
                    reg = A_REGISTRATION_HOME;
                else if (!strcmp("roaming", type))
                    reg = A_REGISTRATION_ROAMING;
                else if (!strcmp("searching", type))
                    reg = A_REGISTRATION_SEARCHING;
                else if (!strcmp("none", type))
                    reg = A_REGISTRATION_UNREGISTERED;
                else if (!strcmp("denied", type))
                    reg = A_REGISTRATION_DENIED;
                amodem_set_data_registration(modem, reg);
                break;
        }
    }else{
    }
}

// XXX


typedef struct {
    SysChannel   channel;
    char         in_buff[ 128 ];
    int          in_pos;

    char         out_buff[ 128 ];
    int          out_pos;
    int          out_size;
} ClientRec, *Client;

static Client
client_alloc( SysChannel  channel )
{
    Client  client = (Client) calloc( sizeof(*client), 1 );

    client->channel = channel;
    return client;
}

static void
client_free( Client  client )
{
    sys_channel_close( client->channel );
    client->channel = NULL;
    free( client );
}

static void
client_append( Client  client, const char*  str, int len );

static void
dump_line( const char*  line, const char*  prefix )
{
    if (prefix)
        printf( "%s", prefix );

    for ( ; *line; line++ ) {
        int  c = line[0];

        if (c >= 32 && c < 127)
            printf( "%c", c );
        else if (c == '\r')
            printf( "<CR>" );
        else if (c == '\n')
            printf( "<LF>" );
        else
            printf( "\\x%02x", c );
    }
    printf( "\n" );
}

static void
client_handle_line( Client  client, const char*  cmd )
{
    const char*  answer;

    dump_line( cmd, "<< " );
    answer = amodem_send( modem, cmd );
    if (answer == NULL)  /* not an AT command, ignored */ {
        printf( "-- NO ANSWER\n" );
        return;
    }

    dump_line( answer, ">> " );
    client_append( client, answer, -1 );
    client_append( client, "\r", 1 );
}

// XXX: Replace with protobuf
static void
cmd_client_handle_line( Client  client, const char*  cmd )
{
    char command[10];
    bzero(command, 10);
    char* p, *args;

    p = (char*) strchr((const char*) cmd, ' ');
    args = p+1;

    SmsPDU* pdus;
    SmsAddressRec sender;
    if (strncmp(cmd, "SMS", 3) == 0)
    {
      p = strchr(p+1, ' ');
      if (sms_address_from_str(&sender, args, p - args) < 0) {
        return;
      }
      p += 1;
      int textlen = strlen(p);
      textlen = sms_utf8_from_message_str(p, textlen, (unsigned char*) p, textlen);
      if (textlen < 0) {
        return;
      }

      pdus = smspdu_create_deliver_utf8( (unsigned char*) p, textlen, &sender, NULL);
      int nn;
      for (nn = 0; pdus[nn] != NULL; nn++) {
        amodem_receive_sms(modem, pdus[nn]);
      }
      smspdu_free_list(pdus);
    }
    else if (!strncmp("NET", cmd, 3))
    {
      const char* net_type = p+1;
      amodem_set_data_network_type(modem, android_parse_network_type(net_type));
    }
    else if (!strncmp("CALL", cmd, 4))
    {
      const char* number = p+1;
      amodem_add_inbound_call(modem, number);
    }
    else if (!strncmp("END", cmd, 3))
    {
      const char* number = p+1;
      amodem_disconnect_call(modem, number);
    }
    else if (!strncmp("SIGNAL", cmd, 6))
    {
      int signal = atoi(p + 1);
      if (signal < 0 || signal > 4)
        signal = 3;
      amodem_set_signal_strength(modem, signal);
    }
    else if (!strncmp("REG", cmd, 3))
    {
      const char* type = p+1;
      ARegistrationState reg = A_REGISTRATION_HOME;
      if (!strcmp("home", type))
        reg = A_REGISTRATION_HOME;
      else if (!strcmp("roaming", type))
        reg = A_REGISTRATION_ROAMING;
      else if (!strcmp("searching", type))
        reg = A_REGISTRATION_SEARCHING;
      else if (!strcmp("none", type))
        reg = A_REGISTRATION_UNREGISTERED;
      else if (!strcmp("denied", type))
        reg = A_REGISTRATION_DENIED;
      amodem_set_data_registration(modem, reg);
    }
}

static void
client_handler( void* _client, int  events )
{
    Client  client = (Client) _client;

    if (events & SYS_EVENT_READ) {
        int  ret;
        /* read into buffer, one character at a time */
        ret = sys_channel_read( client->channel, client->in_buff + client->in_pos, 1 );
        if (ret != 1) {
            fprintf(stderr, "client %p could not read byte, result = %d, error: %s\n",
                    client, ret, strerror(errno) );
            goto ExitClient;
        }
        if (client->in_buff[client->in_pos] == '\r' ||
            client->in_buff[client->in_pos] == '\n' ) {
            const char*  cmd = client->in_buff;
            client->in_buff[client->in_pos] = 0;

            if (client->in_pos > 0) {
                client_handle_line( client, cmd );
                client->in_pos = 0;
            }
        } else
            client->in_pos += 1;
    }

    if (events & SYS_EVENT_WRITE) {
        int  ret;
        /* write from output buffer, one char at a time */
        ret = sys_channel_write( client->channel, client->out_buff + client->out_pos, 1 );
        if (ret != 1) {
            fprintf(stderr, "client %p could not write byte, result = %d, error: %s\n",
                    client, ret, strerror(errno) );
            goto ExitClient;
        }
        client->out_pos += 1;
        if (client->out_pos == client->out_size) {
            client->out_size = 0;
            client->out_pos  = 0;
            /* we don't need to write */
            sys_channel_on( client->channel, SYS_EVENT_READ, client_handler, client );
        }
    }
    return;

ExitClient:
    printf( "client %p exiting\n", client );
    client_free( client );
}


static void
client_append( Client  client, const char*  str, int len )
{
    int  avail;

    if (len < 0)
        len = strlen(str);

    avail = sizeof(client->out_buff) - client->out_size;
    if (len > avail)
        len = avail;

    memcpy( client->out_buff + client->out_size, str, len );
    if (client->out_size == 0) {
        sys_channel_on( client->channel, SYS_EVENT_READ | SYS_EVENT_WRITE, client_handler, client );
    }
    client->out_size += len;
}


static void
accept_func( void*  _server, int  events )
{
    SysChannel  server  = (SysChannel) _server;
    SysChannel  handler;
    Client      client;

    printf( "connection accepted for server channel, getting handler socket\n" );
    handler = sys_channel_create_tcp_handler( server );
    client  = client_alloc( handler );
    printf( "got one. created client %p\n", client );

    events=events;
    sys_channel_on( handler, SYS_EVENT_READ, client_handler, client );
    s_handler = handler;
}

static void
cmd_client_handler( void* _client, int  events )
{
    Client  client = (Client) _client;

    if (events & SYS_EVENT_READ) {
        int byte_count;
        char buffer[4] = {0, 0, 0, 0};
        int ret;
        /* read into buffer, one character at a time */
        if ((byte_count = recv(channel_get_fd(client->channel), buffer, 4, MSG_WAITALL)) <= 0) {
            fprintf(stderr, "client %p could not read byte, result = %d, error: %s\n",
                    client, ret, strerror(errno) );
            goto ExitCmdClient;
        }
        read_body(channel_get_fd(client->channel), read_header(buffer));
        client->in_buff[0] = 0;
        client->in_pos = 0;
    }

    if (events & SYS_EVENT_WRITE) {
        int  ret;
        /* write from output buffer, one char at a time */
        ret = sys_channel_write( client->channel, client->out_buff + client->out_pos, 1 );
        if (ret != 1) {
            fprintf(stderr, "client %p could not write byte, result = %d, error: %s\n",
                    client, ret, strerror(errno) );
            goto ExitCmdClient;
        }
        client->out_pos += 1;
        if (client->out_pos == client->out_size) {
            client->out_size = 0;
            client->out_pos  = 0;
            /* we don't need to write */
            sys_channel_on( client->channel, SYS_EVENT_READ, client_handler, client );
        }
    }
    return;

ExitCmdClient:
    printf( "client %p exiting\n", client );
    client_free( client );
}


static void
cmd_accept_func( void*  _server, int  events )
{
    SysChannel  server  = (SysChannel) _server;
    SysChannel  handler;
    Client      client;

    printf( "connection accepted for server channel, getting handler socket\n" );
    handler = sys_channel_create_tcp_handler( server );
    client  = client_alloc( handler );
    printf( "got one. created client %p\n", client );

    events=events;
    sys_channel_on( handler, SYS_EVENT_READ, cmd_client_handler, client );
}



void func(void* opaque, const char* truc) {
  printf("Unsol: %p %s", opaque, truc);
  sys_channel_write(s_handler, truc, strlen(truc));
}



int  main( void )
{
    int  port = DEFAULT_PORT;
    SysChannel  server, cmd_server;

    sys_main_init();

    server = sys_channel_create_tcp_server( port );
    cmd_server = sys_channel_create_tcp_server( port  + 1);
    printf( "GSM simulator listening on local port %d, %d %p %p\n", port, port + 1, server, cmd_server);

    modem = amodem_create( 1, func, server);

    sys_channel_on( server, SYS_EVENT_READ, accept_func, server );
    sys_channel_on( cmd_server, SYS_EVENT_READ, cmd_accept_func, cmd_server );
    sys_main_loop();
    printf( "GSM simulator exiting\n" );
    return 0;
}
