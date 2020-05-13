#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "line_parser.h"
#include "common.h"

int status;
int sock_id;
client_state* state;
int debug_mode=0;
int conn_handler(char** args, int args_len);
int bye_handler(char** args, int args_len);
int ls_handler(char** args, int args_len);
int get_handler(char** args, int args_len);
struct addrinfo hints, *result;
int (*func_ptr_arr[])(char** args, int args_len)={conn_handler,bye_handler,ls_handler,get_handler};

int get_command_idx(char* cmd){
    if(strcmp(cmd,"conn")==0)return 0;
    else if(strcmp(cmd,"bye")==0)return 1;
    else if(strcmp(cmd,"ls")==0)return 2;
    else if(strcmp(cmd,"get")==0)return 3;
    return -1;
}


int get_handler(char** args, int args_len){
    if(args_len<2){
        if(debug_mode)perror("Invalid input (num of args)");
        return -1;
    }
    if(state->conn_state!=CONNECTED){
        if(debug_mode)perror("Error: conn_state is not CONNECTED");
        return -2;
    }
    char buf[100];
    strcpy(buf,"get ");
    strcpy(buf+4,args[1]);
    strcpy(buf+4+strlen(args[1]),"\n");
    char msg[4+strlen(args[1])+1];
    strcpy(msg,buf);
    if(send(sock_id,msg,strlen(msg),0)==-1){
        if(debug_mode)perror("Error: send");
        return -1;
    }

    char buffer1[3];
    if(recv(sock_id,buffer1,3,0)==-1){
        if(debug_mode)perror("Error: recv");
        return -1;
    }
    
    cmd_line* parsed;
    parsed=parse_cmd_lines(buffer1);
    
    if((parsed->arg_count)<1){
        if(debug_mode)perror("Invalid response from the server");
        return -1;
    }
    else if(strcmp(parsed->arguments[0],"ok")!=0){
        if(strcmp(parsed->arguments[0],"nok")!=0){
        if(debug_mode)perror("Invalid response from the server");
        return -1;
        }
        else{
            if(debug_mode)fprintf(stderr,"Server Error: %s\n",parsed->arguments[1]);
            close(state->sock_fd);
            reset_client(state);
            return -1;
        }
    }
    char size[11];
    if(recv(sock_id,size,11,0)==-1){
        if(debug_mode)perror("Error: recv");
        return -1;
    }
    
    cmd_line* parsed2;
    parsed2=parse_cmd_lines(size);
    
    
    if((parsed2->arg_count)<1){
        if(debug_mode)perror("Invalid response from the server");
        return -1;
    }
    else if(strcmp(parsed2->arguments[0],"nok")==0){
            if(debug_mode)fprintf(stderr,"Server Error: %s\n",parsed2->arguments[1]);
            close(state->sock_fd);
            reset_client(state);
            return -1;
    }
    
    char file[1024];
    FILE* fp;
    if((fp=fopen(args[1],"w+"))==NULL){
        if(debug_mode)perror("Error: open");
        return -1;
    }
    state->conn_state=DOWNLOADING;
    int bytes_recv=0;
    int _recv=0;
   while(bytes_recv<atoi(size)){
        if((_recv=recv(sock_id,file,1024,0))==-1){
            if(debug_mode)perror("Error: recv");
            return -1;
            
        }
        bytes_recv=bytes_recv+_recv;
        fwrite(file,1,_recv,fp);
        fflush(fp);
        
    }
    
    
    
    
    
//     struct timeval tv;
//     tv.tv_sec=10;
//     tv.tv_usec=0;
//     setsockopt(sock_id,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(struct timeval));
//     setsockopt(sock_id,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval));
    if(send(sock_id,"done",5,0)==-1){
      if(remove(args[1])==-1){
          if(debug_mode)perror("Error: remove");
        return -1;
        }
        if(debug_mode)fprintf(stderr,"Error while downloading file %s\n",args[1]);
        return -1;
        
        
    }
    
    char ok[11];
    if(recv(sock_id,ok,11,0)==-1){
       if(remove(args[1])==-1){
          if(debug_mode)perror("Error: remove");
        return -1;
        }
        if(debug_mode)fprintf(stderr,"Error while downloading file %s\n",args[1]);
        return -1;
    }
    
    cmd_line* parsed3;
    parsed3=parse_cmd_lines(ok);
    
    
    if((parsed3->arg_count)<1){
        if(debug_mode)perror("Invalid response from the server");
        return -1;
    }
    else if(strcmp(parsed3->arguments[0],"nok")==0){
            if(debug_mode)fprintf(stderr,"Server Error: %s\n",parsed3->arguments[1]);
            close(state->sock_fd);
            reset_client(state);
            return -1;
    }

    
  
//     tv.tv_sec=1000;
//     tv.tv_usec=0;
//     setsockopt(sock_id,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(struct timeval));
//     setsockopt(sock_id,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(struct timeval));
    state->conn_state=CONNECTED;
    free_cmd_lines(parsed);
    free_cmd_lines(parsed2);
    free_cmd_lines(parsed3);
    
    return 0;
}


