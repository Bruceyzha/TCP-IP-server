#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>   
#include <arpa/inet.h>  
#include <string.h>
#include <dirent.h>
#include "dict.h"
Compress* dict_init(){
    FILE* file=fopen("compression.dict","rb");
    uint8_t length;
    uint8_t last;
    uint8_t half_length=0x00;
    int move=0;
    int pad=0;
    uint8_t first4bit=0x00;
    Compress* unit=malloc(sizeof(Compress)*256);
    for(int i=0;i<256;i++){
            fread(&length,sizeof(uint8_t),1,file);
            uint8_t tmp1=length;
            first4bit=tmp1<<(8-pad);
            length=length>>pad;
            length=length | half_length;
                    if(pad==0){
                        unit[i].left=NULL;
                        unit[i].right=NULL;
                        unit[i].bit_length=length;
                        unit[i].length=length/8+1;
                        if(length%8==0){
                            unit[i].length=length/8;
                        }
                        unit[i].offset=8-length%8;
                        unit[i].compressed=malloc(sizeof(uint8_t)*unit[i].length);
                        fread(unit[i].compressed,sizeof(uint8_t)*unit[i].length,1,file);
                        half_length=unit[i].compressed[unit[i].length-1]<<length%8;
                        unit[i].compressed[unit[i].length-1]=unit[i].compressed[unit[i].length-1]>>unit[i].offset;
                        unit[i].compressed[unit[i].length-1]=unit[i].compressed[unit[i].length-1]<<unit[i].offset;
                        pad=8-length%8;
                    }
                    else{
                    if(length%8<=pad){
                        unit[i].left=NULL;
                        unit[i].right=NULL;
                        unit[i].bit_length=length;
                        if(length%8==0){
                            unit[i].length=length/8;
                            unit[i].compressed=malloc(sizeof(uint8_t)*(unit[i].length));
                            fread(unit[i].compressed,sizeof(uint8_t),(unit[i].length),file);
                            half_length=unit[i].compressed[unit[i].length-1]<<(8+(length%8)-pad);
                        }
                        else{
                            unit[i].length=length/8+1;
                            unit[i].compressed=malloc(sizeof(uint8_t)*(unit[i].length-1));
                            fread(unit[i].compressed,sizeof(uint8_t),(unit[i].length-1),file);
                            half_length=unit[i].compressed[unit[i].length-2]<<(8+(length%8)-pad);
                        }
                        unit[i].offset=8-length%8;
                        uint8_t* replace=malloc(sizeof(uint8_t)*(unit[i].length));
                        replace[0]=first4bit;
                        replace[0]=replace[0]|(unit[i].compressed[0]>>pad);
                        for(int j=1;j<unit[i].length;j++){
                            replace[j]=replace[j]|(unit[i].compressed[j-1]<<(8-pad));
                            if(j!=(unit[i].length-1)){
                            replace[j]=replace[j]|(unit[i].compressed[j]>>pad);
                            }
                            }
                            free(unit[i].compressed);
                        unit[i].compressed=replace;
                        unit[i].compressed[unit[i].length-1]=unit[i].compressed[unit[i].length-1]>>unit[i].offset;
                        unit[i].compressed[unit[i].length-1]=unit[i].compressed[unit[i].length-1]<<unit[i].offset;
                        pad=pad-(length%8);
                    }
                    else if(length>pad){
                        unit[i].left=NULL;
                        unit[i].right=NULL;
                        unit[i].bit_length=length;
                        unit[i].length=length/8+1;
                        if(length%8==0){
                            unit[i].length=length/8;
                        }
                        unit[i].compressed=malloc(sizeof(uint8_t)*unit[i].length);
                        unit[i].offset=8-length%8;
                        fread(unit[i].compressed,sizeof(uint8_t),unit[i].length,file);
                        half_length=unit[i].compressed[unit[i].length-1]<<last<<((length%8)-pad);
                        uint8_t* replace=malloc(sizeof(uint8_t)*unit[i].length);
                        replace[0]=first4bit;
                        replace[0]=replace[0]|(unit[i].compressed[0]>>pad);
                            for(int j=1;j<unit[i].length;j++){
                                replace[j]=replace[j]|(unit[i].compressed[j-1]<<(8-pad));
                                replace[j]=replace[j]|(unit[i].compressed[j]>>pad);
                            }
                            free(unit[i].compressed);
                            unit[i].compressed=replace;
                            unit[i].compressed[unit[i].length-1]=unit[i].compressed[unit[i].length-1]>>unit[i].offset;
                            unit[i].compressed[unit[i].length-1]=unit[i].compressed[unit[i].length-1]<<unit[i].offset;
                        pad=(8+pad-length%8);
                    }
                    }
                }
    fclose(file);
    return unit;
}
void build_tree(Compress* node,Compress* head){
    Compress* tmp=head;
    for(int i=0;i<256;i++){
        uint8_t nodelength=node[i].length*8-node[i].offset;
        for(int j=0;j<nodelength;j++){
            int tmplength = j / 8;
            int tmpoffset = 7 - j % 8;
            uint8_t bit = (node[i].compressed[tmplength] >> tmpoffset) & 0x01;
            if(bit ==0){
                if(tmp->left==NULL){
                    tmp->left=&node[i];
                }
                tmp=tmp->left;
            }
            else{
                if(tmp->right==NULL){
                    tmp->right=&node[i];
                }
                tmp=tmp->right;
            }
        }
    }

}

payload* compress(payload* uncompress,Compress* array){
    int totallength=0;
    for(int i=0;i<uncompress->length;i++){
        totallength=totallength+array[uncompress->load[i]].bit_length;
    }
    payload* compressed=malloc(sizeof(payload));
    if(totallength%8!=0){compressed->length=totallength/8+1;}
    else{compressed->length=totallength/8;}
    compressed->load=malloc(sizeof(uint8_t)*compressed->length);
    compressed->load[0]=array[uncompress->load[0]].compressed[0];
    for(int i=1;i<uncompress->length;i++){
        compressed->load[i-1]=array[uncompress->load[i]];
    }
    return NULL;
}
payload* decompress(payload* compress){
    return NULL;
}