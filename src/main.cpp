#include "wifi_to_hotspot.h"
#include <WiFiClientSecure.h>
#include "MQTT.h"
#include <ArduinoJson.h>
#include "sim.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "rgbled.h"

using BearSSL::X509List;
using BearSSL::PrivateKey;

#define MQTT_HOST "a1lgz1948lk3nw-ats.iot.ap-south-1.amazonaws.com"
#define MQTT_PORT 8883
#define THINGNAME "test_1"
#define SET_WILL "device/test_1/state"

#define SUB_GET_ACCEPTED "$aws/things/test_1/shadow/get/accepted"
#define SUB_UPDATE_DELTA "$aws/things/test_1/shadow/update/delta"
#define SUB_SIM_GET "device/test_1/sim_command"

#define PUB_GET "$aws/things/test_1/shadow/get"
#define PUB_UPDATE "$aws/things/test_1/shadow/update"
#define PUB_SENSOR "device/test_1/sensor"
#define PUB_SIM_POST "device/test_1/sim_response"

#define WIFI_SSID "DEVICE_test_1"
#define WIFI_PASSWORD "DEVICE_test_1"

#define STATE_TAG "state"
#define REPORTED_TAG "reported"
#define DELTA_TAG "delta"
#define CONNECTED_TAG "connected"
#define AUTO_NUM_TAG "auto_num"
#define MAN_NUM_TAG "man_num"
#define ADMIN_NUM_TAG "admin_num"
#define MODE_TAG "mode"
#define DEVICE_ID_TAG "deviceId"
#define USSD_TAG "USSD"
#define CALL_TAG "CALL"
#define SIGNAL_TAG "SIGNAL"
#define SIM_TAG "OPERATOR"
#define SIM "sim_card"
#define CALL_STATE_TAG "call_mode"
#define MSG_STATE_TAG "msg_mode"
#define WIFI_SSID_TAG "wifi_ssid"
#define WIFI_PASSWORD_TAG "wifi_password"
#define LAST_RESET_TAG "last_reset"
#define SIM_COMMAND_TAG "command"
#define WHITELIST_TAG "whitelist"
#define IMEI_TAG "imei"
#define IMSI_TAG "imsi"
#define NRF_ADDRESS_TAG "nrf_address"

#define AUTO_NUM_FILE "/auto_num.txt"
#define MAN_NUM_FILE "/man_num.txt"
#define ROOT_CA "/test_1/ROOT_CA.txt"
#define CLIENT_CRT "/test_1/certificate.crt"
#define KEY "/test_1/private.key"

DynamicJsonDocument receive_doc(2048);
DynamicJsonDocument send_doc(1024);

FileSystem F_sys;
wifi_to_hotspot wifispot;
MQTTClient client;
GSM sim;
WiFiClientSecure net;
RF24 radio(16, 15);

pair<const char *, const char *> connected_wifi;
bool sensor_mode = true;
bool sim_state = false;
bool call_state = true;
bool msg_state = true;
const byte addresses[][6] = {"D001B"};

void connect_MQTT();
void connect_internet();
void set_will();
void emergency_loop(const char *, const char *);
void sim_check();
void nrf_check();
void _loop(uint16_t, bool (*func)());
bool client_loop();
bool server_handleClient();

