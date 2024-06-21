#include <libpynq.h>
#include <iic.h>
#include "vl53l0x.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pulsecounter.h>
#include <uart.h>
#include <limits.h>
#include <gpio.h>
#include <stepper.h>
#include <platform.h>
#include <math.h>
#include "tcs3200.h"
#include "measurements.h"

#define S0 4
#define S1 5
#define S2 9
#define S3 10
#define OUT 8
#define UART_PORT 0
#define  Left_sensor IO_AR6
#define  Right_sensor IO_AR7
#define  Moving_distance 64
#define TENcm 636
#define FIVEcm 320 
#define ONEcm 64
#define HALFturn 1210
#define QAURTERturn 645 //605 old value
#define EIGTHturn 302 
#define Robot 1 // 1 for left and 0 for right side 
#define Robot_width 15;

static uint16_t r_max = 0, g_max = 0, b_max = 0;

void tcs3200_read_20(){
   // Setting frequency-scaling to 20%
    gpio_set_level(TCS3200_S0, GPIO_LEVEL_HIGH);
    gpio_set_level(TCS3200_S1, GPIO_LEVEL_LOW);
}

void tcs3200_init(){
  gpio_set_direction(TCS3200_S0, GPIO_DIR_OUTPUT);
  gpio_set_direction(TCS3200_S1, GPIO_DIR_OUTPUT);
  gpio_set_direction(TCS3200_S2, GPIO_DIR_OUTPUT);
  gpio_set_direction(TCS3200_S3, GPIO_DIR_OUTPUT);
  gpio_set_direction(TCS3200_OUT, GPIO_DIR_INPUT);
  tcs3200_read_20();
  printf("Setup complete\n");
}

void tcs3200_stop_all() {
  gpio_set_level(TCS3200_S0, GPIO_LEVEL_LOW);
  gpio_set_level(TCS3200_S1, GPIO_LEVEL_LOW);
  gpio_set_level(TCS3200_S2, GPIO_LEVEL_LOW);
  gpio_set_level(TCS3200_S3, GPIO_LEVEL_LOW);
}

unsigned long tcs3200_pulseIn(const io_t pin) {
    unsigned long width = 0;
    unsigned long numloops = 0;
    unsigned long maxloops = ULONG_MAX;

    while (gpio_get_level(pin) == GPIO_LEVEL_HIGH) {
        if (numloops++ == maxloops)
            return 0;
    }

    while (gpio_get_level(pin) == GPIO_LEVEL_LOW) {
        if (numloops++ == maxloops)
            return 0;
    }

    while (gpio_get_level(pin) == GPIO_LEVEL_HIGH) {
        if (numloops++ == maxloops)
            return 0;
        width++;
    }

    return width;
}

void tcs3200_read_colors(uint16_t* p_red_level, uint16_t* p_green_level, uint16_t* p_blue_level) {
  #define FREQ_FACTOR 100000

  tcs3200_read_20();
  sleep_msec(100);

  // read red colour
  gpio_set_level(TCS3200_S2, GPIO_LEVEL_LOW);
  gpio_set_level(TCS3200_S3, GPIO_LEVEL_LOW);
  *p_red_level = FREQ_FACTOR / tcs3200_pulseIn(TCS3200_OUT);
  sleep_msec(200);  

  //read green colour
  gpio_set_level(TCS3200_S2, GPIO_LEVEL_HIGH);
  gpio_set_level(TCS3200_S3, GPIO_LEVEL_HIGH);
  *p_green_level = FREQ_FACTOR / tcs3200_pulseIn(TCS3200_OUT);
  sleep_msec(200);

  //read blue colour
  gpio_set_level(TCS3200_S2, GPIO_LEVEL_LOW);
  gpio_set_level(TCS3200_S3, GPIO_LEVEL_HIGH);
  *p_blue_level = FREQ_FACTOR / tcs3200_pulseIn(TCS3200_OUT);
  sleep_msec(200);

  tcs3200_stop_all();
}

void tcs3200_calibrate_white() {
  tcs3200_read_colors(&r_max, &g_max, &b_max);
  //printf("MAX VALUES => r: %d, g: %d, b: %d\n\n", r_max, g_max, b_max);
}

