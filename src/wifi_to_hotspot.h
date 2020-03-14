#ifndef wifi_to_hotspot_h
#define wifi_to_hotspot_h

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "FS.h"
//#include "ESP8266HTTPClient.h"
#include <utility>
#define WIFI_SAVE_FILE "/wifi_config.txt"
#define WEBPAGE "/wifi_page.html"
using std::pair;
using std::make_pair;

extern ESP8266WebServer server;
extern String wifi_string;

class FileSystem
{
public:
    FileSystem();
    static bool write_fs(const char *, String, const char *);
    static String read_fs(const char *);
};

class wifi_to_hotspot : FileSystem
{
public:
    wifi_to_hotspot();
    static void config(const char *, const char *);
    static void main_page();
    static void get_data();
    static void notfound();
    static pair<const char *, const char *> get_connected_wifi(uint16_t, void (*)());
    static bool connect_to_wifi(const char *, const char *, uint16_t, void (*)(), bool *);
    static bool check_internet();
};

#endif
