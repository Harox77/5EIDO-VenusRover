#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <unistd.h>

  void log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str) {
    printf("Log: %s\n", str);
}
int main(){
  int rc;
  struct mosquitto * mosq;

  mosquitto_lib_init();
  mosq = mosquitto_new("publisher", true, NULL);

  mosquitto_username_pw_set(mosq, "Student19", "Aenaech4");
  rc = mosquitto_connect(mosq, "mqtt.ics.ele.tue.nl", 1883, 60);


  int cool = 1;

  if(rc != 0){
    printf("There was an error with %d\n", rc);
    mosquitto_destroy(mosq);
    return -1;
  }

  printf("We are now connected to the broker\n");

  mosquitto_publish(mosq, &cool, "/pynqbridge/10/robotA/recv", 5, "Hello", 0, false);


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
