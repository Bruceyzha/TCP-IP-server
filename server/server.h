#include "dict.h"
typedef struct config_file{
    uint32_t IP;
    uint16_t port;
    uint8_t* path;
    int path_size;
}config;

typedef struct Server_para{
    int socket_id;
    uint8_t compress;
    uint8_t re_com;
    uint8_t* path;
    int path_size;
}server_para;


config* server_init(char * file_name);
int build_server(config* packet);
void send_msg(server_para* para,uint8_t type,payload* load);
void echo(server_para* para);
void error(server_para* para);
int massage_handler(server_para* para);
void shutdown_handler(server_para* para);
void directory_list(server_para* para);
void file_size(server_para* para);