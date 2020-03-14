#include "wifi_to_hotspot.h"

ESP8266WebServer server(80);
String wifi_string;

FileSystem::FileSystem()
{
    if (!SPIFFS.begin())
    {
        ESP.restart();
    }
}
bool FileSystem::write_fs(const char *filename, String data, const char *operation)
{
    File wifi_config_file = SPIFFS.open(filename, operation);
    if (wifi_config_file.print(data))
    {
        wifi_config_file.close();
        return true;
    }
    else
    {
        wifi_config_file.close();
        return false;
    }
}
String FileSystem::read_fs(const char *filename)
{
    File wifi_config_file = SPIFFS.open(filename, "r");
    String data;
    if (wifi_config_file.available())
    {
        data = wifi_config_file.readString();
    }
    wifi_config_file.close();
    return data;
}

wifi_to_hotspot::wifi_to_hotspot()
{
    wifi_string = FileSystem::read_fs(WIFI_SAVE_FILE);
}

void wifi_to_hotspot::config(const char *ssid, const char *password)
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid, password, 1, false, 1);
    IPAddress ip(192, 168, 1, 1);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress mask(255, 255, 255, 0);
    WiFi.softAPConfig(ip, gateway, mask);
    server.on("/", HTTP_GET, main_page);
    server.on("/", HTTP_POST, get_data);
    server.onNotFound(notfound);
    server.begin();
}
void wifi_to_hotspot::main_page()
{
    uint8_t n = WiFi.scanNetworks();
    String web_page = "<div id=\"wifiList\"  class=\"wifiList\">";
    for (uint8_t i = 0; i < n; i++)
    {
        web_page += WiFi.SSID(i);
        web_page += ",";
    }
    web_page += "</div>";
    web_page += FileSystem::read_fs(WEBPAGE);
    server.send(200, "text/html", web_page);
    for (uint16_t i = 0; i <= 1000; i++)
    {
        server.handleClient();
    }
}
void wifi_to_hotspot::get_data()
{
    String data = server.arg("plain");
    if (FileSystem::write_fs(WIFI_SAVE_FILE, data, "a"))
    {
        server.send(200, "text/plain", data);
    }
    else
    {
        server.send(200, "text/plain", "ERROR! refresh page and try again");
    }
    wifi_string = FileSystem::read_fs(WIFI_SAVE_FILE);
    for (uint16_t i = 0; i <= 1000; i++)
    {
        server.handleClient();
    }
}
void wifi_to_hotspot::notfound()
{
    server.send(200, "text/html", "<a href=\"http://192.168.1.1/\">CLICK HERE</a>");
}
pair<const char *, const char *> wifi_to_hotspot::get_connected_wifi(uint16_t timeout, void (*func)())
{
    if (wifi_string.length() > 0)
    {
        char *wifi_list = strdup(wifi_string.c_str());
        char *current_ptr = strtok(wifi_list, "|");
        const char *ssid;
        const char *pass;
        uint8_t n = WiFi.scanNetworks();
        bool reset_flag = false;
        while (current_ptr != NULL)
        {
            ssid = current_ptr;
            current_ptr = strtok(NULL, "|");
            pass = current_ptr;
            for (uint8_t j = 0; j < n; j++)
            {
                /*Serial.print(WiFi.SSID(j));
                Serial.print("  ");
                Serial.print(j);
                Serial.print("  ");
                Serial.println(ssid);*/
                if (strcmp(ssid, WiFi.SSID(j).c_str()) == 0)
                {
                    if (connect_to_wifi(ssid, pass, timeout, *func, &reset_flag))
                    {
                        return make_pair(ssid, pass);
                    }
                }
            }
            current_ptr = strtok(NULL, "|");
        }
        if (reset_flag)
        {
            //ESP.restart();
            ESP.reset();
        }
        free(wifi_list);
        free(current_ptr);
    }
    return make_pair("", "");
}
bool wifi_to_hotspot::connect_to_wifi(const char *ssid, const char *pass, uint16_t timeout, void (*func)(), bool *ptr_flag)
{
    WiFi.begin(ssid, pass);
    uint64_t m = millis();
    while (millis() - m < timeout * 1000)
    {
        func();
        if (WiFi.status() == WL_CONNECTED)
        {
            if (check_internet())
            {
                //Serial.println("net");
                WiFi.mode(WIFI_STA);
                return true;
            }
            else
            {
                *ptr_flag = true;
                //Serial.println("!net");
                return false;
            }
        }
        server.handleClient();
        ESP.wdtFeed();
    }
    return false;
}
bool wifi_to_hotspot::check_internet()
{
    WiFiClient cli;
    if (cli.connect("www.google.com", 80))
    {
        cli.stop();
        return true;
    }
    cli.stop();
    return false;
}