#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>   
#include <arpa/inet.h>  
#include <string.h>
#include <dirent.h>
#include "server.h"
//Read the configuration file, and return the id address, 
//TCP port and the path of file of the client.
config* server_init(char * file_name){
    FILE* file=fopen(file_name,"rb");
    if(file==NULL){
        printf("file is not exist\n");
    }
    fseek(file,0,SEEK_END);
    int size = ftell(file);
    rewind(file);
    config* packet=malloc(sizeof(config));
    packet->path_size=size-5;
    packet->path=malloc(sizeof(unsigned char)*(packet->path_size));
    fread(&packet->IP,sizeof(uint32_t),1,file);
    fread(&packet->port,sizeof(uint16_t),1,file);
    fread(packet->path,sizeof(uint8_t),packet->path_size-1,file);
    fclose(file);
    packet->path[packet->path_size-1]='\0';
    return packet;
}

void shutdown_handler(server_para* para){
    shutdown(para->socket_id,SHUT_RDWR);
    close(para->socket_id);
}
//The error function, when server receive the error message, 
//the function will be called and close the socket id.
void error(server_para* para){
 uint8_t buffer[9];
 buffer[0]=0xf0;
 for(int i=1;i<9;i++){
     buffer[i]=0x00;
 }
    send(para->socket_id,buffer,sizeof(unsigned char)*9,0);
    close(para->socket_id);
}
void file_size(server_para* para){
    uint64_t paylength;
    recv(para->socket_id,&paylength,sizeof(uint64_t),0);
    paylength=(((uint64_t) ntohl(paylength)) << 32) + ntohl(paylength >> 32);
    if(paylength==0){
        error(para);
    }
    else{
        uint8_t *pay=malloc(sizeof(unsigned char)*paylength);
        recv(para->socket_id,pay,sizeof(uint8_t)*paylength,0);
        char* filepath=malloc(sizeof(char)*(para->path_size+paylength+1));
        for(int i=0;i<para->path_size-1;i++){
            filepath[i]=para->path[i];
        }
        filepath[para->path_size-1]='/';
        for(int i=0;i<paylength;i++){
            filepath[para->path_size+i]=pay[i];
        }
        filepath[para->path_size+paylength]='\0';
        FILE* file=fopen(filepath,"rb");
        if(file==NULL){
            error(para);
            free(filepath);
            free(pay);
        }
        else{
            fseek(file,0,SEEK_END);
            uint64_t size=ftell(file);
            fclose(file);
            payload* message=malloc(sizeof(payload));
            uint8_t *size8=malloc(sizeof(uint8_t)*8);
            for(int i=0;i<8;i++){
                size8[i]=size>>(7-i)*8;
                size8[i]=size8[i]<<(7-i)*8;
            }
            message->length=8;
            message->load=size8;
            free(filepath);
            free(pay);
            send_msg(para,0x5,message);
        }
    }

}



void send_msg(server_para* para,uint8_t type,payload* load){
    uint8_t head=type;
        head=head<<4;
        uint8_t new=0;
        if(para->re_com==1){
            new=1;
        }
        new=new<<3;
        head=head | new;
    if((para->compress==0&&para->re_com==0)||(para->compress==1&&para->re_com==1)){
        uint64_t paylen=load->length;
        uint64_t real=paylen;
        paylen=(((uint64_t)htonl(paylen)) << 32)+htonl(paylen >> 32);
        send(para->socket_id,&head,sizeof(unsigned char),0);
        send(para->socket_id,&paylen,sizeof(uint64_t),0);
        send(para->socket_id,load->load,sizeof(unsigned char)*real,0);
    }
    else if(para->compress==0&&para->re_com==1){
            payload* comed=compress(load);
            uint64_t newlength=comed->length;
            uint64_t real=newlength;
            newlength=(((uint64_t) htonl(newlength)) << 32) + htonl(newlength >> 32);
            send(para->socket_id,&head,sizeof(unsigned char),0);
            send(para->socket_id,&newlength,sizeof(uint64_t),0);
            send(para->socket_id,comed->load,sizeof(unsigned char)*real,0);
            free(comed->load);
            free(comed);
        }
    free(load->load);
    free(load);

}
//The echo function, when server receive the echo message,
//The function will be called and send the payload message to back.
//The function has not finished yet, the compress function will be included.
void echo(server_para* para){
    uint64_t paylength;
    recv(para->socket_id,&paylength,sizeof(uint64_t),0);
    paylength=(((uint64_t) ntohl(paylength)) << 32) + ntohl(paylength >> 32);
    uint8_t *pay=NULL;
    if(paylength!=0){
    pay=malloc(sizeof(unsigned char)*paylength);
    recv(para->socket_id,pay,sizeof(uint8_t)*paylength,0);
    payload* message=malloc(sizeof(payload));
    message->length=paylength;
    message->load=pay;
    send_msg(para,0x1,message);}
}

