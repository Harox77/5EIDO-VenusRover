
#include <stdlib.h>
#include <stdio.h>
#include <mosquitto.h>
#include <unistd.h>
#include <string.h>

char received_message[256] = {0};
int flag = 0;



typedef struct pos_t{
    int x;
    int y;
    char c;
    struct pos_t *next;//right
    struct pos_t *prev;//left
    struct pos_t *top;
    struct pos_t *bottom;
}pos_t;


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
    //printf("New message with topic %s: %s\n", msg->topic, (char *) msg->payload);

    strcpy(received_message, msg->payload);
    flag = 1;

}




void addpos(pos_t **list, int x, int y, char c) {
    pos_t *newnode = malloc(sizeof(pos_t));
    int s=1;
    if (newnode == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newnode->x = x;
    newnode->y = y;
    newnode->c = c;
    newnode->top = NULL;
    newnode->bottom = NULL;
    newnode->next = NULL;
    newnode->prev = NULL;

    if (*list == NULL) {
        *list = newnode;
        return;
    }
    pos_t *temp = *list;
    // Navigate to the correct horizontal position
    if (x < temp->x) {
        if(s==1){printf("hello1");}
        while (temp->prev != NULL && temp->x > x){
            if(temp->x ==  x){
                if(s==1){printf("hello1.1");}
                break;
            }
            temp = temp->prev;
        }
        if (temp->x != x) {
            if(temp->prev!=NULL){
                temp->prev->next=newnode;
            }
            newnode->prev=temp->prev;
            temp->prev = newnode;
            newnode->next = temp;
        }
    }
    else if (x > temp->x) {
        if(s==1){printf("hello2");}
        while (temp->next != NULL && temp->x < x){
            if(temp->x ==  x){
                break;
            }
            temp = temp->next;
        }
        if (temp->x != x) {
            if(temp->next!=NULL){
                temp->next->prev=newnode;//something here
            }
            newnode->next = temp->next;
            temp->next = newnode;
            newnode->prev = temp;
        }
    }
    // If we didn't place the node horizontally, place it vertically
    if (temp->x == x) {
        if(s==1){printf("hello3");}
        if (y > temp->y) {
            if(s==1){printf("hello4");}
            while (temp->top != NULL && temp->top->y < y) {
                temp = temp->top;
            }
            if(temp->top!=NULL){
                temp->top->bottom=newnode;
            }
            newnode->top = temp->top;
            temp->top = newnode;
            newnode->bottom = temp;
        }
        if (y < temp->y) {
            if(s==1){printf("hello5");}
            while (temp->bottom != NULL && temp->bottom->y > y) {
                temp = temp->bottom;
            }
            if(temp->bottom!=NULL){
                temp->bottom->top=newnode;
            }
            newnode->bottom = temp->bottom;
            temp->bottom = newnode;
            newnode->top = temp;
        }
    }
}
void printgrid(pos_t **list,int counter){
    pos_t *temp = *list;
    pos_t *xnode = NULL;
    int s=0;
    int c = counter;
    if(temp==NULL){
        return;
    }
    while(temp->prev!=NULL && c == 0){//left most node
        temp=temp->prev;
        if(s==1){printf("hello1");}
    }
    xnode = temp;//saving the xnode
    while(temp->top!=NULL){//left top of the grid
        temp=temp->top;
        if(s==1){printf("hello2");}
    }
    while(temp!=NULL){//printing from top to bottom
        printf(" %c",temp->c);
        temp=temp->bottom;
        if(s==1){printf("hello3");}
    }
    if(xnode->next!=NULL){
        printf("\n");
        if(s==1){printf("hello4");}
        xnode = xnode->next;
        c=1;
        printgrid(&xnode,c);
    }
    else{
        printf("\n");
        if(s==1){printf("hello5");}
        return;
    }
}
int main(void) {
    pos_t *list=NULL;
    char cmd;
    int x=0;
    int y=0;
    int newFlag = 0;
    char c;

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


    do{
        printf("command? ");
        scanf(" %c",&cmd);
        switch(cmd){
            case'q':
                printf("Bye!\n");
                break;
            default:
                printf("Unknown command \'%c\'\n",cmd);
                break;
            case'c':
                while(1){

                    // Main program loop
                    sleep(1);  // Run for 10 seconds

                    if(flag == 1){
                        printf("Received message is %s\n", received_message);
                        flag = 0;
                        x = received_message[0] - '0';
                        y = received_message[1] - '0';
                        c = received_message[2];
                        addpos(&list,x,y,c);
                        printf("XYZ are %d %d %c\n", x, y ,c    );
                        newFlag = 1;
                    }

                    if(newFlag == 1){
                        break;
                    }



                }
                cmd = 'a';
                break;
            case'a':
                printf("co-ordinate x, y, c? ");
                scanf(" %d %d %c", &x, &y, &c);
                addpos(&list,x,y,c);
                cmd = 'c';
                break;
            case'p':
                printgrid(&list,0);
                break;
        }
    }while(cmd!='q');
    // Free allocated memory
    pos_t *current = list;
    while (current != NULL) {
        pos_t *next = current->next;
        free(current);
        current = next;
    }

    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);

    mosquitto_lib_cleanup();

}

