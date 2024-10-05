#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "AsyncUDP.h"
#include "commstructs.h"
#include "udp.h"
#include "main.h"

#define UDPIP IPAddress(239, 10, 0, 1)
#define UDPPORT 16033

UDPcomm udpsync;

extern uint8_t channelList[6];
extern espSetChannelPower curChannel;

void init_udp() {
    udpsync.init();
}

void sendAvail(uint8_t wakeupReason) {
	struct espAvailDataReq eadr = {0};
	uint8_t mac[6];
	WiFi.macAddress(mac);
	memcpy(&eadr.src, mac, 6);
	eadr.adr.lastPacketRSSI = WiFi.RSSI();
	eadr.adr.currentChannel = WiFi.channel();
	eadr.adr.hwType = 0xE2;
	eadr.adr.wakeupReason = wakeupReason;
	eadr.adr.capabilities = 0;
	eadr.adr.tagSoftwareVersion = 0;
	eadr.adr.currentChannel = 1;
	eadr.adr.customMode = 0;
	udpsync.netProcessDataReq(&eadr);
}

UDPcomm::UDPcomm() {
    // Constructor
}

UDPcomm::~UDPcomm() {
    // Destructor
}

void UDPcomm::init() {
    if (udp.listenMulticast(UDPIP, UDPPORT)) {
        udp.onPacket([this](AsyncUDPPacket packet) {
            if (packet.remoteIP() != WiFi.localIP()) {
                this->processPacket(packet);
            }
        });
    }
}

void UDPcomm::processPacket(AsyncUDPPacket packet) {

    switch (packet.data()[0]) {
        case PKT_AVAIL_DATA_INFO: {
            espAvailDataReq adr;
            memset(&adr, 0, sizeof(espAvailDataReq));
            memcpy(&adr, &packet.data()[1], std::min(packet.length() - 1, sizeof(espAvailDataReq)));
            // processDataReq(&adr, false);
            break;
        }
        case PKT_XFER_COMPLETE: {
            espXferComplete xfc;
            memset(&xfc, 0, sizeof(espXferComplete));
            memcpy(&xfc, &packet.data()[1], std::min(packet.length() - 1, sizeof(espXferComplete)));
            // processXferComplete(&xfc, false);
            break;
        }
        case PKT_XFER_TIMEOUT: {
            espXferComplete xfc;
            memset(&xfc, 0, sizeof(espXferComplete));
            memcpy(&xfc, &packet.data()[1], std::min(packet.length() - 1, sizeof(espXferComplete)));
            // processXferTimeout(&xfc, false);
            break;
        }
        case PKT_AVAIL_DATA_REQ: {

            pendingData pending;
            memset(&pending, 0, sizeof(pendingData));
            memcpy(&pending, &packet.data()[1], std::min(packet.length() - 1, sizeof(pendingData)));
            prepareExternalDataAvail(&pending, packet.remoteIP());
            break;
        }
        case PKT_TAGINFO: {
            uint16_t syncversion = (packet.data()[2] << 8) | packet.data()[1];
            if (syncversion != SYNC_VERSION) {
                // wsErr("Got a packet from " + packet.remoteIP().toString() + " with mismatched udp sync version. Update firmware!");
            } else {
                TagInfo* taginfoitem = (TagInfo*)&packet.data()[1];
                // updateTaginfoitem(taginfoitem);
            }
        }
    }
}

void UDPcomm::netProcessDataReq(struct espAvailDataReq* eadr) {
    uint8_t buffer[sizeof(struct espAvailDataReq) + 1];
    buffer[0] = PKT_AVAIL_DATA_INFO;
    memcpy(buffer + 1, eadr, sizeof(struct espAvailDataReq));
    udp.writeTo(buffer, sizeof(buffer), UDPIP, UDPPORT);
}

void UDPcomm::netProcessXferComplete(struct espXferComplete* xfc) {
    uint8_t buffer[sizeof(struct espXferComplete) + 1];
    buffer[0] = PKT_XFER_COMPLETE;
    memcpy(buffer + 1, xfc, sizeof(struct espXferComplete));
    udp.writeTo(buffer, sizeof(buffer), UDPIP, UDPPORT);
}

void UDPcomm::netProcessXferTimeout(struct espXferComplete* xfc) {
    uint8_t buffer[sizeof(struct espXferComplete) + 1];
    buffer[0] = PKT_XFER_TIMEOUT;
    memcpy(buffer + 1, xfc, sizeof(struct espXferComplete));
    udp.writeTo(buffer, sizeof(buffer), UDPIP, UDPPORT);
}

