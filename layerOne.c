#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

typedef struct Interface {
	char name[IF_NAMESIZE];
	_Bool up;
	_Bool running;
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
//		if (strcmp(i->if_name, "lo") == 0) continue;
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

int intDet(Interface *head) {
	if (head == NULL) return 1;
	for (Interface *temp = head; temp != NULL; temp = temp->next) {
		fprintf(stdout,"interface: %s\n" , temp->name);
		if (!temp->up) {
			fprintf(stderr, "Interface is Down, Try: ip link set {interface name} up\nls /sys/class/net: to see available interfaces\n");
			return 1;
		}
		if (!temp->running) {
			fprintf(stderr, "connectivity or driver issue\n");
		    return 1;
		}
		printf("interface is up and running\n");
	}
	return 0;
}

int main() {
	Interface *head = NULL;
	if (probeInterfaces(&head)) {
		fprintf(stderr, "%s" , strerror(errno));
		return 1;
	}

	if (intDet(head)) return 1;
return 0;
}