tcs3200_color_t tcs3200_read_color(void) {
  uint16_t r, g, b;
  uint16_t r_mapped, g_mapped, b_mapped;

  tcs3200_read_colors(&r, &g, &b);
  //printf("\nr: %d, g: %d, b: %d\n", r, g, b);

  r_mapped = map(r, 90, r_max, 0, 255);
  g_mapped = map(g, 80, g_max, 0, 255);
  b_mapped = map(b, 100, b_max, 0, 255);
  //printf("AFTER MAP => r: %d, g: %d, b: %d\n", r_mapped, g_mapped, b_mapped);

  tcs3200_color_t color = TCS3200_UNDEF;
    //int black = (r_mapped + g_mapped + b_mapped)/3;
    //printf("black value is :%d\n", black);

  	 if ((r_mapped + g_mapped + b_mapped)/3 < 30) {
    color = TCS3200_BLACK;
  }
  else if (r_mapped > 1.5*(g_mapped + b_mapped)/2) {
    color = TCS3200_RED;
  }
  else if (g_mapped > 1.5*(r_mapped + b_mapped)/2) {
    color = TCS3200_GREEN;
  }
  else if (b_mapped > 1.5*(r_mapped + g_mapped)/2) {
    color = TCS3200_BLUE;
  }

  

  return color;
}

int rewrite_coordinate(float x){
  int x_out=round(x*100);
  int x_print=x_out/15;
  return x_print;
}

int send(vl53x sensorB, float x, float y){
	int iDistance = 0;

  int x_new = rewrite_coordinate(x);
  int y_new = rewrite_coordinate(y);

  

  uint8_t payload[4];
  payload[0] = x_new;
  payload[1] = y_new;

	tcs3200_color_t color = tcs3200_read_color();

	if( color != 4 ){
		if(color == 1){
			payload[2] = 1;
      printf("Sending red, ");
		}
		else if(color == 2){
			payload[2] = 2;
      printf("Sending green, ");
		}
		else if(color == 3){
			payload[2] = 3;
      printf("Sending blue, ");
		}
		else if(color == 0){
			payload[2] = 0;
      printf("Sending black, \n");
		}
		
		iDistance = tofReadDistance(&sensorB);
		
		if( iDistance < 100 ){
			payload[3] = 6;
      printf("6x6\n");
		}
		else{
			payload[3] = 3;
      printf("3x3\n");
		}
	}
  else{
    printf("Sending no block\n");
    payload[2] = 7;
    payload[3] = 7;
  }

  uint32_t payload_size = sizeof(payload);
  uint8_t *byte_length = (uint8_t*)&payload_size;

  printf("Sending %d bytes\n",payload_size);
  printf("Payload: %s\n", payload);

  for(uint32_t i = 0; i<4;  i++)
  {
    //uart_send(UART1, byte_length[i]);
    printf("%d", byte_length[i]);
  }
  printf(", ");
  for(uint32_t i = 0; i < payload_size; i++)
  {
    //uart_send(UART1, payload[i]);
    printf("%d", payload[i]);
  }
  printf("\n\n");
  return 1;
}

void send_cliff(float x, float y){
  int x_new = rewrite_coordinate(x);
  int y_new = rewrite_coordinate(y);

  uint8_t payload[4] = {'\0'};
  payload[0] = x_new;
  payload[1] = y_new;

  payload[2] = 0;
  payload[3] = 8;

  uint32_t payload_size = sizeof(payload);
  uint8_t *byte_length = (uint8_t*)&payload_size;

  printf("Sending %d bytes\n",payload_size);
  printf("%s\n", payload);

  for(uint32_t i = 0; i<4;  i++)
  {
    //uart_send(UART1, byte_length[i]);
    printf("%d", byte_length[i]);
  }
  printf(", ");
  for(uint32_t i = 0; i < payload_size; i++)
  {
    //uart_send(UART1, payload[i]);
    printf("%d", payload[i]);
  }
  printf("\n");
}

void send_mountain(float x, float y){
  int x_new = rewrite_coordinate(x);
  int y_new = rewrite_coordinate(y);

  uint8_t payload[4] = {'\0'};
  payload[0] = x_new;
  payload[1] = y_new;

  payload[2] = 4;
  payload[3] = 9;

  uint32_t payload_size = sizeof(payload);
  uint8_t *byte_length = (uint8_t*)&payload_size;

  printf("Sending %d bytes\n",payload_size);
  printf("%s\n", payload);

  for(uint32_t i = 0; i<4;  i++)
  {
    uart_send(UART1, byte_length[i]);
    printf("%d", byte_length[i]);
  }
  printf(", ");
  for(uint32_t i = 0; i < payload_size; i++)
  {
    uart_send(UART1, payload[i]);
    printf("%d", payload[i]);
  }
  printf("\n");
}

