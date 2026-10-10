#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
typedef struct Interface {
	char name[IF_NAMESIZE];
	_Bool up;
	_Bool running;
} Interface;

int probeInterfaces(Interface *nic) {
	struct if_nameindex *if_ni, *i;

	if_ni = if_nameindex();
	if (if_ni == NULL) {
		return 1;
	}
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
	  if_freenameindex(if_ni);
	  return 1;
	}
	struct ifreq ifr;
	for (i = if_ni; i->if_index != 0; i++) {
		  if (strcmp(i->if_name, "lo") == 0) continue;
		  strcpy(nic->name ,i->if_name);
		  memset(&ifr, 0, sizeof ifr);
		  strncpy(ifr.ifr_name, i->if_name, sizeof ifr.ifr_name);
		  ioctl(sock, SIOCGIFFLAGS, &ifr);
		  nic->up = ifr.ifr_flags & IFF_UP;
		  nic->running = ifr.ifr_flags & IFF_RUNNING;
		  //nic->next = (struct Interface *)malloc(sizeof(Interface));
	}
	close(sock);
	if_freenameindex(if_ni);
	return 0;
}
int main() {
	Interface nic = {0};
	if (probeInterfaces(&nic)) {
		fprintf(stderr, "%s" , strerror(errno));
		return 1;
	}
	fprintf(stdout,"interface: %s\n" , nic.name);
	if (!nic.up) {
		fprintf(stderr, "Interface is Down, Try: ip link set {interface name} up\nls /sys/class/net: to see available interfaces\n");
		return 1;
	}
	if (!nic.running) {
		fprintf(stderr, "not connected to an AP or driver issue\n");
	    return 1;
	}
	printf("interface is up and running\n");

}
