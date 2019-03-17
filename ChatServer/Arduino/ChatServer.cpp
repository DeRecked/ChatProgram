#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>

#define BUFFER_LENGTH 512

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 10, 2);
IPAddress myDns(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(49152);
EthernetClient clients[4];

void setup()
{
	// initialize the Ethernet device
	Ethernet.begin(mac, ip, myDns, gateway, subnet);
	// start listening for clients
	server.begin();
	// Open serial communications and wait for port to open:
	Serial.begin(9600);
	
	while (!Serial) {}
	Serial.print("Chat server address:");
	Serial.println(Ethernet.localIP());
}

void loop()
{
	// wait for a new client:
	EthernetClient client = server.available();

	// when the client sends the first byte, say hello:
	if (client)
	{
		boolean newClient = true;
		for (byte i = 0; i < 4; i++)
		{
			//check whether this client refers to the same socket as one of the existing instances:
			if (clients[i] == client)
			{
				newClient = false;
				break;
			}
		}

		if (newClient)
		{
			//check which of the existing clients can be overridden:
			for (byte i = 0; i < 4; i++)
			{
				if (!clients[i] && clients[i] != client)
				{
					clients[i] = client;
					// clear out the input buffer:
					client.flush();
					Serial.println("We have a new client");
					break;
				}
			}
		}

		if (client.available() > 0)
		{
			// read the bytes incoming from the client:
			char charBuff = client.read();
			
			// echo the bytes back to all other connected clients:
			for (byte i = 0; i < 4; i++)
			{
				if (clients[i] && (clients[i] != client))
				{
					clients[i].write(charBuff);
				}
			}
			// echo the bytes to the server as well:
			Serial.write(charBuff);
		}
	}
	for (byte i = 0; i < 4; i++)
	{
		if (!(clients[i].connected()))
		{
			// client.stop() invalidates the internal socket-descriptor, so next use of == will allways return false;
			clients[i].stop();
		}
	}
}
