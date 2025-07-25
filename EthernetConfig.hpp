#ifndef ETHERNET_CONFIG_H
#define ETHERNET_CONFIG_H

#define DEBUG_TELNET 0
#define DEBUG_FTP 0/*2*/
#define DEBUG_HTTP 0

#define DEBUG_TX_WINDOW 0
#define DEBUG_TCP 0
#define DEBUG_SOCKET 0
#define DEBUG_IPCONF 0
#define DEBUG_DHCP 0
#define DEBUG_ETHERNET 0
#define DEBUG_NTP 0

#define USE_UID_TO_MAC 1

#define ETH_STATS 10

#define MAX_NO_OF_TELNET_CLIENTS 2
#define MAX_NO_OF_HTTP_CLIENTS 12
#define MAX_NO_OF_FTP_CLIENTS 2

#define SMALL_PACKET_SIZE 16

#define TCP_ALIVE_PROBE_TIMEOUT 600
#define TCP_ALIVE_PROBES 3
#define SOCKET_QUEUE_LEN 16
#define SET_SOCKET_BUFFERIN_CCM 1

#define USE_CONFIGURABLE_IP 1
#define USE_DHCP 1

#define USE_HTTP 1
#define USE_TELNET 1
#define USE_FTP 1

#define DEFAULT_IP 0xC0A83733 /*192.168.55.51*/
#define DEFAULT_DHCP_SERVER  0xC0A83701 /*192.168.55.1*/
#define DEFAULT_DNS_SERVER 0xC0A83701 /*192.168.55.1*/
#define DEFAULT_SUBNET_MASK 0xFFFFFF00
#define DEFAULT_GATEWAY 0xC0A83701 /*192.168.55.1*/

#define OWN_MAC  { 0x00, 0x11, 0x22, 0x33, 0x44, 0x49 }
#define HOST_NAME "LON"

#define TELNET_WELCOME_STRING "LON welcome\n"

#define USE_MDNS 0
#define MDNS_MAC { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB }
#define MDNS_IP  0xE00000FB /*224.0.0.251*/
#define MDNS_PORT 5353

#define USE_NTP 1

#define ARP_ARRAY_SIZE 16

//#define NO_COPY_RXDESC 0




#endif