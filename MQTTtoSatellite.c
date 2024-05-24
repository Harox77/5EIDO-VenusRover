#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <unistd.h>

void log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str) {
    printf("Log: %s\n", str);
}


void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    printf("ID: %d\n", * (int *) obj);
    int cool = 1;
    if(rc) {
        printf("Error with result code: %d\n", rc);
        exit(-1);
    }
    mosquitto_subscribe(mosq, &cool, "/pynqbridge/10/send", 0);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    printf("New message with topic %s: %s\n", msg->topic, (char *) msg->payload);
}



int main(){
    int rc;
    struct mosquitto * mosq;

    mosquitto_lib_init();
    mosq = mosquitto_new("receiver", true, NULL);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    mosquitto_username_pw_set(mosq, "Student19", "Aenaech4");
    rc = mosquitto_connect(mosq, "mqtt.ics.ele.tue.nl", 1883, 60);


    if(rc) {
        printf("Could not connect to Broker with return code %d\n", rc);
        return -1;
    }


    mosquitto_log_callback_set(mosq, log_callback);
    mosquitto_loop_start(mosq);

    // Main program loop
    sleep(10);  // Run for 10 seconds

    // Clean up
    mosquitto_loop_stop(mosq, true);


    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);

    mosquitto_lib_cleanup();






    return 0;
}
