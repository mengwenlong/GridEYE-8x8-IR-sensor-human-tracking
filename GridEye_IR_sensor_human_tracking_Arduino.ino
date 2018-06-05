/*********************************************
Code library from AMG88xx GridEYE 8x8 IR is included.
https://github.com/adafruit/Adafruit_AMG88xx

This code sketch tries to read temperatures at 8x8 pixels from low-cost GridEYE IR sensor,
detect "hot" areas, and determine whether there is a human at some distance (e.g. 1m to 3m)
and track huamn movement. Similary, when a hand waves in front of the IR sensor
(e.g. 0.1m to 0.5m), it detects hand motion.

GridEYE sensors use I2C protocol to communicate with Arduino.

Any Ardunio can be used after this sensor is wired to I2C of specific Arduino model
(e.g. for Arduino Teensy 3.2, SCL is pin 19, SDA is pin 18
https://www.pjrc.com/teensy/td_libs_Wire.html)

Written by Wenlong Meng, June 1, 2018

Feel free to re-use and modify these codes in your owm projects.
Comments and advice are welcomed!
**********************************************/
#include <Wire.h>
#include <WireKinetis.h>
#include <Adafruit_AMG88xx.h>
Adafruit_AMG88xx amg;
#include <struct_grideye_pbai.h> // Create and put this header file into Arduino libraries folder on your computer


#define noise_threshold 1.5 // an example value of background noise
#define motion_resolution 0.5 // adjust this value based on
// (1) how far humans are from the sensor (2) how fast humans move (3) how often you would like to check (e.g. every 0.5s)
#define coordinate_size 8 // 8x8 IR sensor pixels
#define hotPoint_size 64


float pixels[AMG88xx_PIXEL_ARRAY_SIZE];
float background_pixels_xy[coordinate_size][coordinate_size];
float before_pixels_xy [coordinate_size][coordinate_size];
float after_pixels_xy[coordinate_size][coordinate_size];
float difference[coordinate_size][coordinate_size]; // compare temperatue change between 'before' case and 'after' case
float sum_difference;
float average_difference;
int i, j, k;

float background_sum_pixels_xy[8][8];
float background_average_pixels_xy[8][8];
int background_count = 0;

Point_type hotPoint[hotPoint_size];
/* start of block

// define a 'struct' variable to record the 2-D coordinates of hot pixels
// remember to put this code block into a header file called <struct_grideye_pbai.h>,
// and put it into the library folder of Arduino

typedef struct{
  int x;
  int y;
}Point_type;
Point_type hotpoint[64];

*/
//end of block


// To be simple yet reasonable, just assume a human have same thermal features in different body parts, so we treat all hot pixels EQUALLY hot.
// Acutal tempertures vary. e.g. different part of human clothes have different thermal characteristics.

void setup() {
    Serial.begin(9600);

    bool status;
    status = amg.begin();
    if (!status) {
        Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
        while (1);
    }

    Serial.println("GridEye starts working");
    delay(3000); // let the sensor has sufficient time to boot up and become stable


// CAREFUL: if the background temperature is suddenly changed when Arduino is working,
// then remember to reboot Arduino (generally there is a rebooting button on most Arudnio)
// so a new background temperature reference can be obtained.
// Later on, this code sketch will be improved so that Arduino can automatically
// detect and update siginficant background temperature changes.


 // read temperatues at pixels
   amg.readPixels(pixels);

// get the average temperatures of multiple iterations as background values
// check and delete ab-normal room temperatures, e.g. less than 10 degrees or greater than 50
// bugs needs to be fixed here. For now, only use last iteration of background_pixles_xy[j][k]

 for (background_count = 1; background_count <= 5;)
 {
  for (i=1; i<=AMG88xx_PIXEL_ARRAY_SIZE; i++)
  {
    j = (i-1)/8;
    k = (i-1)%8;
    if (pixels[i-1] >=10 && pixels[i-1] <= 50) // case: normal temperature
    {
      background_pixels_xy[j][k] = pixels[i-1];
      background_sum_pixels_xy[j][k] = background_sum_pixels_xy[j][k] + background_pixels_xy[j][k];
      background_average_pixels_xy[j][k] = background_sum_pixels_xy[j][k]/background_count;
      background_count ++;
    }
    else
    {  //case: ab-normal reading, skip the whole iteration, not only one bad pixel
      Serial.print("bad reading, skip to next loop");
      background_count--; // reduce count by 1
      break;
    }

    }

   }

Serial.println("Average background temperatures:");
  for (j=0; j <=7; j++)
  {
    for (k=0;k<=7;k++)
    {
      Serial.print(background_pixels_xy[j][k]);
      // for now, not use 'average' as some bugs still exist
      Serial.print(",");
      if( k == 7 ) Serial.println();
     }
  }

} // end of setup()