void move( int cmd ){
  switch(cmd){

      case 2: // move backward
          stepper_steps(ONEcm,ONEcm);
        break;

      case 4: //move left 
          stepper_steps(QAURTERturn,-QAURTERturn);
          stepper_steps(-FIVEcm,-FIVEcm);
          break;

      case 6: // move right
          stepper_steps(-QAURTERturn,QAURTERturn);
          stepper_steps(-FIVEcm,-FIVEcm);
          break;

      case 7: // move to left corner
          stepper_steps(EIGTHturn,-EIGTHturn);
          stepper_steps(-FIVEcm*1.414,-FIVEcm*1.414);
          sleep_msec(1000);
          stepper_steps(-EIGTHturn,EIGTHturn);
        break;

      case 8: //move forward
          stepper_steps(-ONEcm*0.1,-ONEcm*0.1);
        break;

      case 9: //move right corner 
          stepper_steps(-EIGTHturn,EIGTHturn);
          stepper_steps(-FIVEcm*1.414,-FIVEcm*1.414);
          sleep_msec(1000);
          stepper_steps(EIGTHturn,-EIGTHturn);
        break;
  }
  return;
}

float follow_line2(int side){ // on which side of the robot the line is 0 is left 1 is right 
  float length=0;
  if (side==0){
    //int i_line=0;
    int i_lost=0;
    //int direction=2; // 2 undefined, 0 going downwards on the line, 1 going upwards on the line. 
    //int temp_direction=2;
    while(gpio_get_level(Right_sensor)==0){
      if(gpio_get_level(Left_sensor)==1){
        stepper_steps(-ONEcm,-ONEcm);
        sleep_msec(3.3*ONEcm);
        length=length+0.01;
      }
      if(gpio_get_level(Left_sensor)==0){
        i_lost=0;
        int count=0;
          while (i_lost==0){
            if(gpio_get_level(Left_sensor)==1){
              i_lost=1;
              //direction=temp_direction;
            }
            else if(gpio_get_level(Left_sensor)==0 && count<5){
              stepper_steps(0,-ONEcm*0.5);
              sleep_msec(100);
              count=count+1;
              //temp_direction=0;
            }
            else if(gpio_get_level(Right_sensor)==0 && count>=5 && count<15){
              stepper_steps(-ONEcm*0.5,0);
              sleep_msec(100);
              count=count+1;
              //temp_direction=1;
            }
            else if(count>=15){
              printf("error\n");
              count=0;
            }
          }
      }
    }
    if(gpio_get_level(Right_sensor)==1){
      printf("length=%f\n",length);
    }
  }
  if (side==1){
  // int i_line=0;
    int i_lost=0; 
    //int temp_direction=2;
    while(gpio_get_level(Left_sensor)==0){
      if(gpio_get_level(Right_sensor)==1){
        stepper_steps(-ONEcm,-ONEcm);
        sleep_msec(3.3*ONEcm);
        length=length+0.0108;
      }
      if(gpio_get_level(Right_sensor)==0){
        printf("Lost the line\n");
        i_lost=0;
        int count=0;
          while (i_lost==0){
            if(gpio_get_level(Right_sensor)==1){
              i_lost=1;
              printf("found line\n");
              //direction=temp_direction;
            }
            else if(gpio_get_level(Right_sensor)==0 && count<10){
              stepper_steps(0,-ONEcm*0.5);
              sleep_msec(100);
              count=count+1;
              //temp_direction=0;
            }
            else if(gpio_get_level(Right_sensor)==0 && count>=10 && count<30){
              stepper_steps(-ONEcm*0.5,0);
              sleep_msec(100);
              count=count+1;
              //temp_direction=1;
            }
            else if(count>=15){
              printf("error\n");
              count=0;
            }
          }
        }
      } 
  }
    length=length+0.18;
    return length;
}

