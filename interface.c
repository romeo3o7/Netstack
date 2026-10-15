#include "interface.h"

static Interface *createNode(const char *s,int ifindex,_Bool up,_Bool running){
	Interface *node = (Interface *)calloc(1,sizeof(Interface));
	if (node == NULL) return NULL;
	strcpy(node->name,s);
	node->ifindex = ifindex;
	node->up = up;
	node->running = running;
	node->next = NULL;
	return node;
}
static int probeInterfaces(Interface **head) {
	*head = NULL;
	_Bool status = 0;
	char *errOrigin = "no Error";
	struct if_nameindex *if_ni, *i;
	if_ni = if_nameindex();
	if (!if_ni) return 1;
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
		Interface *node = createNode(i->if_name,i->if_index,up,running);
		if (!node) {
			errOrigin = "creating a node";
			status = 1;
			goto cleanup;
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

static int ifaddr(Interface *head) {
 	 struct ifaddrs *ifaddr;
 	 if (getifaddrs(&ifaddr) == -1) return 1;
	 for (Interface *temp = head; temp != NULL; temp = temp->next) {
		 char *target = temp->name;
		 temp->ipv4Count = 0;
		 temp->ipv6Count = 0;
 		 int family;
 		 for (struct ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
 		 	  if (!ifa->ifa_addr) {
					continue;
 		 	  }
 		 	  family = ifa->ifa_addr->sa_family;
 		 	  if (strcmp(ifa->ifa_name, target) == 0) {
 		 	 	 if (family == AF_PACKET) {
			 	   	struct sockaddr_ll sl;
					memcpy(&sl,ifa->ifa_addr,sizeof(sl));
			 	   	if (sl.sll_halen == 6) {
			 	   	  snprintf(temp->mac, sizeof(temp->mac), "%02x:%02x:%02x:%02x:%02x:%02x",
			 	   			   sl.sll_addr[0], sl.sll_addr[1], sl.sll_addr[2],
			 	   			   sl.sll_addr[3], sl.sll_addr[4], sl.sll_addr[5]);
			 	   	}
 		 	 	 }
 		 	 	 if (family == AF_INET) {
			 	   	struct sockaddr_in sa;
					memcpy(&sa,ifa->ifa_addr,sizeof(sa));
					if (temp->ipv4Count >= 10) continue;
			 	   	if (inet_ntop(AF_INET, &sa.sin_addr, temp->ipv4[temp->ipv4Count], sizeof(temp->ipv4[temp->ipv4Count])) == NULL) continue;
					temp->ipv4Count+=1;
 		 	 	 }
 		 	 	 if (family == AF_INET6) {
			 	   	struct sockaddr_in6 sa6;
					memcpy(&sa6,ifa->ifa_addr,sizeof(sa6));
					if (temp->ipv6Count >= 10) continue;
			 	   	if (inet_ntop(AF_INET6, &sa6.sin6_addr, temp->ipv6[temp->ipv6Count], sizeof(temp->ipv6[temp->ipv6Count])) == NULL) continue;
					temp->ipv6Count+=1;
 		 	 	 }
 		 	 }
 		 }
	}
 	 freeifaddrs(ifaddr);
 	 return 0;
}

static int interfacesPrint(Interface *head) {
	if (head == NULL) return 1;
	for (Interface *temp = head; temp != NULL; temp = temp->next) {
		printf("interface: %s\n" , temp->name);
		if (!temp->up) {
			printf("Interface is Down.\nTry: ip link set {interface name} up\nls /sys/class/net: to see available interfaces\n");
			continue;
		}
		if (!temp->running) {
			printf("Resource Not allocated for the interface (Connectivity) or driver issue\n");
		    continue;
		}
		printf("interface is up and running\n");
		char errorMsg[] = "Address Not Assigned";

		printf("MAC Address  -> %s\n" , (*temp->mac)  ? temp->mac  : errorMsg);
		for (size_t i = 0; i < temp->ipv4Count; i++) {
			printf("IPV4 Address -> %s\n" , (*temp->ipv4) ? temp->ipv4[i] : errorMsg);
		}
		for (size_t i = 0; i < temp->ipv6Count; i++) {
			printf("IPV6 Address -> %s\n" , (*temp->ipv6) ? temp->ipv6[i] : errorMsg);
		}
		printf("gateway -> %s\n" , (*temp->gateway) ? temp->gateway : errorMsg);
		printf("----------------------\n");
	}
	return 0;
}

static void freeInterface(Interface *head) {
	while (head) {
		Interface *next = head->next;
		free(head);
		head = next;
	}
}

int main(void) {
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
	get_default_gateway(head);
	if (interfacesPrint(head)) {
		status = 1;
		goto cleanup;
	}
	cleanup:
	if (head) freeInterface(head);
	return status;
}
