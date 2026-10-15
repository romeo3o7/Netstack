#include "interface.h"

int get_default_gateway(Interface *head) {
    int sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (sock < 0) return -1;

    char buf[8192] = {0};
    struct nlmsghdr *nlh = (struct nlmsghdr *)buf;

    nlh->nlmsg_len   = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlh->nlmsg_type  = RTM_GETROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq   = 1;
    nlh->nlmsg_pid   = (uint32_t)getpid();

    if (send(sock, nlh, nlh->nlmsg_len, 0) < 0) {
        close(sock);
        return -1;
    }

    _Bool found = 0;
    _Bool done  = 0;

    while (!done) {
        ssize_t len = recv(sock, buf, sizeof(buf), 0);
        if (len < 0) {
            if (errno == EINTR) continue;
            close(sock);
            return -1;
        }
        if (len == 0) break;

        for (struct nlmsghdr *h = (struct nlmsghdr *)buf; NLMSG_OK(h, len); h = NLMSG_NEXT(h, len)) {

            if (h->nlmsg_type == NLMSG_DONE) { done = 1; break; }
            if (h->nlmsg_type == NLMSG_ERROR) { close(sock); return -1; }
            if (h->nlmsg_type != RTM_NEWROUTE) continue;

            struct rtmsg *rtm = (struct rtmsg *)NLMSG_DATA(h);
            if (rtm->rtm_family != AF_INET) continue;
            if (rtm->rtm_dst_len != 0) continue;
            if (rtm->rtm_table != RT_TABLE_MAIN) continue;

            char gw[INET_ADDRSTRLEN] = {0};
            int ifindex = 0;
            _Bool has_gw = 0;

            struct rtattr *rta = RTM_RTA(rtm);
            int rta_len = RTM_PAYLOAD(h);

            for (; RTA_OK(rta, rta_len); rta = RTA_NEXT(rta, rta_len)) {
                if (rta->rta_type == RTA_GATEWAY) {
                    inet_ntop(AF_INET, RTA_DATA(rta), gw, sizeof(gw));
                    has_gw = 1;
                } else if (rta->rta_type == RTA_OIF) {
                    memcpy(&ifindex, RTA_DATA(rta), sizeof(ifindex));
                }
            }

            if (!has_gw) continue;

            for (Interface *temp = head; temp != NULL; temp = temp->next) {
                if (temp->ifindex == ifindex) {
                    memcpy(temp->gateway,gw,sizeof(temp->gateway));
                    break;
                }
            }
        }
    }

    close(sock);
    return found;
}