void find_border(){
  int i =0;
  int R =0;
  int L =0;
  int i2=1;
  while (i==0){ // driving up to the border
    if(gpio_get_level(Left_sensor)==1||gpio_get_level(Right_sensor)==1){ //determining which sensor hit the border 
      if (gpio_get_level(Left_sensor)==1&&gpio_get_level(Right_sensor)==0){
        //printf("Black tape detected at right sensor\n");
        L=1; 
        R=0;
        i2=0;
      }
      else if(gpio_get_level(Right_sensor)==1&&gpio_get_level(Left_sensor)==0){ 
        //printf("Black tape detected at left sensor\n");
        R=1;
        L=0;
        i2=0;
      }
      else if(gpio_get_level(Right_sensor)==0&&gpio_get_level(Left_sensor)==0){
        printf("Black tape detected at left & right sensor\n");
        L=1;
        R=1;
        i2=0;
      }
      i=i+1;
    }
    else{
      move(8);
    }
  }
  
  while(i2==0){ //rotating such that both sensors are up to the border 
    if(L==1 && R==1){
      i2=1;
    }
    if(L==1 && R==0){
      int i3=0;
      while(i3==0){
        stepper_steps(0,-ONEcm*0.2);
        if(gpio_get_level(Right_sensor)==1){
          R=1;
          i3=1;
          int i7=0;
          while(i7==0){
            if(gpio_get_level(Left_sensor)==1){
              i7=1;
            }
            else{
            stepper_steps(ONEcm*0.2,0);
            }
          }
        }
      }
    }
    if(L==0 && R==1){
      int i4=0;
      while(i4==0){
        stepper_steps(-ONEcm*0.2,0);
        if(gpio_get_level(Left_sensor)==1){
          L=1;
          i4=1;
          int i7=0;
          while(i7==0){
            if(gpio_get_level(Right_sensor)==1){
              i7=1;
            }
            else{
            stepper_steps(0,ONEcm*0.2);
            }
          }
        }
      }
    }
  }

}

void quarter_turn(int robot_side){
  sleep_msec(100);
  stepper_steps(-ONEcm*2.05,-ONEcm*2.05);
  sleep_msec(100);
  if(robot_side==0){
    stepper_steps(QAURTERturn,-QAURTERturn);
  }
  else if(robot_side==1){
    stepper_steps(-QAURTERturn,QAURTERturn);
  }
  sleep_msec(2000);
}

void quarter_turn2(int robot_side){
  sleep_msec(100);
  sleep_msec(100);
  if(robot_side==0){
    stepper_steps(QAURTERturn,-QAURTERturn);
  }
  else if(robot_side==1){
    stepper_steps(-QAURTERturn,QAURTERturn);
  }
  sleep_msec(1000);
}

int detect_border(float length, float x, int direction){ // returns 0 when it detects a cliff and returns 1 when it detects the border
    int r=2;
    float remained_length=length-x; //meters
    if(gpio_get_level(Left_sensor)==1||gpio_get_level(Right_sensor)==1){
      //printf("Remained length:%f\n",remained_length);
      if(direction==1){
       if(remained_length>0.15){
          printf("Detected cliff, x=%f\n",x);
           r=0;
        }
       else{
            r=1;
          printf("Detected border\n");
        }
      }
      else{
        if(x>0.15){
          printf("Detected cliff, x=%f\n",x);
           r=0;
        }
       else{
            r=1;
          printf("Detected border\n");
        }
      }
    }
    return r;
}

int detect_mountain(float length, float x,int direction,vl53x sensorA){
    int remained_length=length*1000-x*1000; // milimeters
    int distance=tofReadDistance(&sensorA); // milimeters
    //printf("Distance measured: %d\n",distance);
    int r=2;
    if (direction==1){
      if(distance<=remained_length && distance<=300){
          r=1;
      }
      else{
        r=0;
      }
    }
    else{
      if(x>0.35 && distance<=300){
        r=1;
      }
      else{
        r=0;
      }
    }
    return r;
}
int detect_block(){

  //int iDistance = 0;

  tcs3200_color_t color = tcs3200_read_color();

	if( color != 4 ){
		/*if(color == 1){
			printf("Detected red\n");
		}
		else if(color == 2){
			printf("Detected green\n");
		}
		else if(color == 3){
			printf("Detected blue\n");
		}
		else if(color == 0){
			printf("Detected Black\n");
		}
		
		iDistance = tofReadDistance(&sensorB);
		
		if( iDistance < 100 ){
			printf("Detected 6x6\n");	
		}
		else{
			printf("Detected 3x3\n");
		}*/
	}
  else{
    //printf("Detected no block\n");
    return 0;
  }
  return 1;
}

