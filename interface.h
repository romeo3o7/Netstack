#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ifaddrs.h>

typedef struct Interface {
	char name[IF_NAMESIZE];
	int ifindex;
	char gateway[INET_ADDRSTRLEN];
	_Bool up;
	_Bool running;
	char mac[18];
	char ipv4[10][INET_ADDRSTRLEN];
	size_t ipv4Count;
	char ipv6[10][INET6_ADDRSTRLEN];
	size_t ipv6Count;
	struct Interface *next;
} Interface;

int callinterfaces(void);
int get_default_gateway(Interface *head);
