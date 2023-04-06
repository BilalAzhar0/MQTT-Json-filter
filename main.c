#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <mosquitto.h>
#include <time.h>
#include "json-c/json.h"
#include <string.h>
#define CAM_OFFLINE_TIMEOUT  10
#define AC_POWEROFF_TIMEOUT  10
int exit_all=0;
void *print_message_function( void *ptr );
void *attempt_mqtt_connect(void *ptr);
void *print_led_state(void* ptr);
void mqtt_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
void mqtt_connect_callback(struct mosquitto *mosq, void *userdata, int result);
void mqtt_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos);
void mqtt_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str);
void mqtt_disconnect_callback(struct mosquitto *mosq, void *userdata, int result);
void mqtt_publish_callback(struct mosquitto *mosq, void *userdata, int result);
void mqtt_unsubscribe_callback(struct mosquitto *mosq, void *userdata, int result);
void update_AC(char* payload);
void Toggle_AC(char* payload);
const char* parse_json(char* payload, const char* keyword);
bool get_status_AC(void);
bool check_timeout(clock_t past_time, int timeout);
char *sub_topic_1 = "detection/001/status";
char *sub_topic_2 = "detection/001/command";
char *sub_topic_3 = "tasmota/stat/ac_switch/RESULT";
char *AC_offline_payload = "{  \"command\": \"offline\" }";
clock_t message_time_stamp = 0 ;
clock_t AC_power0FF_time_stamp = 0 ;
clock_t AC_powerON_time_stamp = 0 ;
bool Last_state_AC = 0;
struct mosquitto_server{
		char *host;
		int port;
		int keepalive;
		bool clean_session;
	};