float avoid_mountain( float x, int direction, vl53x sensorA){
  uint32_t distance=300; //meters 
  /*while(tofReadDistance(&sensorA)>distance){
    stepper_steps(-ONEcm,-ONEcm);
    sleep_msec(3.3*ONEcm);
    if (direction==1){
        x=x+0.01;    
    }
    else{
        x=x-0.01;    
    }
    
  }*/
  //printf("Direction is: %i\n",direction);
  if (direction==1){
    int i_mountain=0;
    int i_counter=1;
    while(i_mountain==0){
        quarter_turn2(0);
        sleep_msec(1000);
        stepper_steps(-ONEcm*5,-ONEcm*5);
        sleep_msec(3.3*ONEcm*5);
        quarter_turn2(1);
        sleep_msec(2000);
        uint32_t measured=tofReadDistance(&sensorA);
        //printf("Measured in while loop: %d\n",measured);
        if(measured>distance){
           i_mountain=1;
                    
        }
        else{
        i_counter=i_counter+1;
        }
    }
    //printf("Left while loop\n");
    quarter_turn2(0);
    sleep_msec(1000);
    stepper_steps(-ONEcm*10,-ONEcm*10);
    sleep_msec(ONEcm*10*3.3);
    quarter_turn2(1);
    sleep_msec(1000);
    stepper_steps(-ONEcm*31,-ONEcm*31);
    sleep_msec(3.3*31*ONEcm);
    x=x+0.31;
    int i_mountain2=0;
    while(i_mountain2==0){
        quarter_turn2(1);
        sleep_msec(1000);
        if(tofReadDistance(&sensorA)>distance){
           i_mountain2=1;         
        }
        else{
          quarter_turn2(0);
          sleep_msec(1000);
          stepper_steps(-ONEcm*5,-ONEcm*5);
          sleep_msec(3.3*ONEcm*5);
          x=x+0.05;
        }
            
    }
    //printf("Left second while loop\n");
    sleep_msec(1000);
    quarter_turn2(0);
    sleep_msec(1000);
    stepper_steps(-ONEcm*12,-ONEcm*12);
    x=x+0.12;
    sleep_msec(3.3*10*ONEcm);
    quarter_turn2(1);
    sleep_msec(1000);
    stepper_steps(-ONEcm*10,-ONEcm*10);
    sleep_msec(ONEcm*10*3.3);
    stepper_steps(-ONEcm*5*(i_counter+1),-ONEcm*5*(i_counter+1));
    sleep_msec(ONEcm*5*(i_counter+1)*3.3);
    quarter_turn2(0);
    sleep_msec(1000);
  }
  else{
      int i_mountain=0;
      int i_counter=1;
    while(i_mountain==0){
        quarter_turn2(1); 
        sleep_msec(1000);
        stepper_steps(-ONEcm*5,-ONEcm*5);
        sleep_msec(3.3*ONEcm*5);
        quarter_turn2(0);
        sleep_msec(2000);
        uint32_t measured=tofReadDistance(&sensorA);
        //printf("Measured in while loop: %d\n",measured);
        if(measured>distance){
           i_mountain=1;
                    
        }
        else{
        i_counter=i_counter+1;
        }
    }
    //printf("Left while loop\n");
    quarter_turn2(1);
    sleep_msec(1000);
    stepper_steps(-ONEcm*12,-ONEcm*12);
    sleep_msec(ONEcm*10*3.3);
    quarter_turn2(0);
    sleep_msec(1000);
    stepper_steps(-ONEcm*31,-ONEcm*31);
    sleep_msec(3.3*31*ONEcm);
    x=x-0.31;
    int i_mountain2=0;
    while(i_mountain2==0){
        quarter_turn2(0);
        sleep_msec(2000);
        if(tofReadDistance(&sensorA)>distance){
           i_mountain2=1;         
        }
        else{
          quarter_turn2(1);
          sleep_msec(1000);
          stepper_steps(-ONEcm*5,-ONEcm*5);
          sleep_msec(3.3*ONEcm*5);
          x=x-0.05;
        }
            
    }
    //printf("Left second while loop\n");
    sleep_msec(1000);
    quarter_turn2(1);
    sleep_msec(1000);
    stepper_steps(-ONEcm*12,-ONEcm*12);
    x=x-0.12;
    sleep_msec(3.3*10*ONEcm);
    quarter_turn2(0);
    sleep_msec(1000);
    stepper_steps(-ONEcm*10,-ONEcm*10);
    sleep_msec(ONEcm*10*3.3);
    stepper_steps(-ONEcm*5*(i_counter+1),-ONEcm*5*(i_counter+1));
    sleep_msec(ONEcm*5*(i_counter+1)*3.3);
    quarter_turn2(1);
    sleep_msec(1000);
     
  }
  printf("x:%f\n",x);
  return x;
}

