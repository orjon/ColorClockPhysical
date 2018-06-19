

// Possible commands are listed here:
//
// "digital/13"     -> digitalRead(13)
// "digital/13/1"   -> digitalWrite(13, HIGH)
// "toggle/13"      -> toggles pin 13
// "analog/2/123"   -> analogWrite(2, 123)
// "analog/2"       -> analogRead(2)
// "mode/13/input"  -> pinMode(13, INPUT)
// "mode/13/output" -> pinMode(13, OUTPUT)

#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>

// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
BridgeServer server;

void setup() {
  Serial.begin(9600);
  Serial.println("startup");

  // Bridge startup
  pinMode(13,OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);

  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();
}

void loop() {
  // Get clients coming from server
  BridgeClient client = server.accept();
  Bridge.put("Hello", " fucker");
  // There is a new client?
  if (client) {
    // Process request
    process(client);

    // Close connection and free resources.
    client.stop();
  }

  delay(500); // Poll every 50ms
}

void process(BridgeClient client) {
  // read the command
  String command = client.readStringUntil('/');

  // is "digital" command?
  if (command == "digital") {
    digitalCommand(client);
  }

  // is "toggle" command?
  if (command == "toggle") {
    toggleCommand(client);
  }

  // is "red" command?
  if (command == "red") {
    redCommand(client);
  }





}

void toggleCommand(BridgeClient client) {
  int pin, value;

  // Read pin number
  pin = client.parseInt();
  value = !digitalRead(pin); //read pin value and invert it
  digitalWrite(pin, value);  //write this inverted value to the pin

  // Send feedback to client
  client.print(F("Pin D"));
  client.print(pin);
  client.print(F(" set to "));
  client.println(value);

  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
}

void redCommand(BridgeClient client) {
  int pin = 3;
  int value;
  float brightness;

  // Read brightness
  brightness = client.parseInt();
  value = 255 - 255*(brightness/100);
  pinMode(pin, OUTPUT);
  analogWrite(pin, value);

  // Send feedback to client
  client.print(F("Pin D"));
  client.print(pin);
  client.print(F(" set to analog "));
  client.println(value);

  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
}

void digitalCommand(BridgeClient client) {
  int pin, value;

  // Read pin number
  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/13/1"
  if (client.read() == '/') {
    value = client.parseInt();
    digitalWrite(pin, value);
  } 
  else {
    value = digitalRead(pin);
  }

  // Send feedback to client
  client.print(F("Pin D"));
  client.print(pin);
  client.print(F(" set to "));
  client.println(value);

  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
}


