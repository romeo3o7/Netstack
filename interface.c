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
	_Bool up;
	_Bool running;
	char mac[1025];
	char ipv4[1025];
	char ipv6[1025];
	struct Interface *next;
} Interface;

Interface *createNode(const char *s,_Bool up,_Bool running){
	Interface *node = malloc(sizeof(Interface));
	if (node == NULL) return NULL;
	strcpy(node->name,s);
	node->up = up;
	node->running = running;
	node->next = NULL;
	return node;
}
int probeInterfaces(Interface **head) {
	_Bool status = 0;
	char *errOrigin;
	struct if_nameindex *if_ni, *i;
	if_ni = if_nameindex();
	if (!if_ni) return 1;
	*head = NULL;
	Interface *tail = NULL;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		status = 1;
		errOrigin = "socket";
		goto cleanup;
	}

	struct ifreq ifr;
	for (i = if_ni; !(i->if_index == 0 && i->if_name == NULL); i++) {
		memset(&ifr, 0, sizeof ifr);
		strncpy(ifr.ifr_name, i->if_name, sizeof ifr.ifr_name);
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) == -1) {
			errOrigin = "ioctl";
			status = 1;
			goto cleanup;
		}
		_Bool up = !!(ifr.ifr_flags & IFF_UP);
		_Bool running = !!(ifr.ifr_flags & IFF_RUNNING);
		Interface *node = createNode(i->if_name,up,running);
		if (!node) {
			errOrigin = "creating a node";
			status = 1;
			continue;
		}
		if (*head == NULL) {
			*head = node;
		} else {
			tail->next = node;
		}
		tail = node;
	}
	cleanup:
	if (status) fprintf(stderr,"%s : %s" , errOrigin, strerror(errno));
	if (sock >= 0) close(sock);
	if_freenameindex(if_ni);
	return status;
}

int ifaddr(Interface *head) {
 	 struct ifaddrs *ifaddr;
 	 if (getifaddrs(&ifaddr) == -1) return 1;
	 for (Interface *temp = head; temp != NULL; temp = temp->next) {
		 char *target = temp->name;
 		 int family;
 		 for (struct ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
 		 	  if (!ifa->ifa_addr) {
					continue;
 		 	  }
 		 	  family = ifa->ifa_addr->sa_family;
 		 	  if (strcmp(ifa->ifa_name, target) == 0) {
 		 	 	 if (family == AF_PACKET) {
			 	   	struct sockaddr_ll *sl = (struct sockaddr_ll *)ifa->ifa_addr;
			 	   	if (sl->sll_halen == 6) {
			 	   	  snprintf(temp->mac, sizeof(temp->mac), "%02x:%02x:%02x:%02x:%02x:%02x",
			 	   			   sl->sll_addr[0], sl->sll_addr[1], sl->sll_addr[2],
			 	   			   sl->sll_addr[3], sl->sll_addr[4], sl->sll_addr[5]);
			 	   	}
 		 	 	 }
 		 	 	 if (family == AF_INET) {
			 	   	struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
			 	   	if (inet_ntop(AF_INET, &sa->sin_addr, temp->ipv4, sizeof(temp->ipv4)) == NULL) return 1;
 		 	 	 }
 		 	 	 if (family == AF_INET6) {
			 	   	struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)ifa->ifa_addr;
			 	   	if (inet_ntop(AF_INET6, &sa6->sin6_addr, temp->ipv6, sizeof(temp->ipv6)) == NULL) return 1;
 		 	 	 }
 		 	 }
 		 }
	}
 	 freeifaddrs(ifaddr);
 	 return 0;
}

int interfacesPrint(Interface *head) {
	if (head == NULL) return 1;
	for (Interface *temp = head; temp != NULL; temp = temp->next) {
		fprintf(stdout,"interface: %s\n" , temp->name);
		if (!temp->up) {
			fprintf(stderr, "Interface is Down.\nTry: ip link set {interface name} up\nls /sys/class/net: to see available interfaces\n");
			return 1;
		}
		if (!temp->running) {
			fprintf(stderr, "connectivity or driver issue\n");
		    return 1;
		}
		printf("MAC Address = %s\n" , temp->mac);
		printf("IPV4 Address = %s\n" , temp->ipv4);
		printf("IPV6 Address = %s\n" , temp->ipv6);
		printf("interface is up and running\n");
		printf("----------------------\n");
	}
	return 0;
}

int main() {
	int status = 0;
	Interface *head = NULL;
	if (probeInterfaces(&head)) {
		status = 1;
		goto cleanup;
	}
	if (ifaddr(head)) {
		status = 1;
		goto cleanup;
	}
	if (interfacesPrint(head)) {
		status = 1;
		goto cleanup;
	} return 1;

	cleanup:
	if (head) free(head);
	return status;
}
