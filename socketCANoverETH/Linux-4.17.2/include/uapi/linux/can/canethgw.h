#ifndef CANETHGW_H
#define CANETHGW_H

#include <linux/types.h>

struct cegw_ioctl
{
	__u32 can_sock;
	__u32 udp_sock;
	__u32 udp_dstcnt;
	__u32 udp_addrlen;
	__u8 udp_dst[0];
};

#define CEGW_IOCTL_BASE 'c'
#define CEGW_IOCTL_START _IOW(CEGW_IOCTL_BASE, 0, struct cegw_ioctl)

#endif /* CANETHGW_H */
