#ifndef _ETHERNET_H_
#define _ETHERNET_H_

/* IP address configuration. */
#define configIP_ADDR0 10
#define configIP_ADDR1 10
#define configIP_ADDR2 10
#define configIP_ADDR3 10

/* Netmask configuration. */
#define configNET_MASK0 255
#define configNET_MASK1 255
#define configNET_MASK2 255
#define configNET_MASK3 0

/* Gateway address configuration. */
#define configGW_ADDR0 10
#define configGW_ADDR1 10
#define configGW_ADDR2 10
#define configGW_ADDR3 1

#ifdef __cplusplus
extern "C" {
#endif

void initEthernet(void);
void EthernetTasks(void);
void udpServer_init(void);

#ifdef __cplusplus
}
#endif

#endif

