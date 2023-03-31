/*
 *  main.ino
 *
 *  Created on: 06/09/2022
 *      Author: Muhammad Danish
 */

#include "esp_camera.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include "ESP32_MailClient.h"
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>


//------------------WIFI---------------
// REPLACE WITH YOUR NETWORK CREDENTIALS
//const char* ssid = "RCAI";
//const char* password = "RCAIned@123";
const char* ssid = "Extensity";
const char* password = "password1";
#define NWT_TIMEOUT 1*60*1000 //trying for 1 min 

//------------------------------------




//--------------------------ESP-CAM-----------
//sending picture to gmail at particular time interval
//unsigned long tm_now = -5 * 60 * 1000;
//unsigned long Alert_tm = 5 * 60 * 1000; //1 min * 60 second * 1000 ms = 1 min
// ledPin refers to ESP32-CAM GPIO 4 (flashlight)
#define Network_led 2
#define FLASH_GPIO_NUM 4

// To send Emails using Gmail on port 465 (SSL), you need to create an app password: https://support.google.com/accounts/answer/185833
#define emailSenderAccount    "engrmuhammaddanish001@gmail.com"
#define emailSenderPassword   "ntcxyfmngcwwoxay"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "ESP32-CAM Photo Captured"
#define emailRecipient        "smartdanish96@gmail.com"
//#define emailRecipient        "engrmuhammaddanish001@gmail.com"
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#else
#error "Camera model not selected"
#endif

bool run_mode = false; //use for capture image or not  
// The Email Sending data object contains config and data to send
SMTPData smtpData;

// Photo File Name to save in SPIFFS
#define FILE_PHOTO "/photo.jpg"
//---------------------------------------------------------


//---------------------------------DEEP-SLEEP------------------
//deep sleep variable
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30*60    /* set to 30 min: Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

/*
  Method to print the reason by which ESP32
  has been awaken from sleep
*/
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

//----------------------------------------------------------------------

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  // initialize digital pin ledPin as an output
  //  pinMode(FLASH_GPIO_NUM, PULL_UP);
  pinMode(Network_led,OUTPUT);
  
  Serial.begin(115200);
  Serial.println();

  Serial.print(millis());
  Serial.println("ms: start Time");
  //------------DEEP SLEEP MODE----------

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  /*
    First we configure the wake up source
    We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                 " Seconds");

  //------------DEEP SLEEP MODE END------------------

  //--------------------WIFI-CONNECTIVITY-----------
  //Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED && millis() <= NWT_TIMEOUT) { //if wifi not found then trying for 1 min
    digitalWrite(Network_led,!digitalRead(Network_led));
    Serial.print(".");
    delay(500);
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("WIFI NOT FOUND");
    digitalWrite(Network_led,HIGH);
    run_mode = LOW;
  } else { 
    // Print ESP32 Local IP Address
    Serial.print("IP Address: http://");
    Serial.println(WiFi.localIP());
    digitalWrite(Network_led,LOW);
    run_mode = HIGH;
  }
  Serial.println();

  //--------------------WIFI-CONNECTIVITY-END-----------

  if(run_mode){ //IF RUN MODE IS ACTIVATE THEN CAPTURE THE IMAGE 
  //-------------------ESP-CAM-MODE------------
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }



  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    //    config.frame_size = FRAMESIZE_UXGA;
    //    config.jpeg_quality = 10;
    //    config.fb_count = 2;
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  //-------------------ESP-CAM-MODE-END------------
  
  //------------CAPTURED IMAGE SEND TO SERVER----------
  delay(200);  
  //    digitalWrite(FLASH_GPIO_NUM, HIGH);
  capturePhotoSaveSpiffs();
  //    digitalWrite(FLASH_GPIO_NUM, LOW);
  sendPhoto();

  //------------CAPTURED IMAGE SERVER END-----------
  }


  //------------DEEP SLEEP MODE----------


  /*
    Now that we have setup a wake cause and if needed setup the
    peripherals state in deep sleep, we can now start going to
    deep sleep.
    In the case that no wake up sources were provided but deep
    sleep was started, it will sleep forever unless hardware
    reset occurs.
  */
  Serial.println("Going to sleep now");
  digitalWrite(Network_led,LOW);
  delay(200);
  Serial.flush();
  //  digitalWrite(FLASH_GPIO_NUM, LOW);
  esp_deep_sleep_start();
  Serial.println("This will never be printed");

  //------------DEEP SLEEP MODE END------------------



}

void loop() {

}

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Photo file name
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
}

void sendPhoto( void ) {
  // Preparing email
  Serial.println("Sending email...");
  // Set the SMTP Server Email host, port, account and password
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);

  // Set the sender name and Email
  smtpData.setSender("ESP32-CAM-D1", emailSenderAccount);

  // Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("High");

  // Set the subject
  smtpData.setSubject(emailSubject);

  // Set the email message in HTML format
  smtpData.setMessage("<h2>Photo captured with ESP32-CAM-DEVICE-1 and attached in this email.</h2>", true);
  // Set the email message in text format
  //smtpData.setMessage("Photo captured with ESP32-CAM and attached in this email.", false);

  // Add recipients, can add more than one recipient
  smtpData.addRecipient(emailRecipient);
  //smtpData.addRecipient(emailRecipient2);

  // Add attach files from SPIFFS
  smtpData.addAttachFile(FILE_PHOTO, "image/jpg");
  // Set the storage type to attach files in your email (SPIFFS)
  smtpData.setFileStorageType(MailClientStorageType::SPIFFS);

  smtpData.setSendCallback(sendCallback);

  // Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData)){
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
  }else{
     for(int i=0; i<2 ; i++){
      digitalWrite(Network_led,HIGH);
      delay(250);
      digitalWrite(Network_led,LOW);
      delay(250);
     }
  }
  // Clear all data from Email object to free memory
  smtpData.empty();

  //delete created file
   if(SPIFFS.remove(FILE_PHOTO)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    } 
}

// Callback function to get the Email sending status
void sendCallback(SendStatus msg) {
  //Print the current status
  Serial.println(msg.info());
}