void messageReceived(String &topic, String &payload)
{
  // Serial.println(topic);
  //Serial.println(payload);
  sim.send_at("at+sttone=1,16,500", 1, false);
  if (topic == SUB_GET_ACCEPTED || topic == SUB_UPDATE_DELTA)
  {
    deserializeJson(receive_doc, payload);
    String _state = receive_doc[STATE_TAG];
    deserializeJson(receive_doc, _state);
    if (topic == SUB_GET_ACCEPTED)
    {
      String _desired = receive_doc[DELTA_TAG];
      deserializeJson(receive_doc, _desired);
    }
    JsonObject reported = send_doc.createNestedObject(STATE_TAG).createNestedObject(REPORTED_TAG);
    reported[CONNECTED_TAG] = "true";
    reported[DEVICE_ID_TAG] = THINGNAME;
    reported[WIFI_SSID_TAG] = connected_wifi.first;
    reported[WIFI_PASSWORD_TAG] = connected_wifi.second;
    reported[SIM] = String(sim_state);
    reported[LAST_RESET_TAG] = String(millis() / 1000) + " sec ago due to " + ESP.getResetReason();

    if (receive_doc[CALL_STATE_TAG].size() > 0)
    {
      if (receive_doc[CALL_STATE_TAG][0] == "OFF")
      {
        call_state = false;
      }
      else if (receive_doc[CALL_STATE_TAG][0] == "ON")
      {
        call_state = true;
      }
    }
    JsonArray call_state_json = reported.createNestedArray(CALL_STATE_TAG);
    if (call_state)
    {
      call_state_json.add("ON");
    }
    else
    {
      call_state_json.add("OFF");
    }

    if (receive_doc[MSG_STATE_TAG].size() > 0)
    {
      if (receive_doc[MSG_STATE_TAG][0] == "OFF")
      {
        msg_state = false;
      }
      else if (receive_doc[MSG_STATE_TAG][0] == "ON")
      {
        msg_state = true;
      }
    }
    JsonArray msg_state_json = reported.createNestedArray(MSG_STATE_TAG);
    if (msg_state)
    {
      msg_state_json.add("ON");
    }
    else
    {
      msg_state_json.add("OFF");
    }

    if (receive_doc[AUTO_NUM_TAG].size() > 0)
    {
      JsonArray auto_num_json = reported.createNestedArray(AUTO_NUM_TAG);
      if (F_sys.write_fs(AUTO_NUM_FILE, receive_doc[AUTO_NUM_TAG], "w"))
      {
        for (uint8_t i = 0; i < receive_doc[AUTO_NUM_TAG].size(); i++)
        {
          auto_num_json.add(receive_doc[AUTO_NUM_TAG][i]);
        }
      }
      else
      {
        auto_num_json.add("erroe_adding_number_to_fs");
      }
    }

    if (receive_doc[MAN_NUM_TAG].size() > 0)
    {
      JsonArray man_num_json = reported.createNestedArray(MAN_NUM_TAG);
      if (F_sys.write_fs(MAN_NUM_FILE, receive_doc[MAN_NUM_TAG], "w"))
      {
        for (uint8_t i = 0; i < receive_doc[MAN_NUM_TAG].size(); i++)
        {
          man_num_json.add(receive_doc[MAN_NUM_TAG][i]);
        }
      }
      else
      {
        man_num_json.add("erroe_adding_number_to_fs");
      }
    }

    if (receive_doc[ADMIN_NUM_TAG].size() > 0)
    {
      JsonArray admin_num_json = reported.createNestedArray(ADMIN_NUM_TAG);
      for (uint8_t i = 0; i < receive_doc[ADMIN_NUM_TAG].size(); i++)
      {
        sim.whitelist(receive_doc[ADMIN_NUM_TAG][i], i + 1);
        admin_num_json.add(receive_doc[ADMIN_NUM_TAG][i]);
      }
      for (uint8_t i = receive_doc[ADMIN_NUM_TAG].size(); i < 3; i++)
      {
        sim.clear_whitelist(i + 1);
      }
    }

    if (receive_doc[MODE_TAG].size() > 0)
    {
      if (receive_doc[MODE_TAG][0] == "AUTO")
      {
        sensor_mode = true;
      }
      if (receive_doc[MODE_TAG][0] == "MANUAL")
      {
        sensor_mode = false;
      }
    }
    JsonArray mode_json = reported.createNestedArray(MODE_TAG);
    if (sensor_mode)
    {
      mode_json.add("AUTO");
    }
    else
    {
      mode_json.add("MANUAL");
    }

    payload = "";
    serializeJson(send_doc, payload);
    //Serial.println(payload);
    client.publish(PUB_UPDATE, payload, 0, 0);
  }
  if (topic == SUB_SIM_GET)
  {
    deserializeJson(receive_doc, payload);
    if (receive_doc[USSD_TAG].size() > 0)
    {
      send_doc[USSD_TAG] = sim.ussd_at(receive_doc[USSD_TAG][0], 2);
    }
    if (receive_doc[CALL_TAG].size() > 0)
    {
      send_doc[CALL_TAG] = sim.call(receive_doc[CALL_TAG][0], 1, client_loop);
    }
    if (receive_doc[SIM_COMMAND_TAG].size() > 0)
    {
      if (receive_doc[SIM_COMMAND_TAG][0] == SIGNAL_TAG)
      {
        send_doc[SIGNAL_TAG] = sim.send_at("AT+CSQ", 1, false);
      }
      else if (receive_doc[SIM_COMMAND_TAG][0] == SIM_TAG)
      {
        send_doc[SIM_TAG] = sim.send_at("AT+CSPN?", 1, false);
      }
      else if (receive_doc[SIM_COMMAND_TAG][0] == WHITELIST_TAG)
      {
        send_doc[WHITELIST_TAG] = sim.send_at("AT+CWHITELIST?", 1, false);
      }
      else if (receive_doc[SIM_COMMAND_TAG][0] == IMEI_TAG)
      {
        send_doc[IMEI_TAG] = sim.send_at("AT+GSN", 1, false);
      }
      else if (receive_doc[SIM_COMMAND_TAG][0] == IMSI_TAG)
      {
        send_doc[IMSI_TAG] = sim.send_at("AT+CIMI", 1, false);
      }
    }
    payload = "";
    serializeJson(send_doc, payload);
    //Serial.println(_data);
    client.publish(PUB_SIM_POST, payload, 0, 0);
  }
  send_doc.clear();
  receive_doc.clear();
}
void setup()
{
  radio.begin();
  radio.setDataRate(RF24_2MBPS);
  radio.enableDynamicPayloads();
  radio.enableAckPayload();
  radio.enableDynamicAck();
  radio.openReadingPipe(1, addresses[0]);
  radio.startListening();
  rgb_mode.blue();
  WiFi.mode(WIFI_OFF);
  SPIFFS.begin();
  Serial.begin(9600);
  sim.check();
  set_will();
  client.onMessage(messageReceived);
  wifispot.config(WIFI_SSID, WIFI_PASSWORD);
}

