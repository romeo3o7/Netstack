#include "interface.h"

#define check(x) do { if(x) { status = 1; goto cleanup;} }while(0)

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

static Interface *findif(Interface *head, const char *name) {
	for (Interface *t = head; t != NULL; t = t->next) {
		if (strcmp(t->name, name) == 0) return t;
	}
	return NULL;
}

static int probeif(Interface **head) {
	*head = NULL;
	struct ifaddrs *ifaddr;
	if (getifaddrs(&ifaddr) == -1) {
		fprintf(stderr, "getifaddrs : %s\n", strerror(errno));
		return 1;
	}

	int status = 0;
	Interface *tail = NULL;

	for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) continue;
		if (strcmp(ifa->ifa_name, "lo") == 0) continue;

		Interface *node = findif(*head, ifa->ifa_name);
		if (node == NULL) {
			_Bool up      = !!(ifa->ifa_flags & IFF_UP);
			_Bool running = !!(ifa->ifa_flags & IFF_RUNNING);
			node = createNode(ifa->ifa_name, 0, up, running);
			if (node == NULL) {
				fprintf(stderr, "creating a node : %s\n", strerror(errno));
				status = 1;
				break;
			}
			if (*head == NULL) *head = node;
			else tail->next = node;
			tail = node;
		}

		sa_family_t family = ifa->ifa_addr->sa_family;
		if (family == AF_PACKET) {
			struct sockaddr_ll sl;
			memcpy(&sl, ifa->ifa_addr, sizeof(sl));
			node->ifindex = sl.sll_ifindex;
			if (sl.sll_halen == 6) {
				snprintf(node->mac, sizeof(node->mac), "%02x:%02x:%02x:%02x:%02x:%02x",
				         sl.sll_addr[0], sl.sll_addr[1], sl.sll_addr[2],
				         sl.sll_addr[3], sl.sll_addr[4], sl.sll_addr[5]);
			}
		} else if (family == AF_INET) {
			struct sockaddr_in sa;
			memcpy(&sa, ifa->ifa_addr, sizeof(sa));
			if (node->ipv4Count >= 10) continue;
			if (inet_ntop(AF_INET, &sa.sin_addr, node->ipv4[node->ipv4Count], sizeof(node->ipv4[0])) == NULL) continue;
			node->ipv4Count += 1;
		} else if (family == AF_INET6) {
			struct sockaddr_in6 sa6;
			memcpy(&sa6, ifa->ifa_addr, sizeof(sa6));
			if (node->ipv6Count >= 10) continue;
			if (inet_ntop(AF_INET6, &sa6.sin6_addr, node->ipv6[node->ipv6Count], sizeof(node->ipv6[0])) == NULL) continue;
			node->ipv6Count += 1;
		}
	}

	freeifaddrs(ifaddr);
	return status;
}

static int ifPrint(Interface *head) {
	if (head == NULL) return 1;
	for (Interface *temp = head; temp != NULL; temp = temp->next) {
		printf("\ninterface: %s\n" , temp->name);
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
		printf("gateway -> %s\n\n" , (*temp->gateway) ? temp->gateway : errorMsg);
	}
	return 0;
}

static void freeif(Interface *head) {
	while (head) {
		Interface *next = head->next;
		free(head);
		head = next;
	}
}

int main(void) {
	int status = 0;
	Interface *head = NULL;

	check(probeif(&head));
	get_default_gateway(head);
	check(ifPrint(head));

	cleanup:
	if (head) freeif(head);
	return status;
}
