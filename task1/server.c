#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include "line_parser.h"
#include "common.h"


int status;
client_state* state;
int debug_mode=0;
int conn_handler(char** args, int args_len);
int bye_handler(char** args, int args_len);
int ls_handler(char** args, int args_len);
int get_handler(char** args, int args_len);
struct addrinfo hints, *result;
int (*func_ptr_arr[])(char** args, int args_len)={conn_handler,bye_handler,ls_handler,get_handler};
int sock_id;
int client_sock_id;
int count=0;

void cut_new_line(char* cmd){
    while(*cmd){
        if(*cmd=='\n'){
            *cmd='\0';
            break;
        }
        cmd++;
    }
}

int get_command_idx(char* cmd){
//     cut_new_line(cmd);
    if(strcmp(cmd,"hello")==0)return 0;
    else if(strcmp(cmd,"bye")==0)return 1;
    else if(strcmp(cmd,"ls")==0)return 2;
    else if(strcmp(cmd,"get")==0)return 3;
    return -1;
}

int exec(char** args, int args_len){
     if(args_len<1){
         
          if(debug_mode)printf("%d|ERROR: %s",count,"Invalid command from the client\n");

        return -1;
    }
    int command;
    if((command=get_command_idx(args[0]))==-1){
        if(debug_mode)printf("%d|ERROR: %s",count,"Invalid command from the client\n");
        if(send(client_sock_id,"Invalid command\n",17,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
            }
            reset_client(state);
            close(client_sock_id);
        return -2;
  
    }
    int result = (*func_ptr_arr[command])(args,args_len);  
    return result;
}



int get_handler(char** args, int args_len){
    if(state->conn_state!=CONNECTED){
        if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: conn_state is not CONNECTED\n");
        if(send(client_sock_id,"nok client state is not CONNECTED\n",40,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
            }
            reset_client(state);
            close(client_sock_id);
            return -2;
    }
    cut_new_line(args[1]);
    long filesize;
    if((filesize=file_size(args[1]))==-1){
        if(send(client_sock_id,"nok filesystem\n",16,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
            }
            reset_client(state);
            close(client_sock_id);
            return -2;
    }
    
    if(send(client_sock_id,"ok ",3,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
            }
    char response[11];
    sprintf(response,"%lu",filesize);
    if(send(client_sock_id,response,11,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
    }
    FILE* fp;
    if((fp=fopen(args[1],"r+"))==NULL){
        if(debug_mode)perror("Error: open");
        return -1;
    }
    state->conn_state=DOWNLOADING;
    int sent_bytes=0;
    
    while(sent_bytes<filesize){
    if((sent_bytes=+sendfile(client_sock_id,fileno(fp),NULL,filesize))==-1){
        if(debug_mode)perror("Error: sendfile");
        return -1;
    }
    }
//       struct timeval tv;
//     tv.tv_sec=10;
//     tv.tv_usec=0;
//     setsockopt(sock_id,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval));
//     
    char buffer[5];
    if(recv(client_sock_id,buffer,5,0)==-1){
            if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: recv\n");
            return -1;
    }  
    cmd_line* parsed;
    parsed=parse_cmd_lines(buffer);
    
    if(parsed==NULL || strcmp(parsed->arguments[0],"done")!=0){
            if(debug_mode)printf("%d|ERROR: %s",count,"Invalid response\n");
            if(send(client_sock_id,"nok done\n",16,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
            }
            reset_client(state);
            close(client_sock_id);
            return -2;
    } 
    state->conn_state=CONNECTED;   
    //recv done (if not arrive send -> nok done)
    if(send(client_sock_id,"ok",2,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
    }

    printf("Sent file %s\n",args[1]);
    free_cmd_lines(parsed);
//     tv.tv_sec=1000;
//     tv.tv_usec=0;
//     setsockopt(sock_id,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval));
    return 0;
    
}


int ls_handler(char** args, int args_len){
    if(state->conn_state!=CONNECTED){
        if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: conn_state is not CONNECTED\n");
        if(send(client_sock_id,"nok client state is not CONNECTED\n",40,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
            }
            reset_client(state);
            close(client_sock_id);
            return -2;
    }
    char* response;
    response=list_dir();
    if(response==NULL){
        if(send(client_sock_id,"nok filesystem\n",16,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
            }
            reset_client(state);
            close(client_sock_id);
            return -2;
    }
    if(send(client_sock_id,"ok ",3,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
            }
    if(send(client_sock_id,response,strlen(response),0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
    }
    free(response);
    char cwd[100];
    getcwd(cwd,100);
    printf("Listed files at %s\n",cwd);
    return 0;
}

int conn_handler(char** args, int args_len){
    
    if(state->conn_state!=IDLE){
        if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: conn_state is not IDLE\n");
        if(send(client_sock_id,"nok client state is not IDLE\n",30,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return -1;
            }
            reset_client(state);
            close(client_sock_id);
            return -2;
    }
    
    state->conn_state=CONNECTED;
    char response[100];
    state->client_id=malloc(5);
    char tmp[2];
    tmp[0]=count+'0';
    tmp[1]='\0';
    strcpy(response,"hello ");
    response[6]=*tmp;
    response[7]='\0';

    if(send(client_sock_id,response,strlen(response),0)==-1){
        if(debug_mode)perror("Error: send");
        return -1;
    }
    printf("Client %d connected\n",count);
    state->sock_fd=client_sock_id;
    *(state->client_id)=*tmp;
    

    return 0;
}



int bye_handler(char** args, int args_len){
    int return_val=0;
    if(state->conn_state!=CONNECTED){
        if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: conn_state is not CONNECTED\n");
            if(send(client_sock_id,"nok client state is not CONNECTED\n",40,0)==-1){
                if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
                return_val= -1;
            }
            
        return_val= -2;
    }
    else if(send(client_sock_id,"bye\n",4,0)==-1){
        if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: send\n");
        return_val= -1;
    }
    reset_client(state);
    close(client_sock_id);
    if(return_val==0)printf("Client %d disconnected\n",count);
    count++;
    if(count>9){
        count=0;
    }
    return return_val;
}

int client_loop(){
    while(1){
        char buffer[100];
        buffer[0]='\0';
        if(recv(client_sock_id,buffer,100,0)==-1){
            if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: recv\n");
            return -1;
        }    
        cmd_line* parsed;
        parsed=parse_cmd_lines(buffer);
        if(parsed==NULL){
            if(debug_mode)printf("%d|ERROR: %s",count,"Invalid command from the client\n");
            return -1;
        } 
        int result = exec( (char**)parsed->arguments,parsed->arg_count);
        if(result<0){
            return -1;
        }
        free_cmd_lines(parsed);
        if(state->conn_state==IDLE)
            break;
    }
    return 0;
}






int main(int argc, char **argv) {
    
    if(argc>1){
        if(strcmp(argv[1],"-d")==0){
            debug_mode=1;
        }   
    }
    state=init_client();
    strcpy(state->server_addr,"127.0.0.1");
//     gethostname(state->server_addr,2048);
//     struct sockaddr_in s_addr;
//     s_addr.sin_family=AF_INET;
//     s_addr.sin_port=htons("127.0.1.1");
//     s_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if((status = getaddrinfo(state->server_addr,"2018",&hints,&result))==-1){
        if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: getaddrinfo\n");
        return -1;
    }
    
    if((sock_id=socket(result->ai_family,result->ai_socktype,result->ai_protocol))==-1){ //args : ipv4,tcp,1 protocol==0
//     if((sock_id=socket(AF_INET,SOCK_STREAM,0))==-1){
        if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: socket\n");
        return -1;
    }
 
    if(bind(sock_id,result->ai_addr,result->ai_addrlen)==-1){
//     if(bind(sock_id,&s_addr,sizeof(s_addr))==-1){
        if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: bind\n");
        return -1;
    }
    if(listen(sock_id,1)==-1){
        if(debug_mode)printf("%s|Log: %s",state->server_addr,"Error: listen\n");
        return -1;
    }
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    
    
    if((client_sock_id=accept(sock_id,(struct sockaddr *)&client_addr,&client_addr_size))==-1){
        if(debug_mode)perror("Error: accept");
        return -1;
    }
    freeaddrinfo(result); //no longer needed
    client_loop();
    close(sock_id);
    
    return 0;
}

    
    