void loop() {

 amg.readPixels(pixels);

 // Record the starting time and end time, just to check time cost
 // There is delay(1000), so remember to subtract 1000 ms
  unsigned long start_time =  millis();

  Serial.println();
  //Serial.println("Temperature of new frame:");
  Serial.println("The'before' temperatures:");

  for(i=1; i<=AMG88xx_PIXEL_ARRAY_SIZE; i++){
  j = (i-1)/8;
  k = (i-1)%8;
  before_pixels_xy[j][k] = pixels[i-1]; // convert 1-D array [64] to 2-D array[8][8]

  Serial.print(before_pixels_xy[j][k]);
  Serial.print(",");
  if( i%8 == 0 ) Serial.println();

  }

//read pixels in a new moment. Example delay is 0.5s as interval between 2 detections
  delay (500);
  amg.readPixels(pixels);

Serial.println("The 'after' temperatures:");
for(int i=1; i<=AMG88xx_PIXEL_ARRAY_SIZE; i++){
  j = (i-1)/8;
  k = (i-1)%8;
  after_pixels_xy[j][k] = pixels[i-1];

  Serial.print(after_pixels_xy[j][k]);
  Serial.print(",");
  if( i%8 == 0 ) Serial.println();
}

// Algorithm 1: directly compare 'after' case and 'before' case about temperature change at same pixels
// Algorithm 2: (1) compare 'before' case with backgroud values, calculate difference to check if a human exists or not, calculate center of 'hot' area.
// (2) do same to compare 'after' case with background values (3) compare hot area center of 'before' case and 'after' case.
// Algorithm 2 is chosen below.

sum_difference = 0;
int count_actuated = 0; // only count 'hot' (i.e. actuated) pixels
int a=0; // use 'a' as 1-D array index, and record matching coordinates (x,y) of hot pixels
float sum_x = 0;
float sum_y = 0;
float average_x = 0;
float average_y = 0;
float before_average_x = 0;
float after_average_x = 0;
float before_average_y = 0;
float after_average_y = 0;

// "before" case
for(j = 0; j <= 7; j++){
  for (k=0;k<=7;k++){

// check 4 neighboring pixels
    difference[j][k] = before_pixels_xy[j][k] - background_pixels_xy[j][k];
    difference[j][(k-1)>=0?(k-1):0] = before_pixels_xy[j][(k-1)>=0?(k-1):0] - background_pixels_xy[j][(k-1)>=0?(k-1):0]; // conditional operator
    difference[j][(k+1)<=7?(k+1):7] = before_pixels_xy[j][(k+1)<=7?(k+1):7] - background_pixels_xy[j][(k+1)<=7?(k+1):7];
    difference[(j+1)<=7?(j+1):7][k] = before_pixels_xy[(j+1)<=7?(j+1):7][k] - background_pixels_xy[(j+1)<=7?(j+1):7][k];
    difference[(j-1)>=0?(j-1):0][k] = before_pixels_xy[j][(j-1)>=0?(j-1):0] - background_pixels_xy[(j-1)>=0?(j-1):0][k];


      if(difference[j][k] >= noise_threshold){
        // check 4 neighboring pixels, e.g. at least 3 out 4 should also be 'hot' pixels. This helps remove isolated/false hot noise
          if((difference[j][(k-1)>=0?(k-1):0] >= noise_threshold) + (difference[j][(k+1)<=7?(k+1):7] >= noise_threshold) + (difference[(j+1)<=7?(j+1):7][k] >= noise_threshold) + (difference[(j-1)>=0?(j-1):0][k] >=noise_threshold) >= 3)
          {
          sum_difference = sum_difference + difference[j][k];
          count_actuated ++;

          hotPoint[a].x = j;
          hotPoint[a].y = k;

          sum_x = sum_x + hotPoint[a].x;
          average_x = sum_x/count_actuated;
          sum_y = sum_y + hotPoint[a].y;
          average_y = sum_y/count_actuated;

          a++;

          }
      }

   }

}

before_average_x = average_x; // keep a copy here for 'before' case
before_average_y = average_y;

Serial.print("Hot center for 'before' case:");
Serial.print("(");
Serial.print(average_x);
Serial.print(",");
Serial.print(average_y);
Serial.println(")");


// "after" case
sum_difference = 0;
count_actuated = 0;
a=0;
sum_x = 0;
sum_y = 0;
average_x =0;
average_y =0;

for(j=0;j<= 7;j++){
 for(k =0;k<=7;k++){
    difference[j][k] = after_pixels_xy[j][k] - background_pixels_xy[j][k];
    difference[j][(k-1)>=0?(k-1):0] = after_pixels_xy[j][(k-1)>=0?(k-1):0] - background_pixels_xy[j][(k-1)>=0?(k-1):0];
    difference[j][(k+1)<=7?(k+1):7] = after_pixels_xy[j][(k+1)<=7?(k+1):7] - background_pixels_xy[j][(k+1)<=7?(k+1):7];
    difference[(j+1)<=7?(j+1):7][k] = after_pixels_xy[(j+1)<=7?(j+1):7][k] - background_pixels_xy[(j+1)<=7?(j+1):7][k];
    difference[(j-1)>=0?(j-1):0][k] = after_pixels_xy[j][(j-1)>=0?(j-1):0] - background_pixels_xy[(j-1)>=0?(j-1):0][k];

      if (difference[j][k] >= noise_threshold){
       if((difference[j][(k-1)>=0?(k-1):0] >=noise_threshold) + (difference[j][(k+1)<=7?(k+1):7] >= noise_threshold) + (difference[(j+1)<=7?(j+1):7][k] >=noise_threshold) + (difference[(j-1)>=0?(j-1):0][k] >= noise_threshold) >= 3 )
        {
        sum_difference = sum_difference + difference[j][k];
        count_actuated ++;

        //record these coordinates of actuated pixels, then calculate the center of "hot area"
        hotPoint[a].x = j;
        hotpoint[a].y = k;
        sum_x = sum_x + hotPoint[a].x;
        average_x = sum_x/count_actuated;
        sum_y = sum_y + hotPoint[a].y;
        average_y = sum_y/count_actuated;

        a++;

        }
      }
  }
}

after_average_x = average_x;  // keep a copy for 'after' case
after_average_y = average_y;

Serial.print("Hot center for 'after' case:");
Serial.print("(");
Serial.print(after_average_x);
Serial.print(",");
Serial.print(after_average_y);
Serial.println(")");

// Compare hot center coordinates, so we know human moving direction.
// Will fix the code later because movements can be hybrid, i.e. both left-right motion and up-down motion with repect to sensor location.
// note: if a human keeps moving right, e.g. from (5,1) to (7,1), then at the edge of view, as no hot pixels exist any more, so the center will return to (0, 0).
// This is NOT "move left" case. Whenever hot center is (0,0), we should always regard it as "no humans" because the moving direction
// is not 100% uncertain, e.g. suddenly change directions at view edge and run FAST out of view.


// when hot center coordinates change little (e.g. < 1 pixel), regard it as 'no movement'
if (abs(after_average_x - before_average_x) <= motion_resolution && abs(after_average_y - before_average_y) <= motion_resolution){
     if ((after_average_x == 0 && after_average_y == 0)||(before_average_x == 0 && before_average_y == 0))
     {Serial.println("No humans any more");}
     else
     {Serial.println("Humans exist with no movement");}
}

else // human move
{
    if( (after_average_x - before_average_x) > motion_resolution){
    Serial.println("Human move right");}
    if ((after_average_x - before_average_x) < - motion_resolution){
    Serial.println("Human move left"); }
    if ((after_average_y - before_average_y) > motion_resolution){
    Serial.println("Human move up"); }
    if ((after_average_y - before_average_y) < -motion_resolution){ // e.g. <-1
    Serial.println("Human move down"); }
}


// just for checking time cost
unsigned long end_time =  millis();
unsigned long elapse_time= end_time - start_time;
Serial.print("Total time elapse is (500ms delay included): ");
Serial.println(elapse_time);


// void setMovingAverageMode(bool mode) is a function within amg library. May need it later to help remove noise.

}
// end of void loop()
