#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <mosquitto.h>


char received_message[256] = {0};
int flag = 0;
int newFlag = 0;


typedef struct pos_t{
  int x;
  int y;
  int c;
  int t;
  struct pos_t *next;//right
  struct pos_t *prev;//left
  struct pos_t *top;
  struct pos_t *bottom;
}pos_t;

void write_file(int c, int t) {

  FILE *file = fopen("file.txt", "a");
  // int s=0;
  // int d=0;
  // if(d=1){printf("write1");}
  // if (file == NULL) {
  //     printf("File does not exist\n");
  //     return;
  // }

  if(c!= 69){
    fprintf(file, "%d%d ", c, t);
  }
  else if(c == 69 && t == 69){
    fprintf(file, "\n");
    // if(d=1){printf("write3");}
    // s=1;
  }
  // if(s=0){
  //     if(d=1){printf("write4%d",c);}
  //     fprintf(file, "%d%d", c, t);
  //     fprintf(file, " ");
  // }
  // if(s=1){
  //     if(d=1){printf("write5");}
  //     fprintf(file,"\n");
  // }


  fclose(file);
}



void empty_file(char * filename){
  FILE * file = fopen(filename, "w");
  fclose(file);
}

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


  strcpy(received_message, msg->payload);
  printf("receiveved");
  flag = 1;
  newFlag = 1;

}


void addpos(pos_t **list, int x, int y, int c,int t) {
  pos_t *newnode = malloc(sizeof(pos_t));
  int s=0;
  if (newnode == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  newnode->x = x;
  newnode->y = y;
  newnode->c = c;
  newnode->t = t;
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
    if(s == 1){printf("hello1");}
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
void mountain(pos_t **list, int beginx, int beginy, int endx,int endy) {
  if(beginy != endy){printf("erorr in mountain function\n");}
  int s = 1;
  if(s==1){printf("hello69m");}
  if(endx>beginx){//checks directions :right
    if(s==1){printf("hello69R");}
    for(int i = endx-beginx;i > 0;i--){
      if(s==1){printf("hello69RR");}
      addpos(list,beginx++,beginy,4,9);
    }
  }
  if(endx<beginx ){//left
    beginx--;
    if(s==1){printf("hello69L");}
    for(int j = beginx-endx;j > 0;j--){
      if(s==1){printf("hello69LL");}
      addpos(list,beginx--,endy,4,9);
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
    printf(" %d%d",temp->c,temp->t);
    write_file(temp->c,temp->t);
    temp=temp->bottom;
    if(s==1){printf("hello3");}
  }
  if(xnode->next!=NULL){
    printf("\n");
    write_file(69,69);
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
  int x=0;
  int y=0;
  int c=0;
  int t=0;
  int rc;
  char cmd;
  struct mosquitto * mosq;
  int tempx=-1;
  int tempy=-1;
  empty_file("file.txt");
  int s = 0;//for debugging

  //cmd = 'c';
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
  printf("Got here");
  mosquitto_loop_start(mosq);

  printf("Aftet loop start");

  while(1){

  // Main program loop
  sleep(1); // Run for 10 seconds
    if(flag == 1){
      printf("Received message is %s\n", received_message);


      x = received_message[0] - '0';
      y = received_message[1] - '0';
      c = received_message[2] - '0';
      t = received_message[3] - '0';
      if(c==4 && t==9){
        printf("hhh");
        if(tempx==-1 && tempy==-1){
          tempx = x;
          tempy = y;
        printf("jkgjg");
        }
        else{
          mountain(&list,tempx,tempy,x,y);//filling the mountain
          if(s==1){printf("hello69\n");}
          tempx=-1;
          tempy=-1;
        }
      }

      flag = 0;
    }

      if(newFlag == 1){
      addpos(&list,x,y,c,t);
      //printf("Received: %d %d %s", x, y, &c);
      empty_file("file.txt");
      printgrid(&list,0);
      newFlag = 0;
      }


  }


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
