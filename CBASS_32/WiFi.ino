

IPAddress connectAsAccessPoint() {
  // JSR: specify the AP's IP
  IPAddress gateway_ip = IPAddress(192, 168, 111, 22);  // Not sure what this is!
  IPAddress local_ip = IPAddress(192, 168, 111, 1);     // The address to connect to.
  IPAddress subnet = IPAddress(255, 255, 255, 0);
  // With the settings above the first client gets address 192.168.111.2 and the web page is at 111.1.
  WiFi.softAPConfig(local_ip, gateway_ip, subnet);

  // You can remove the password parameter if you want the AP to be open.
  // a valid password must have more than 7 characters
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    while (1)
      ;
  }

  Serial.printf("Connect to WiFi %s and use URL http://%s", ssid, WiFi.softAPIP());

  return WiFi.softAPIP();
}

IPAddress connectAsStation() {
  WiFi.setHostname("CBASS");
  Serial.printf("Connecting to '%s' with '%s'\n", ssid, password);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.printf("\nConnected to WiFi\n\nConnect to server at %s:%d\n", WiFi.localIP().toString().c_str(), port);

  return WiFi.localIP();
}

IPAddress connectWiFi() {
#if WIFIMODE == WIFIAP
  WiFi.mode(WIFI_AP);
  return connectAsAccessPoint();
#else
  WiFi.mode(WIFI_STA);
  return connectAsStation();
#endif
}