float avoid_cliff( float x, float y, int direction){
  send_cliff(x, y);
  //printf("Direction is: %i\n",direction);
  stepper_steps(ONEcm*10,ONEcm*10);
  sleep_msec(1000);
  if (direction==1){
    x=x-0.10;
    int i_cliff=0;
    int i_counter=1;
    int i_counter2=0;
    while(i_cliff==0){
        quarter_turn2(0);
        sleep_msec(2000);
        stepper_steps(-ONEcm*5,-ONEcm*5);
        sleep_msec(3.3*ONEcm*5);
        quarter_turn2(1);
        sleep_msec(2000);
        i_counter=1;
          while(i_counter<=25){
             if(gpio_get_level(Left_sensor)==1||gpio_get_level(Right_sensor)==1){
                i_counter=30;   
             }
             else{
             stepper_steps(-ONEcm,-ONEcm);
             x=x+0.01;
             sleep_msec(ONEcm*3.3);
             i_counter=i_counter+1;
             }
          }
           if(i_counter==26){
           i_cliff=1;
           }
           else{
            stepper_steps(ONEcm*5,ONEcm*5);
            sleep_msec(ONEcm*3.3*5);
            x=x-0.05;
           }
        i_counter2=i_counter2+1;
    }
        quarter_turn2(0);
        sleep_msec(2000);
        stepper_steps(-ONEcm*5,-ONEcm*5);
        sleep_msec(3.3*ONEcm*5);
      quarter_turn2(1);
      sleep_msec(2000);
      quarter_turn2(1);
      sleep_msec(2000);
      int i_counter3=i_counter2*5+5;
    while(i_counter3>0){
        if(gpio_get_level(Right_sensor)==1){
            stepper_steps(ONEcm*5,ONEcm*5);
            sleep_msec(ONEcm*5*3.3);
            i_counter3=i_counter3+5;
            quarter_turn2(0);
            sleep_msec(2000);
            stepper_steps(-ONEcm*5,-ONEcm*5);
            sleep_msec(ONEcm*5*3.3);
            x=x+0.05;
            quarter_turn2(1); 
            sleep_msec(2000);
        }
        else{
            stepper_steps(-ONEcm,-ONEcm);
            sleep_msec(ONEcm*3.3);
            i_counter3=i_counter3-1;
        }
    }
    quarter_turn2(0);
    sleep_msec(2000);
   }
    else{
    x=x+0.10;
    int i_cliff=0;
    int i_counter=1;
    int i_counter2=0;
    while(i_cliff==0){
        quarter_turn2(1);
        sleep_msec(2000);
        stepper_steps(-ONEcm*5,-ONEcm*5);
        sleep_msec(3.3*ONEcm*5);
        quarter_turn2(0);
        sleep_msec(2000);
        i_counter=1;
          while(i_counter<=25){
             if(gpio_get_level(Left_sensor)==1||gpio_get_level(Right_sensor)==1){
                i_counter=30;   
             }
             else{
             stepper_steps(-ONEcm,-ONEcm);
             x=x-0.01;
             sleep_msec(ONEcm*3.3);
             i_counter=i_counter+1;
             }
            }
           if(i_counter==26){
           i_cliff=1;
           }
           else{
            stepper_steps(ONEcm*5,ONEcm*5);
            sleep_msec(ONEcm*3.3*5);
            x=x+0.05;
           }
        i_counter2=i_counter2+1;
    }
        quarter_turn2(1);
        sleep_msec(2000);
        stepper_steps(-ONEcm*5,-ONEcm*5);
        sleep_msec(3.3*ONEcm*5);
      quarter_turn2(0);
      sleep_msec(2000);
      quarter_turn2(0);
      int i_counter3=i_counter2*5+5;
    while(i_counter3>0){
        if(gpio_get_level(Left_sensor)==1){
            stepper_steps(ONEcm*5,ONEcm*5);
            sleep_msec(ONEcm*5*3.3);
            i_counter3=i_counter3+5;
            quarter_turn2(1);
            sleep_msec(2000);
            stepper_steps(-ONEcm*5,-ONEcm*5);
            sleep_msec(ONEcm*5*3.3);
            x=x-0.05;
            quarter_turn2(0); 
            sleep_msec(2000);
        }
        else{
            stepper_steps(-ONEcm,-ONEcm);
            sleep_msec(ONEcm*3.3);
            i_counter3=i_counter3-1;
        }
    }
    quarter_turn2(1);
    sleep_msec(2000);
   }
    send_cliff(x,y);
    return x;
}
void flick(int direction){
  if( direction == 1 ){
    stepper_set_speed(40000,40000);
    sleep_msec(2000);
    stepper_enable();
    stepper_steps(3*ONEcm,3*ONEcm);
    sleep_msec(3.3*ONEcm);
    stepper_set_speed(15000,15000);
    stepper_steps(QAURTERturn,-QAURTERturn);
    sleep_msec(1000);
    stepper_steps(-QAURTERturn,QAURTERturn);
    sleep_msec(1000);
    stepper_set_speed(40000,40000);
    stepper_steps(-3*ONEcm,-3*ONEcm);
    sleep_msec(3.3*ONEcm);
  }
  else{
    stepper_set_speed(40000,40000);
    sleep_msec(2000);
    stepper_enable();
    stepper_steps(3*ONEcm,3*ONEcm);
    sleep_msec(3.3*ONEcm);
    stepper_set_speed(15000,15000);
    stepper_steps(-QAURTERturn,QAURTERturn);
    sleep_msec(1000);
    stepper_steps(QAURTERturn,-QAURTERturn);
    sleep_msec(1000);
    stepper_set_speed(40000,40000);
    stepper_steps(-3*ONEcm,-3*ONEcm);
    sleep_msec(3.3*ONEcm);
  }
}
int *rewrite_coordinates(float x, float y){
  int x_out=round(x*100);
  int x_print=x_out/15;
  int y_out=round(y*100);
  int y_print=y_out/15;
  static int coordinates[2];
  coordinates[0]=x_print;
  coordinates[1]=y_print;
  return coordinates;

}
float find_border_zigzag(float x,float y,int direction,float length,vl53x sensorA, vl53x sensorB){ 
  int count=1;
  int block_help = 0;
   while(detect_border(length,x,direction)!=1){
        if(detect_border(length,x,direction)==0){
        x=avoid_cliff(x,y,direction);
        }
        else if(detect_mountain(length,x,direction,sensorA)==1){
        printf("Detected mountain\n");
        x=avoid_mountain(x,direction,sensorA);
        }
        else if(count%5==0){
          if(detect_block() == 1 && block_help != 2){
            block_help++;
          }
          else if( block_help == 2 ){
            send(sensorB, x, y);
            flick(direction);
            block_help = 0;
          }
          else if(count%15==0){
            send(sensorB, x, y);
          }
          else{
            block_help = 0;
          }
        }
        else{
            stepper_steps(-ONEcm,-ONEcm);
            sleep_msec(3*ONEcm);
            if (direction==0){
                x=x-0.01;
            }
            else if(direction==1){
                x=x+0.01;
            }
        }
        count++;
    }
    find_border();
   
  return x;
}