int ls_handler(char** args, int args_len){
    if(state->conn_state!=CONNECTED){
        if(debug_mode)perror("Error: conn_state is not CONNECTED");
        return -2;
    }
    if(send(sock_id,"ls",3,0)==-1){
        if(debug_mode)perror("Error: send");
        return -1;
    }

    char buffer1[3];
    if(recv(sock_id,buffer1,3,0)==-1){
        if(debug_mode)perror("Error: recv");
        return -1;
    }
    
    cmd_line* parsed;
    parsed=parse_cmd_lines(buffer1);
    
    if((parsed->arg_count)<1){
        if(debug_mode)perror("Invalid response from the server");
        return -1;
    }
    else if(strcmp(parsed->arguments[0],"ok")!=0){
        if(strcmp(parsed->arguments[0],"nok")!=0){
        if(debug_mode)perror("Invalid response from the server");
        return -1;
        }
        else{
            if(debug_mode)fprintf(stderr,"Server Error: %s\n",parsed->arguments[1]);
            close(state->sock_fd);
            reset_client(state);
            return -1;
        }
    }
    char buffer2[2048];
    if(recv(sock_id,buffer2,2048,0)==-1){
        if(debug_mode)perror("Error: recv");
        return -1;
    }
    
    
    cmd_line* parsed2;
    parsed2=parse_cmd_lines(buffer2);
    if((parsed2->arg_count)<1){
        if(debug_mode)perror("Invalid response from the server");
        return -1;
    }
    else if(strcmp(parsed2->arguments[0],"nok")==0){
            if(debug_mode)fprintf(stderr,"Server Error: %s\n",parsed2->arguments[1]);
            close(state->sock_fd);
            reset_client(state);
            return -1;
    }
    
    printf("%s",buffer2);
    free_cmd_lines(parsed);
    free_cmd_lines(parsed2);
    return 0;
}

void quit_handler(){
    //if(state->conn_state) check conn
    exit(0);
}

int conn_handler(char** args,int args_len){
    
    if(args_len<2){
        if(debug_mode)perror("Invalid input (num of args)");
        return -1;
    }
    if(state->conn_state!=IDLE){
        if(debug_mode)perror("Error: conn_state is not IDLE");
        return -2;
    }
    if((status = getaddrinfo(args[1],"2018",&hints,&result))==-1){
        if(debug_mode)perror("Error: getaddrinfo");
        return -1;
    }
    if((sock_id=socket(AF_INET,SOCK_STREAM,0))==-1){ //args : ipv4,tcp,1 protocol==0
        if(debug_mode)perror("Error: socket");
        return -1;
    }
    if(connect(sock_id,result->ai_addr,result->ai_addrlen)==-1){
         if(debug_mode)perror("Error: connect");
        return -1;
    }
    freeaddrinfo(result); //no longer needed
    if(send(sock_id,"hello",6,0)==-1){
        if(debug_mode)perror("Error: send");
        return -1;
    }
    state->conn_state=CONNECTING;
    char buffer[100];
    if(recv(sock_id,buffer,100,0)==-1){
        if(debug_mode)perror("Error: recv");
        return -1;
    }
    
    cmd_line* parsed;
    parsed=parse_cmd_lines(buffer);
    if((parsed->arg_count)<2){
        perror("Invalid response from the server");
    }
    else if(strcmp(parsed->arguments[0],"hello")!=0){
        if(strcmp(parsed->arguments[0],"nok")!=0){
        perror("Invalid response from the server");
        }
        else{
            fprintf(stderr,"Server Error: %s\n",parsed->arguments[1]);
            close(state->sock_fd);
            reset_client(state);
        }
    }
    
    state->client_id=parsed->arguments[1];
    free_cmd_lines(parsed);
    state->conn_state=CONNECTED;
    state->sock_fd=sock_id;
    strcpy(state->server_addr,*(args+1));
    
    return 0;
    
}
int bye_handler(char** args,int args_len){
    
    if(state->conn_state!=CONNECTED){
        if(debug_mode)perror("Error: conn_state is not CONNECTED");
        return -2;
    }
    if(send(state->sock_fd,"bye",4,0)==-1){
        if(debug_mode)perror("Error: send");
        return -1;
    }
    close(state->sock_fd);
    reset_client(state);
    return 0;
}

int exec(char** args, int args_len){
    if(args_len<1){
          if(debug_mode)perror("Invalid input");

        return -1;
    }
    int command;
    if((command=get_command_idx(args[0]))==-1){
        if(debug_mode)perror("Invalid input (command)");
        return -1;
    }
    int result = (*func_ptr_arr[command])(args,args_len);  
    return result;
}


int main(int argc, char **argv) {
    if(argc>1){
        if(strcmp(argv[1],"-d")==0){
            debug_mode=1;
        }   
    }
    state=init_client();
    char input[2048];
    printf("server:%s>",state->server_addr);
    fgets(input,2048,stdin);
    while(1){
        if(strcmp(input,"quit\n")==0)quit_handler();
        if(input[0]=='\n')
            printf("Invalid input\n");
        if(input[0]=='\t'||input[0]==' '){
            strncpy(input,input+1,strlen(input));
            continue;
        }
        
        if(strcmp(input,"")!=0){
            
        cmd_line* parsed;
        parsed=parse_cmd_lines(input);
        int result = exec( (char**)parsed->arguments,parsed->arg_count);
        if(result<0){
            perror("Invalid input");
            exit(1);
        }

        free_cmd_lines(parsed);
        printf("server:%s>",state->server_addr);//to do init the adress
        fgets(input,2048,stdin);
    }
     else printf("Invalid input\n");
       
    }
    free_client(state);
    return 0;
}

    
    
