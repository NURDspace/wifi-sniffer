#include <ESP8266WiFi.h>       // added this

#define DATA_LENGTH           112

#define TYPE_MANAGEMENT       0x00
#define TYPE_CONTROL          0x01
#define TYPE_DATA             0x02
#define SUBTYPE_PROBE_REQUEST 0x04


struct RxControl {
	signed rssi:8; // signal intensity of packet
	unsigned rate:4;
	unsigned is_group:1;
	unsigned:1;
	 unsigned sig_mode:2; // 0:is 11n packet; 1:is not 11n packet;
	 unsigned legacy_length:12; // if not 11n packet, shows length of packet.
	 unsigned damatch0:1;
	 unsigned damatch1:1;
	 unsigned bssidmatch0:1;
	 unsigned bssidmatch1:1;
	 unsigned MCS:7; // if is 11n packet, shows the modulation and code used (range from 0 to 76)
	 unsigned CWB:1; // if is 11n packet, shows if is HT40 packet or not
	 unsigned HT_length:16;// if is 11n packet, shows length of packet.
	 unsigned Smoothing:1;
	 unsigned Not_Sounding:1;
	unsigned:1;
	 unsigned Aggregation:1;
	 unsigned STBC:2;
	 unsigned FEC_CODING:1; // if is 11n packet, shows if is LDPC packet or not.
	 unsigned SGI:1;
	 unsigned rxend_state:8;
	 unsigned ampdu_cnt:8;
	 unsigned channel:4; //which channel this packet in.
	unsigned:12;
};

struct SnifferPacket{
	struct RxControl rx_ctrl;
	uint8_t data[DATA_LENGTH];
	uint16_t cnt;
	uint16_t len;
};

static void showMetadata(SnifferPacket *snifferPacket) {

	uint16_t frameControl = (uint16_t(snifferPacket->data[1]) << 8) + snifferPacket->data[0];

	uint8_t version      = (frameControl & 0b0000000000000011) >> 0;
	uint8_t frameType    = (frameControl & 0b0000000000001100) >> 2;
	uint8_t frameSubType = (frameControl & 0b0000000011110000) >> 4;
	uint8_t toDS         = (frameControl & 0b0000000100000000) >> 8;
	uint8_t fromDS       = (frameControl & 0b0000001000000000) >> 9;

	// Only look for probe request packets

	if (frameType != TYPE_MANAGEMENT || frameSubType != SUBTYPE_PROBE_REQUEST)
		return;

	uint8_t SSID_length = snifferPacket->data[25];

	if (SSID_length == 0 || SSID_length > 32)
		return;

	for(int i=0; i<SSID_length; i++) {
		uint8_t c = snifferPacket->data[26 + i];

		if (c < 32 || c > 126)
			return;
	}

	Serial.printf("%d|%d|%02x:%02x:%02x:%02x:%02x:%02x|%02x:%02x:%02x:%02x:%02x:%02x|%02x:%02x:%02x:%02x:%02x:%02x|", snifferPacket->rx_ctrl.rssi, snifferPacket->rx_ctrl.channel,
		snifferPacket->data[4], //
		snifferPacket->data[5],
		snifferPacket->data[6],
		snifferPacket->data[7],
		snifferPacket->data[8],
		snifferPacket->data[9],
		snifferPacket->data[10],
		snifferPacket->data[11], //
		snifferPacket->data[12],
		snifferPacket->data[13],
		snifferPacket->data[14],
		snifferPacket->data[15],
		snifferPacket->data[16],
		snifferPacket->data[17],
		snifferPacket->data[18], //
		snifferPacket->data[19],
		snifferPacket->data[20],
		snifferPacket->data[21],
		snifferPacket->data[22],
		snifferPacket->data[23],
		snifferPacket->data[24]
);
		
	for(int i=0; i<SSID_length; i++) {
		if (snifferPacket->data[26 + i] < 32 || snifferPacket->data[26 + i] > 126)
			break;

		Serial.print(char(snifferPacket->data[26 + i]));
	}

	Serial.println(F(""));

#if 0
	StaticJsonDocument<200> doc;

	doc["rssi"] = snifferPacket->rx_ctrl.rssi, DEC;
	doc["txPower"] = txPower;
	doc["channel"] = wifi_get_channel();

	char addr[] = "00:00:00:00:00:00";
	getMAC(addr, snifferPacket->data, 10);
	doc["clientMac"] = addr;

	uint8_t SSID_length = snifferPacket->data[25];
	doc["ssid"] = printDataSpan(26, SSID_length, snifferPacket->data);
	doc["serial"] = serial;
	doc["networkId"] = networkId;

	serializeJson(doc, Serial);
	// Start a new line
	std::cout << std::endl;
#endif
}

/**
 * Callback for promiscuous mode
 */
static void ICACHE_FLASH_ATTR sniffer_callback(uint8_t *buffer, uint16_t length) {
	struct SnifferPacket *snifferPacket = (struct SnifferPacket*) buffer;

	showMetadata(snifferPacket);
}

static void getMAC(char *addr, uint8_t* data, uint16_t offset) {
	sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5]);
}

#define CHANNEL_HOP_INTERVAL_MS   250
static os_timer_t channelHop_timer;

/**
 * Callback for channel hoping
 */
void channelHop()
{
	// hoping channels 1-14
	uint8 new_channel = wifi_get_channel() + 1;
	// Serial.print(F("Hop to "));
	// Serial.println(new_channel);
	if (new_channel >= 14)
		new_channel = 1;
	wifi_set_channel(new_channel);
}

#define DISABLE 0
#define ENABLE  1

void setup() {
	Serial.begin(115200);

	delay(10);
	wifi_set_opmode(STATION_MODE);
	wifi_set_channel(1);
	wifi_promiscuous_enable(DISABLE);
	delay(10);
	wifi_set_promiscuous_rx_cb(sniffer_callback);
	delay(10);
	wifi_promiscuous_enable(ENABLE);

	// setup the channel hoping callback timer
	os_timer_disarm(&channelHop_timer);
	os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
	os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL_MS, 1);
}

void loop() {
	delay(10);  
}
