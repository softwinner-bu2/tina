#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stddef.h>
#include "connect.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

int(*func)(char*,int) = NULL;
#ifndef ANDROID_ENV
#define UNIX_DOMAIN "/tmp/UNIX.domain"
#else
//#define LOG_TAG "smartlink_jni"
//#include <cutils/log.h>
//#include <android/log.h>
#define UNIX_DOMAIN "/data/misc/wifi/UNIX.domain"
//#undef LOGD
//#undef LOGE
//#define LOGD ALOGD
//#define LOGE ALOGE
#endif
int sockfd;

void printf_info(struct _cmd *c){
	LOGD("cmd: %d\n",c->cmd);
	LOGD("ssid: %s\n",c->info.base_info.ssid);
	LOGD("pasd: %s\n",c->info.base_info.password);
	LOGD("pcol: %d\n",c->info.protocol);
	LOGD("radm: %d\n",c->info.airkiss_random);
	
}

int init_socket(){
	int ret;  
	struct sockaddr_un srv_addr;  
	//creat unix socket
	sockfd=socket(PF_UNIX,SOCK_STREAM,0);  
	if(sockfd<0)  
	{  
		LOGE("cannot create communication socket");  
		return -1;  
	}     
	srv_addr.sun_family=AF_UNIX;  
	//strcpy(srv_addr.sun_path,UNIX_DOMAIN);  
	strcpy(srv_addr.sun_path+1,UNIX_DOMAIN);
	srv_addr.sun_path[0] = '\0';
	int size = offsetof(struct sockaddr_un,sun_path) + strlen(UNIX_DOMAIN)+1;
	//connect server
	ret=connect(sockfd,(struct sockaddr*)&srv_addr,size);
	if(ret==-1)  
	{  
		LOGE("cannot connect to the server: %s",strerror(errno));  
		close(sockfd);  
		return -1;  
	}
	return 0;
}

int init(int fd,int(*f)(char*,int))
{
	func = f;
	return init_socket();
}
void release()
{
	close(sockfd);
}

void* readthread(void* arg){
	char buf[1024];
	if(func != NULL && func(buf,THREAD_INIT) == THREAD_EXIT);//test
	while(1){
		memset(buf,0,sizeof(buf));
		//int bytes_read = recv( sockfd, buf, sizeof(buf), 0 );
		int bytes_read = read(sockfd, buf, sizeof(buf)); //read once only
		//LOGD("read byte %d\n",bytes_read);
		if ( bytes_read == -1 )	{
			if( errno == EAGAIN || errno == EWOULDBLOCK ){
				continue;
			}
			LOGE("recv failed(%s)\n",strerror(errno));
			//exit(0);
		}
		else if ( bytes_read == 0 ){
			LOGE("server close\n");
			if(func != NULL)
				func(buf,bytes_read);
			break;
			//exit(0);
		}

		if(func != NULL && func(buf,bytes_read) == THREAD_EXIT)
			break;
	}
	release();
	return (void*)0;
}
int init_thread(){
	pthread_t tid;
	if(pthread_create( &tid, NULL, readthread, (void*)0 ) != 0){
		LOGE("create read thread failed!\n");
		return -1;
	}
	if( pthread_detach( tid ) ){
		LOGE("detach read thread failed!\n");
		return -1;
	}
	return 0;
}
int startairkiss(){
	struct _cmd c;
	c.cmd = START_AIRKISS;
	LOGD("cmd = %d\n",c.cmd);
	if(init_thread() == -1) return -1;
	return send(sockfd,(char*)&c,sizeof(c),0);
}
int easysetupfinish(struct _cmd *c){
	//printf_info(c);
	c->cmd = AIRKISS_FINISHED;
	
	return send(sockfd,(char*)c,sizeof(struct _cmd),0);

}
#ifdef __cplusplus
}
#endif