void zigzag(float length_side, float length, vl53x sensorA, vl53x sensorB){
  sleep_msec(100);
  stepper_steps(ONEcm*15,ONEcm*15);
  sleep_msec(3.3*15*ONEcm);
  quarter_turn(1);
  sleep_msec(100);
  printf("Length side used: %f\n",length_side);
  int turns= ceil(length_side*100/15-2);
  printf("Amount of turns: %i\n",turns);
  int i_loop=1;
  float x=0.15;
  float y=0.075;
  while(i_loop<=turns){
    if(i_loop%2==0){
    sleep_msec(2000);
    x=find_border_zigzag(x,y,0,length,sensorA, sensorB);
    quarter_turn(0);
    stepper_steps(-ONEcm*15,-ONEcm*15);
    sleep_msec(3.3*15*ONEcm);
    y=y+0.15;
    quarter_turn(0);
    x=x+0.15;
    i_loop=i_loop+1;
    }
    else{
    sleep_msec(2000);
    x=find_border_zigzag(x,y,1,length,sensorA, sensorB);
    quarter_turn(1);
    stepper_steps(-ONEcm*15,-ONEcm*15);
    sleep_msec(3.3*15*ONEcm);
    y=y+0.15;
    quarter_turn(1);
    x=x-0.15;
    i_loop=i_loop+1;
    }
  }
}