struct mosquitto *mosq1 = NULL;
struct mosquitto *mosq2 = NULL;
int main()
{
	//struct args *Allen = (struct args *)malloc(sizeof(struct args));
	// struct mosquitto_server* m1 = {"192.168.6.17",6883,60,true};
	struct mosquitto_server m1 = {"192.168.6.17",6883,60,true};
	struct mosquitto_server m2 = {"test.mosquitto.org",1883,60,true};
    // pthread_t thread1, thread2, thread3;
    // char *message1 = "Thread 1";
    // char *message2 = "Thread 2";
    // int  iret1, iret2;

    /* Create independent threads each of which will execute function */

    //iret1 = pthread_create( &thread1, NULL, print_message_function, (void*) message1);
    //iret2 = pthread_create( &thread2, NULL, attempt_mqtt_connect, (void*) m1);
	//iret3 = pthread_create( &thread3, NULL, attempt_mqtt_connect, (void*) m1);
	
     /* Wait till threads are complete before main continues. Unless we  */
     /* wait we run the risk of executing an exit which will terminate   */
     /* the process and all threads before the threads have completed.   */

     //pthread_join( thread1, NULL);
     //pthread_join( thread2, NULL); 

	// printf("Thread 1 returns: %d\n",iret1);
	// printf("Thread 2 returns: %d\n",iret2);
	//printf("Thread 3 returns: %d\n",iret3);
    //pthread_join( thread2, NULL);      
	
	

	// char *host = "192.168.6.17";
	// int port = 6883;
	// int keepalive = 60;
	// bool clean_session = true;
	
	mosquitto_lib_init();
	mosq1 = mosquitto_new("Filter_host", true, NULL);
	if(!mosq1){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}
	mosq2 = mosquitto_new("Control_host", true, NULL);
	if(!mosq2){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}
	mosquitto_disconnect_callback_set(mosq1, mqtt_disconnect_callback);	
	mosquitto_disconnect_callback_set(mosq2, mqtt_disconnect_callback);
	mosquitto_publish_callback_set(mosq1, mqtt_publish_callback);
	mosquitto_publish_callback_set(mosq2, mqtt_publish_callback);	
	mosquitto_unsubscribe_callback_set(mosq1, mqtt_unsubscribe_callback);		
	mosquitto_log_callback_set(mosq1, mqtt_log_callback);
	mosquitto_connect_callback_set(mosq1, mqtt_connect_callback);
	mosquitto_connect_callback_set(mosq2, mqtt_connect_callback);
	mosquitto_message_callback_set(mosq1, mqtt_message_callback);
	mosquitto_message_callback_set(mosq2, mqtt_message_callback);
	mosquitto_subscribe_callback_set(mosq1, mqtt_subscribe_callback);
	mosquitto_subscribe_callback_set(mosq2, mqtt_subscribe_callback);
	if(mosquitto_connect(mosq1, m1.host, m1.port, m1.keepalive)){
		fprintf(stderr, "Unable to connect.\n");
		return 1;
	}
	if(mosquitto_connect(mosq2, m2.host, m2.port, m2.keepalive)){
		fprintf(stderr, "Unable to connect.\n");
		return 1;
	}
	message_time_stamp = clock();
	while(1) {
		mosquitto_loop(mosq1, -1, 1);
		mosquitto_loop(mosq2, -1, 1);
		//mosquitto_publish(mosq2,NULL,"tasmota/cmnd/ac_switch/POWER2",0,NULL,0,NULL);
		if(check_timeout(message_time_stamp,CAM_OFFLINE_TIMEOUT)){
			printf("Camera is offline\n");
			//mosquitto_publish(mosq2,NULL,sub_topic_2,strlen(AC_offline_payload),AC_offline_payload,0,true);
		}
	} 
	mosquitto_destroy(mosq1);
	mosquitto_destroy(mosq2);
	mosquitto_lib_cleanup();
     return 1;
}
void *print_led_state(void* ptr)
{
     while(1)
     {
          if(exit_all)
               break;
     }
}
void *print_message_function(void *ptr)
{
     char *message;
     message = (char *) ptr;
     printf("%s \n", message);
}
// void *attempt_mqtt_connect(void *ptr)
// {
// 	while(mosquitto_connect(mosq, (struct*)ptr->host, port, keepalive)){
// 		printf("Connection to Mosquitto broker failed, attempting again \n");
// 	}
// }
void mqtt_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if(message->payloadlen){
		printf("%s %s\n", message->topic,(char*)message->payload);
		message_time_stamp = clock();
		if(!strcmp(message->topic,sub_topic_3)){
			Toggle_AC((char*)message->payload);
		}
		if(!strcmp(message->topic,sub_topic_1)){
			update_AC((char*)message->payload);
		}
	}
	else{
		printf("%s (null)\n", (char*)message->topic);
	}
	fflush(stdout);
}
void mqtt_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	int i;
	if(!result){
		/* Subscribe to broker information topics on successful connect. */
		printf("Mosquitto Connect Success\n");
		if(mosq == mosq2){
			printf("Mosquitto 2 Connect Success\n");
			if(mosquitto_subscribe(mosq2,NULL,sub_topic_3,0)){
				printf("Subscription to topic %s failed",sub_topic_3);	
			}
		}
		if(mosq == mosq1){
			printf("Mosquitto 1 Connect Success\n");
			if(mosquitto_subscribe(mosq,NULL,sub_topic_1,0)){
				printf("Subscription to topic %s failed",sub_topic_1);	
			}
		}	
	}else{
		fprintf(stderr, "Connect failed\n");
	}
}
void mqtt_disconnect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	fprintf(stderr, "mosquitto disconnected\n");
}
void mqtt_publish_callback(struct mosquitto *mosq, void *userdata, int result)
{
	printf("mqtt message published  {info:%s, id:%d}\n",(char*)userdata, result);
}
void mqtt_unsubscribe_callback(struct mosquitto *mosq, void *userdata, int result)
{
	printf("mqtt unsubscribe {info:%s, id:%d}\n",(char*)userdata, result);
}
void mqtt_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;

	printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		printf(", %d", granted_qos[i]);
	}
	printf("\n");
}
void mqtt_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
	printf("%s\n", str);
}
void update_AC(char* payload)
{
	//printf("checking AC\n");
	char* keyword = "status";
	if(!Last_state_AC){
		if(!strcmp(parse_json(payload,keyword),"found")){
			Last_state_AC = 1;
			printf("Turning AC ON\n");
			mosquitto_publish(mosq2,NULL,"tasmota/cmnd/ac_switch/POWER2",0,NULL,0,NULL);
		}
	}
	else if(Last_state_AC){
		if(!strcmp(parse_json(payload,keyword),"not_found")){
			printf("Checking timeout\n");
			if(check_timeout(AC_power0FF_time_stamp,AC_POWEROFF_TIMEOUT)){
				Last_state_AC = 0;
				printf("Turning AC OFF\n");
				mosquitto_publish(mosq2,NULL,"tasmota/cmnd/ac_switch/POWER2",0,NULL,0,NULL);
			}
		}
		else{ AC_power0FF_time_stamp = clock(); }	
	}	
}
const char* parse_json(char* payload, const char* keyword)
{
	char buffer[strlen(payload)+1];
	sprintf(buffer,"%s",payload);
	struct json_object *parsed_json;
	struct json_object *status;
	parsed_json = json_tokener_parse(buffer);
    if(!json_object_object_get_ex(parsed_json,keyword,&status)){
		printf("Json Invalid\n");
		return "null";
	}
	else{
		const char* status_mess = json_object_get_string(status);
		//printf("last output %s\n",status_mess);
		return status_mess;	
	}
}
bool check_timeout(clock_t past_time, int timeout)
{
	clock_t present = clock();
	double time_elapsed = ((double)(present - past_time) / CLOCKS_PER_SEC) * 10000;
	printf("Timeout is %f \n",time_elapsed);
	if(time_elapsed >= timeout){ return 1; }
	else{ return 0;}
}
void Toggle_AC(char* payload)
{
	char* keyword = "POWER2";
	printf("checking current AC state\n");
	if(Last_state_AC){
		printf("AC supposed to be on\n");
		if(!strcmp(parse_json(payload,keyword),"ON")){
			printf("AC has been turned ON\n");
			mosquitto_publish(mosq2,NULL,"tasmota/cmnd/ac_switch/POWER2",3,"OFF",0,NULL);
		}
		else{printf("AC already ON\n");}
	}
	else{
		if(!strcmp(parse_json(payload,keyword),"OFF")){ 
			mosquitto_publish(mosq2,NULL,"tasmota/cmnd/ac_switch/POWER2",2,"ON",0,NULL);
			printf("AC has been turned OFF\n");
		}
	}
}