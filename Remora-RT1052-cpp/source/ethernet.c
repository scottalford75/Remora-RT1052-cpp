#include "lwip/opt.h"

#include "lwip/timeouts.h"
#include "lwip/init.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "netif/ethernet.h"
#include "enet_ethernetif.h"

#include "board.h"
#include "ethernet.h"

#include "fsl_silicon_id.h"
#include "fsl_phy.h"
#include "fsl_phylan8720a.h"
#include "fsl_enet_mdio.h"
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"

#include "configuration.h"
#include "remora.h"

extern rxData_t rxBuffer;
extern volatile rxData_t rxData;
extern volatile txData_t txData;
extern volatile bool cmdReceived;
extern volatile bool mpgReceived;

static mdio_handle_t mdioHandle = {.ops = &enet_ops};
static phy_handle_t phyHandle   = {.phyAddr = BOARD_ENET0_PHY_ADDRESS, .mdioHandle = &mdioHandle, .ops = &phylan8720a_ops};
struct netif netif;

void udp_data_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
void udp_mpg_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

void initEthernet(void)
{
    ip4_addr_t netif_ipaddr, netif_netmask, netif_gw;
    ethernetif_config_t enet_config = {
        .phyHandle = &phyHandle
    };

    const clock_enet_pll_config_t config = {.enableClkOutput = true, .enableClkOutput25M = false, .loopDivider = 1};
    CLOCK_InitEnetPll(&config);

    IOMUXC_EnableMode(IOMUXC_GPR, kIOMUXC_GPR_ENET1TxClkOutputDir, true);

    mdioHandle.resource.csrClock_Hz = CLOCK_GetFreq(kCLOCK_IpgClk);

    (void)SILICONID_ConvertToMacAddr(&enet_config.macAddress);

    IP4_ADDR(&netif_ipaddr, configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3);
    IP4_ADDR(&netif_netmask, configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3);
    IP4_ADDR(&netif_gw, configGW_ADDR0, configGW_ADDR1, configGW_ADDR2, configGW_ADDR3);

    lwip_init();

    netif_add(&netif, &netif_ipaddr, &netif_netmask, &netif_gw, &enet_config, ethernetif0_init, ethernet_input);
    netif_set_default(&netif);
    netif_set_up(&netif);

}

void EthernetTasks(void)
{
    ethernetif_input(&netif);
    sys_check_timeouts();
}

void udpServer_init(void)
{
   struct udp_pcb *upcb, *upcb2;
   err_t err;

   ip_addr_t myIPADDR;
   IP_ADDR4(&myIPADDR, 10, 10, 10, 10);

   // UDP control block for data
   upcb = udp_new();
   err = udp_bind(upcb, &myIPADDR, 27181);  // 27181 is the server UDP port


   /* 3. Set a receive callback for the upcb */
   if(err == ERR_OK)
   {
	   udp_recv(upcb, udp_data_callback, NULL);
   }
   else
   {
	   udp_remove(upcb);
   }


   // UDP control block for MPG
   upcb2 = udp_new();
   err = udp_bind(upcb2, &myIPADDR, 27182);  // 27182 is the server UDP port for NVMPG

   if(err == ERR_OK)
   {
	   udp_recv(upcb2, udp_mpg_callback, NULL);
   }
   else
   {
	   udp_remove(upcb2);
   }
}

void udp_data_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	int txlen = 0;
	struct pbuf *txBuf;

	// copy the UDP payload into the rxData structure
	memcpy(&rxBuffer.rxBuffer, p->payload, p->len);

	if (rxBuffer.header == PRU_READ)
	{
		txData.header = PRU_DATA;
		txlen = BUFFER_SIZE;
		cmdReceived = true;
	}
	else if (rxBuffer.header == PRU_WRITE)
	{
		txData.header = PRU_ACKNOWLEDGE;
		txlen = sizeof(txData.header);
		cmdReceived = true;

		// ensure an atomic access to the rxBuffer
		// disable thread interrupts
		__disable_irq();

		// then move the data
		for (int i = 0; i < BUFFER_SIZE; i++)
		{
			rxData.rxBuffer[i] = rxBuffer.rxBuffer[i];
		}

		// re-enable thread interrupts
		__enable_irq();
	}


	// allocate pbuf from RAM
	txBuf = pbuf_alloc(PBUF_TRANSPORT, txlen, PBUF_RAM);

	// copy the data into the buffer
	pbuf_take(txBuf, (char*)&txData.txBuffer, txlen);

	// Connect to the remote client
	udp_connect(upcb, addr, port);

	// Send a Reply to the Client
	udp_send(upcb, txBuf);

	// free the UDP connection, so we can accept new clients
	udp_disconnect(upcb);

	// Free the p_tx buffer
	pbuf_free(txBuf);

	// Free the p buffer
	pbuf_free(p);
}

void udp_mpg_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	// copy the UDP payload into the nvmpg structure
	memcpy(&mpgData.payload, p->payload, p->len);

	// Free the p buffer
	pbuf_free(p);

	if (mpgData.header == PRU_NVMPG)
	{
		mpgReceived = true;
	}
}
