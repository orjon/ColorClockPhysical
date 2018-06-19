#include <Bridge.h>;
#include <YunServer.h>
#include <YunClient.h>
 
 
YunServer server;
int i;
byte pinDirs[]={1,1,1,1,1,1,1,1,1,1,1,0};
byte pinVals[]={0,0,0,0,0,0,0,0,0,0,0,0};
int  anVals[]={0,0,0,0,0,0};
char* content_type="Content-type: application/json; charset=utf-8";
char* allow_type="Access-Control-Allow-Origin: *";
 
void setup() {
 
 
  pinMode(13,OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);
  server.listenOnLocalhost();
  server.begin();
  setPinDirs();
  setPinVals();
}
 
 
 
void loop() {
 
  YunClient client = server.accept();
 
 
  if (client)
  {
 
    String command;
    command = client.readStringUntil('/');
    command.trim();        
 
                Serial.println("command:"+command);
 
    if (command=="io")
    {
 
      command=client.readStringUntil('/');
      command.trim();
 
      for (i=0;i<command.length();i++)
      {
        pinDirs[i]=byte(command.charAt(i)-48);
      }
 
      client.println("Status: 200");
      client.println(content_type);
                        client.println(allow_type);
      client.println();
      client.print("{\"ret\":\"ok\"}");
      setPinDirs();
    }
 
    if (command=="do")
    {
 
      command=client.readStringUntil('/');
      command.trim();
 
      for (i=0;i<command.length();i++)
      {
        if (command.charAt(i)!='-')
        {
          pinVals[i]=byte(command.charAt(i)-48);
        }
        else
        {
          pinVals[i]=255;
        }
      }
 
 
      client.println("Status: 200");
      client.println(content_type);
                        client.println(allow_type);
      client.println();
      client.print("{\"ret\":\"ok\"}");
      setPinVals();
    }    
 
 
    if (command=="in")
    {
 
      setPinVals();
      client.println("Status: 200");
      client.println(content_type);
                        client.println(allow_type);
      client.println();
 
      client.print("{\"Datadir\" : [");
      for (i=0;i<12;i++)
      {
        client.print("{\"datadir\" : "+String(pinDirs[i])+"}");
                                Serial.println(pinDirs[i]);
        if (i<11) client.print(",");
      }
 
      client.print("],\"Digital\" : [");
      for (i=0;i<12;i++)
      {
        if(pinDirs[i]==0)
        {
          client.print("{\"dataval\" : "+String(pinVals[i])+"}");
        }
        else
        {
          client.print("{\"dataval\" : "+String(10+pinVals[i])+"}");
        }
        if (i<11) client.print(",");
                                Serial.println(pinVals[i]);
      }  
 
      client.print("],\"Analog\" : [");
      for (i=0;i<6;i++)
      {
        client.print("{\"dataval\" : "+String(anVals[i])+"}");
        if (i<5) client.print(",");
                                Serial.println(anVals[i]);
      }
      client.print("]}");
    }    
 
 
 
    client.stop();
  }
 
  delay(50); 
}
 
 
void setPinDirs()
{
  for(i=0;i<12;i++)
  {
    if (pinDirs[i]==0)  pinMode(2+i, OUTPUT);
    if (pinDirs[i]==1)  pinMode(2+i, INPUT);
    if (pinDirs[i]==2)  pinMode(2+i, INPUT_PULLUP);
  }  
}
 
 
void setPinVals()
{
  for(i=0;i<12;i++)
  {
    if (pinDirs[i]==0 && pinVals[i]==0) digitalWrite(2+i,LOW);
    if (pinDirs[i]==0 && pinVals[i]==1) digitalWrite(2+i,HIGH);    
    if (pinDirs[i]==1 || pinDirs[i]==2)
    {
      if (digitalRead(2+i)==LOW)  pinVals[i]=0;
      if (digitalRead(2+i)==HIGH)  pinVals[i]=1;
    }
  }  
 
 
  for (i=0;i<6;i++)
  {
    anVals[i]=analogRead(i);  
    delay(20);  
    anVals[i]=analogRead(i);  
  }
}
