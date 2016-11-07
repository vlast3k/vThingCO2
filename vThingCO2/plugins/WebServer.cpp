#include "plugins\WebServer.hpp"
#include "MenuHandler.hpp"
#include <Timer.h>
#include <ESP8266WebServer.h>
#include <ESP8266NetBIOS.h>
#include <ESP8266mDNS.h>

const char* apiKey = "Qw8rdb20aV";
//http://randomkeygen.com/


WebServerClass::WebServerClass() {
  registerPlugin(this);
}

void WebServerClass::setup(MenuHandler *handler) {
  handler->registerCommand(new MenuEntry(F("webserver_start"), CMD_BEGIN, WebServerClass::cmdStartWebServer, F("webserver_start")));
}

void WebServerClass::cmdStartWebServer(const char *ignore) {
  WebServer.cmdStartWebServerInst();
}
void WebServerClass::loop() {
  if (server != NULL) server->handleClient();
}

void WebServerClass::cmdStartWebServerInst() {
  if (server != NULL) delete server;
  if (waitForWifi() != WL_CONNECTED) return;
  server = new ESP8266WebServer(80);

  server->on("/", WebServerClass::onCommand);
  server->begin();

  Serial.print("\n\nOpen http://");
  Serial.print(WiFi.localIP());
  Serial.println("/ in your browser to see it working");

  String hostname = PropertyList.readProperty(PROP_ESP_HOSTNAME);
  if (hostname.length() == 0) hostname = "vthing";
  Serial << "Web Server accessible on :" << endl;
  Serial << "   http://" << WiFi.localIP() << endl;
  if (MDNS.begin(hostname.c_str())) {
    Serial << "   http://" << hostname << ".local/" << endl;
    MDNS.addService("http", "tcp", 80);
  }
  NBNS.begin(hostname.c_str());
  Serial << "   http://" << hostname << "/" << endl;

}

void WebServerClass::onCommand() {
  WebServer.onCommandInst();
}

void WebServerClass::onCommandInst() {
  // if (!server.hasArg("key") || server.arg("key") != apiKey) {
  //   server.send(401, "text/plain", "missing key");
  //   return;
  // }

  if (server->hasArg("cmd")) {
    menuHandler.scheduleCommand(server->arg("cmd").c_str());
  }

  server->send(200, "text/plain", "Received");
}