void UDPcomm::netSendDataAvail(struct pendingData* pending) {
    uint8_t buffer[sizeof(struct pendingData) + 1];
    buffer[0] = PKT_AVAIL_DATA_REQ;
    memcpy(buffer + 1, pending, sizeof(struct pendingData));
    udp.writeTo(buffer, sizeof(buffer), UDPIP, UDPPORT);
}

void UDPcomm::netTaginfo(struct TagInfo* taginfoitem) {
    uint8_t buffer[sizeof(struct TagInfo) + 1];
    buffer[0] = PKT_TAGINFO;
    memcpy(buffer + 1, taginfoitem, sizeof(struct TagInfo));
    udp.writeTo(buffer, sizeof(buffer), UDPIP, UDPPORT);
}

void prepareExternalDataAvail(struct pendingData *pending, IPAddress remoteIP) {
	uint8_t mac[6];
	WiFi.macAddress(mac);
	uint8_t macWithZeros[8] = {0};
	memcpy(&macWithZeros, mac, 6);

	char hexmac1[17];
	mac2hex(pending->targetMac, hexmac1);

	if (memcmp(pending->targetMac, macWithZeros, 8) == 0) {

			Serial.printf("test type %d\n", pending->availdatainfo.dataType);


			switch (pending->availdatainfo.dataType) {
			case 0x20:
			case 0x21: {
                char md5[17];
                mac2hex(reinterpret_cast<uint8_t*>(&pending->availdatainfo.dataVer), md5);
                char imageUrl[80];
                snprintf(imageUrl, sizeof(imageUrl), "http://%s/getdata?mac=%s&md5=%s", remoteIP.toString().c_str(), hexmac1, md5);
			    Serial.println(imageUrl);

				HTTPClient http;
				http.begin(imageUrl);
				int httpCode = http.GET();

				if (httpCode == 200) {

                    int downloadRemaining = http.getSize();
			        Serial.printf("Download Remaining %d\n", downloadRemaining);

                    uint16_t screenWidth = 480;
                    uint16_t screenHeight = 480;
					const size_t bufferSize = screenWidth * screenHeight * 2;
					uint8_t *buffer = (uint8_t *)malloc(bufferSize);

			        Serial.printf("Buffer Size %d\n", bufferSize);

					if (buffer) {
                        #define CHUNK_SIZE HTTP_TCP_BUFFER_SIZE/2
                        bool success = true;

                        uint8_t * cur_buffer = buffer;
                        
						WiFiClient *stream = http.getStreamPtr();
						
                        //size_t bytesRead = 0;
						//while (stream->available()) {
							//size_t bytesToRead = stream->available();
							//if (bytesToRead > bufferSize) bytesToRead = bufferSize;
							//bytesRead += stream->readBytes(buffer + bytesRead, bytesToRead);
						//}

                        //stream->setTimeout(30);

                        while (success &&
                            http.connected() &&
                            (downloadRemaining > 0 || downloadRemaining == -1)
                            ) 
                        {
                            auto size = stream->available();
                            if (size > 0) {
                                auto c = stream->readBytes(cur_buffer, ((size > CHUNK_SIZE) ? CHUNK_SIZE : size));
                                cur_buffer = cur_buffer + c;
                                auto totalSize = cur_buffer - buffer;
                                if (totalSize + 2 * CHUNK_SIZE > bufferSize)
                                {
                                    cur_buffer = buffer;
                                }

                                success &= c > 0;

                                if (downloadRemaining > 0) {
                                    downloadRemaining -= c;
                                }
                            }
                            //yield();
                            delay(1);
                        }

						drawImage(buffer, pending->availdatainfo.dataType);

						struct espXferComplete xfc = {0};
						memcpy(xfc.src, pending->targetMac, 8);
						udpsync.netProcessXferComplete(&xfc);
					}
					free(buffer);
				}
				http.end();

				break;
			}
			default: {
				Serial.printf("got unknown data type %d\n", pending->availdatainfo.dataType);
			}
			}
	}
}

void mac2hex(uint8_t *mac, char *hexBuffer) {
	sprintf(hexBuffer, "%02X%02X%02X%02X%02X%02X%02X%02X",
			mac[7], mac[6], mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
}