void loop()
{
  if (client.connected())
  {
    sim_check();
    _loop(30, client_loop);
  }
  else
  {
    connect_internet();
    _loop(10, server_handleClient);
  }
  sim.inbox(&sensor_mode);
}
bool client_loop()
{
  delay(10);
  return client.loop();
}
bool server_handleClient()
{
  server.handleClient();
  delay(10);
  return true;
}
void _loop(uint16_t t, bool (*func)())
{
  uint64_t m = millis();
  while (millis() - m <= t * 1000)
  {
    if (!func())
    {
      return;
    }
    ESP.wdtFeed();
    nrf_check();
  }
}
void connect_MQTT()
{
  X509List *cert = new X509List(F_sys.read_fs(ROOT_CA).c_str());
  X509List *client_crt = new X509List(F_sys.read_fs(CLIENT_CRT).c_str());
  PrivateKey *key = new PrivateKey(F_sys.read_fs(KEY).c_str());
  net.setTrustAnchors(cert);
  net.setClientRSACert(client_crt, key);
  client.begin(MQTT_HOST, MQTT_PORT, net);
  configTime(1, 0, "pool.ntp.org");
  uint8_t sntp_count = 0;
  while (time(nullptr) < 1578150958)
  {
    if (sntp_count == 9)
    {
      ESP.restart();
    }
    delay(1000);
    sntp_count++;
  }
 // Serial.println(ESP.getFreeHeap());
  if (client.connect(THINGNAME))
  {
    //Serial.println("connected!");
    if (!client.subscribe(SUB_GET_ACCEPTED) || !client.subscribe(SUB_UPDATE_DELTA) || !client.subscribe(SUB_SIM_GET))
    {
      //Serial.println("ERROR");
      ESP.restart();
    }
    else
    {
      rgb_mode.green();
      client.publish(PUB_GET, "", 0, 0);
    }
  }
  else
  {
    //Serial.print("SSL Error Code: ");
    //Serial.println(client.lastError());
    ESP.restart();
  }
  delete cert;
  delete client_crt;
  delete key;
}
void connect_internet()
{
  rgb_mode.blue();
  connected_wifi = wifispot.get_connected_wifi(10, nrf_check);
//  Serial.println(connected_wifi.first);
 // Serial.println(connected_wifi.second);
  if (connected_wifi.first == "")
  {
    if (WiFi.getMode() != WIFI_AP_STA)
    {
     // Serial.println("hotspot");
      WiFi.mode(WIFI_AP_STA);
    }
  }
  else
  {
    /*Serial.print("conected  wifi= ");
    Serial.print(connected_wifi.first);
    Serial.print("  password= ");
    Serial.println(connected_wifi.second);*/
    connect_MQTT();
  }
}
void set_will()
{
  String will;
  JsonObject reported = send_doc.createNestedObject(STATE_TAG).createNestedObject(REPORTED_TAG);
  reported[CONNECTED_TAG] = "false";
  reported[DEVICE_ID_TAG] = THINGNAME;
  serializeJson(send_doc, will);
  client.setWill(SET_WILL, will.c_str(), 0, 0);
  send_doc.clear();
}
void emergency_loop(const char *filename, const char *sensor_name)
{
  String content;
  send_doc[DEVICE_ID_TAG] = THINGNAME;
  send_doc["time"] = time(nullptr);
  send_doc["sensor"] = sensor_name;
  serializeJson(send_doc, content);
  deserializeJson(receive_doc, F_sys.read_fs(filename));
  if (client.publish(PUB_SENSOR, content, 0, 1))
  {
    if (call_state)
    {
      for (uint8_t i = 1; i < receive_doc.size(); i++)
      {
        if (sim.call(receive_doc[i], 30, client_loop))
        {
          break;
        }
      }
    }
    if (msg_state)
    {
      for (uint8_t i = 1; i < receive_doc.size(); i++)
      {
        sim.msg(receive_doc[i], 60, content.c_str(), client_loop);
      }
    }
  }
  else
  {
    for (uint8_t i = 0; i < receive_doc.size(); i++)
    {
      if (sim.call(receive_doc[i], 30, server_handleClient))
      {
        break;
      }
    }
    for (uint8_t i = 1; i < receive_doc.size(); i++)
    {
      sim.msg(receive_doc[i], 60, content.c_str(), server_handleClient);
    }
  }
  send_doc.clear();
  receive_doc.clear();
  sim.inbox(&sensor_mode);
}
void sim_check()
{
  bool temp = sim.check_sim();
  if (temp != sim_state)
  {
    sim_state = temp;
    String response;
    JsonObject reported = send_doc.createNestedObject(STATE_TAG).createNestedObject(REPORTED_TAG);
    reported[SIM] = String(sim_state);
    serializeJson(send_doc, response);
    client.publish(PUB_UPDATE, response, 0, 1);
    send_doc.clear();
  }
}
void nrf_check()
{
  if (radio.available())
  {
    char text[4];
    radio.read(&text, sizeof(text));
    if (text[1] == 'T')
    {
      if (sensor_mode)
      {
        emergency_loop(AUTO_NUM_FILE, "button");
      }
      else
      {
        emergency_loop(MAN_NUM_FILE, "button");
      }
    }
    else if (text[1] == 'B')
    {
      String response;
      JsonObject reported = send_doc.createNestedObject(STATE_TAG).createNestedObject(REPORTED_TAG);
      reported["BUTTON"] = '"' + *((uint16_t *)&text[2]) + '"';
      serializeJson(send_doc, response);
      client.publish(PUB_UPDATE, response, 0, 0);
      send_doc.clear();
    }
  }
  delay(10);
}