int main(void) {
  pynq_init();

  // start of setup motors
    gpio_init();
    gpio_set_direction(Left_sensor, GPIO_DIR_INPUT);
    gpio_set_direction(Right_sensor,GPIO_DIR_INPUT);
    stepper_init();
    stepper_enable();
    stepper_set_speed(40000,40000);
    float distance_long_side=0;
  // end of setup motors

  //start of setup distance sensor

	  //Setting up the buttons & LEDs
	  //Init the IIC pins
    switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
    switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
    iic_init(IIC0);
    iic_reset(IIC0);

        int i;
        //Setup Sensor A
        printf("Initialising Sensor A:\n");

        //Change the Address of the VL53L0X
        uint8_t addrA = 0x69;
        i = tofSetAddress(IIC0, 0x29, addrA);
        printf("---Address Change: ");
        if(i != 0)
        {
          printf("Fail\n");
          return 1;
        }
        printf("Succes\n");
        
        i = tofPing(IIC0, addrA);
        printf("---Sensor Ping: ");
        if(i != 0)
        {
          printf("Fail\n");
          return 1;
        }
        printf("Succes\n");

        //Create a sensor struct
        vl53x sensorA;

        //Initialize the sensor

        i = tofInit(&sensorA, IIC0, addrA, 0);
        if (i != 0)
        {
          printf("---Init: Fail\n");
          return 1;
        }

        uint8_t model, revision;

        tofGetModel(&sensorA, &model, &revision);
        printf("---Model ID - %d\n", model);
        printf("---Revision ID - %d\n", revision);
        printf("---Init: Succes\n");
        fflush(NULL);

        printf("\n\nNow Power Sensor B!!\nPress \"Enter\" to continue...\n");
        getchar();

        //Setup Sensor B
        printf("Initialising Sensor B:\n");

        //Use the base addr of 0x29 for sensor B
        //It no longer conflicts with sensor A.
        uint8_t addrB = 0x29;	
        i = tofPing(IIC0, addrB);
        printf("---Sensor Ping: ");
        if(i != 0)
        {
          printf("Fail\n");
          return 1;
        }
        printf("Succes\n");

        //Create a sensor struct
        vl53x sensorB;

        //Initialize the sensor

        i = tofInit(&sensorB, IIC0, addrB, 0);
        if (i != 0)
        {
          printf("---Init: Fail\n");
          return 1;
        }

        tofGetModel(&sensorB, &model, &revision);
        printf("---Model ID - %d\n", model);
        printf("---Revision ID - %d\n", revision);
        printf("---Init: Succes\n");
        fflush(NULL); //Get some output even if the distance readings hang
        printf("\n");
        
        //uint32_t iDistance;

  //end of setup distance sensor

  //start of setup color sensor
    tcs3200_init();
    tcs3200_calibrate_white();

  //end of setup color sensor

  //start of setup communication
    switchbox_set_pin(IO_AR0, SWB_UART1_RX);
    switchbox_set_pin(IO_AR1, SWB_UART1_TX);

    uart_init(UART1);
    uart_reset_fifos(UART1);
  //end of setup communication
  //printf("going into find border\n");
  find_border();
  quarter_turn(Robot);
  follow_line2(0);
  quarter_turn(Robot);
  float length=follow_line2(0);
  quarter_turn(Robot);
  distance_long_side=follow_line2(0);
  zigzag(distance_long_side,length,sensorA, sensorB);


    




	iic_destroy(IIC0);
	pynq_destroy();
	return EXIT_SUCCESS;
}