//This area will add more function to implement the server.
//(Directory listing,File size query,Retrieve file,Shutdown command,
//Lossless compression).

void directory_list(server_para* para){
    DIR * dir=NULL;
    struct dirent *entry;
    char* path=malloc(sizeof(char)*para->path_size);
    for(int i=0;i<para->path_size;i++){
        path[i]=para->path[i];
    }
    dir=opendir(path);
    uint8_t *file=malloc(sizeof(uint8_t)*4096);
    int file_num=0;
    while((entry = readdir(dir)) != NULL){
        if(entry->d_type==DT_REG){
            memcpy(file+file_num, entry->d_name, strlen(entry->d_name));
            file_num += strlen(entry->d_name);
            file[file_num] = 0x00;
            file_num++;
        }
    }
    closedir(dir);
    if(file_num==0){
        file[0]=0x00;
        file_num++;
    }
    uint8_t *real=malloc(sizeof(uint8_t)*file_num);
    for(int i=0;i<file_num;i++){
        real[i]=file[i];
    }
    free(path);
    free(file);
    payload* load=malloc(sizeof(payload));
    load->length=file_num;
    load->load=real;
    send_msg(para,0x3,load);
}


//Here is a massage_handler function. The server receives the message 
//from the client, the function will call different function according 
//to the msg_header(the first byte of the message). In milestone 
//the function just have error command and echo command. In the function,
//there is a infinite loop to deal with the multiple request until the error
//message or request finished.
int massage_handler(server_para* para){
    int status=0;
    while(1){
    uint8_t msg_header;
    if(read(para->socket_id,&msg_header,1)<0){
        break;
    }
    uint8_t type_field = msg_header >> 4;
    uint8_t compress=msg_header << 4;
    compress=compress >> 7;
    uint8_t re_com=msg_header << 5;
    re_com=re_com >> 7;
    para->compress=compress;
    para->re_com=re_com;
    if(type_field == 0x0){
        echo(para);
    }
    else if(type_field==0x8){
        shutdown_handler(para);
        status=1;
        break;
    }
    else if(type_field==0x2){
        directory_list(para);
    }
    else if(type_field==0x4){
        file_size(para);
    }
    else if(type_field==0x6){
        Retrieve(para);
    }
    else{
        error(para);
        break;
    }
    }
    shutdown(para->socket_id,SHUT_RDWR);
    close(para->socket_id);
    return status;
}


//In main function, the program initialize the server and connect with the 
//client. The while loop is for multiple parallel connections by using multiple 
//process.
int main(int argc, char** argv){
    if(argv[1]!=NULL){
        config* packet = server_init(argv[1]);
        int server = socket(AF_INET,SOCK_STREAM,0);
    if(server < 0){
        return -1;
    }
    struct sockaddr_in serv_addr;
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=packet->port;
    struct in_addr IP;
    IP.s_addr=packet->IP;
    serv_addr.sin_addr=IP;
    int addrlen=sizeof(serv_addr);
    bind(server,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    listen(server,3);
    while(1){
        int new_socket = accept(server,(struct sockaddr*)&serv_addr,(socklen_t*)&addrlen);
        pid_t p=fork();
        server_para* para=malloc(sizeof(server_para));
        para->socket_id=new_socket;
        para->path=packet->path;
        para->path_size=packet->path_size;
        if(p==0){
            int status=massage_handler(para);
            if(status==1){
                shutdown(new_socket,SHUT_RDWR);
                close(new_socket);
                exit(0); 
            }
            }
        
        
        }
    }
    return 0;
}