/*
 * Copyright: (c) 2012 Czech Technical University in Prague
 *
 * Authors:
 * 	Radek Matějka <radek.matejka@gmail.com>
 * 	Michal Sojka  <sojkam1@fel.cvut.cz>
 *
 * Funded by: Volkswagen Group Research
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/file.h>
#include <net/sock.h>
#include <linux/can.h>
#include <linux/miscdevice.h>
#include <linux/can/canethgw.h>

MODULE_LICENSE("GPL");

enum msg_types {
	CAN_FRAME,
};

struct cegw_job
{
	struct kref refcount;
	struct socket* can_sock;
	struct socket* udp_sock;
	u32  udp_dstcnt;
	u32  udp_addrlen;
	u8   udp_dst[0];
};

static int cegw_udp2can(void *data);
static int cegw_udp_send(struct socket *udp_sock, struct can_frame *cf,
		struct sockaddr* addr, int addrlen);
static int cegw_can2udp(void *data);
static int cegw_can_send(struct socket *can_sock, struct can_frame *cf);
static int cegw_thread_start(void *data);
static int cegw_thread_stop(struct cegw_job *job);
static void cegw_job_release(struct kref *ref);

static int cegw_udp_send(struct socket *udp_sock, struct can_frame *cf, struct sockaddr *addr,
		int addrlen)
{
	struct msghdr mh;
	struct kvec vec[2];
	int err;
	__u16 type = CAN_FRAME;

	mh.msg_name = addr;
	mh.msg_namelen = addrlen;
	mh.msg_control = NULL;
	mh.msg_controllen = 0;
	mh.msg_flags = 0;

	type = cpu_to_be16(type);
	vec[0].iov_base = &type;
	vec[0].iov_len = sizeof(type);
	vec[1].iov_base = cf;
	vec[1].iov_len = sizeof(*cf);

	err = kernel_sendmsg(udp_sock, &mh, vec, 2, sizeof(type)+sizeof(*cf));

	return err;
}

static int cegw_can_send(struct socket* can_sock, struct can_frame* cf)
{
	struct msghdr mh;
	struct kvec vec;
	int err;

	mh.msg_name = NULL;
	mh.msg_namelen = 0;
	mh.msg_control = NULL;
	mh.msg_controllen = 0;
	mh.msg_flags = 0;

	vec.iov_base = cf;
	vec.iov_len = sizeof(*cf);

	err = kernel_sendmsg(can_sock, &mh, &vec, 1, sizeof(*cf));

	return err;
}

/**
 * cegw_udp2can - performs udp->can routing
 *
 * This function is run as a thread.
 */
static int cegw_udp2can(void *data)
{
	struct can_frame cf;
	struct kvec vec[2];
	struct msghdr mh;
	struct cegw_job *job = (struct cegw_job *)data;
	struct socket *udp_sock = NULL, *can_sock = NULL;
	int ret = 0;
	__u16 type;

	memset(&mh, 0, sizeof(mh));
	udp_sock = job->udp_sock;
	can_sock = job->can_sock;

	while (1) {
		vec[0].iov_base = &type;
		vec[0].iov_len = sizeof(type);
		vec[1].iov_base = &cf;
		vec[1].iov_len = sizeof(cf);
		ret = kernel_recvmsg(udp_sock, &mh, vec, 2,
				     sizeof(type) + sizeof(cf), 0);
		if (ret == 0)
			break;
		if (ret != sizeof(type) + sizeof(cf)) {
			pr_notice_ratelimited("canethgw: UDP length mismatch\n");
			continue;
		}

		type = be16_to_cpu(type);
		switch (type) {
		case CAN_FRAME:
			cf.can_id = be32_to_cpu(cf.can_id);
			cegw_can_send(can_sock, &cf);
			break;
		default:
			pr_notice_ratelimited("canethgw: Unknown message type\n");
		}
	}

	cegw_thread_stop(job);
	kref_put(&job->refcount, cegw_job_release);
	do_exit(ret);
}

/**
 * cegw_can2udp - performs can->udp routing
 *
 * This function is run as a thread.
 */
static int cegw_can2udp(void* data)
{
	struct msghdr mh;
	struct kvec vec;
	struct sockaddr *udst;
	struct can_frame cf;
	struct cegw_job* job = (struct cegw_job*)data;
	struct socket* udp_sock = job->udp_sock;
	struct socket* can_sock = job->can_sock;
	int i;
	int ret;

	memset(&mh, 0, sizeof(mh));

	while (1) {
		vec.iov_base = &cf;
		vec.iov_len = sizeof(cf);

		ret = kernel_recvmsg(can_sock, &mh, &vec, 1,
				     sizeof(cf), 0);
		if (ret != sizeof(cf))
			break;

		cf.can_id = cpu_to_be32(cf.can_id);
		for (i=0; i<job->udp_dstcnt; i++) {
			udst = (struct sockaddr *)(job->udp_dst + i*job->udp_addrlen);
			cegw_udp_send(udp_sock, &cf, udst, job->udp_addrlen);
		}
	}

	cegw_thread_stop(job);
	kref_put(&job->refcount, cegw_job_release);
	do_exit(ret);
}

static void cegw_job_release(struct kref *ref)
{
	struct cegw_job *job = container_of(ref, struct cegw_job, refcount);

	fput(job->can_sock->file);
	fput(job->udp_sock->file);
	kfree(job);
}

/**
 * cegw_thread_start - start working threads
 * @data: (struct cegw_job *) with sockets and udp addresses filled in
 *
 * Two threads are started. One is serving udp->can routing and the other
 * can->udp.
 */
static int cegw_thread_start(void *data)
{
	struct task_struct *task = NULL;
	struct cegw_job *job = (struct cegw_job *)data;

	kref_get(&job->refcount);
	task = kthread_run(cegw_udp2can, data, "canethgw_udp2can");
	if (IS_ERR(task)) {
		kref_put(&job->refcount, cegw_job_release);
		return -ENOMEM;
	}

	kref_get(&job->refcount);
	task = kthread_run(cegw_can2udp, data, "canethgw_can2udp");
	if (IS_ERR(task)) {
		cegw_thread_stop(job);
		kref_put(&job->refcount, cegw_job_release);
		return -ENOMEM;
	}

	return 0;
}

/**
 * cegw_thread_stop - stops threads
 */
static int cegw_thread_stop(struct cegw_job *job)
{
	int how = SHUT_RDWR;
	struct sock *sk = NULL;
	struct socket *udp_sock = job->udp_sock;
	struct socket *can_sock = job->can_sock;

	if (udp_sock)
		kernel_sock_shutdown(udp_sock, SHUT_RDWR);

	/* PF_CAN sockets do not implement shutdown - do it manualy */
	sk = can_sock->sk;
	how++;
	lock_sock(sk);
	sk->sk_shutdown |= how;
	sk->sk_state_change(sk);
	release_sock(sk);

	return 0;
}

static int cegw_open(struct inode *inode, struct file *file)
{
	file->private_data = NULL;

	if (try_module_get(THIS_MODULE) == false)
		return -EAGAIN;

	return 0;
}

static int cegw_release(struct inode *inode, struct file *file)
{
	struct cegw_job *job = (struct cegw_job *)file->private_data;

	if (job) {
		cegw_thread_stop(job);
	}
	kref_put(&job->refcount, cegw_job_release);

	module_put(THIS_MODULE);
	return 0;
}

/**
 * cegw_ioctl_start - processes ioctl CEGW_IOCTL_START call
 *
 * The function takes over cegw_ioctl structure from userspace and
 * prepares cegw_job structure. The cegw_job is stored in
 * file->private_data and used by kernel threads to serve gateway
 * functionality.
 */
static long cegw_ioctl_start(struct file *file, unsigned long arg)
{
	int i;
	int chckfam;
	int err = 0;
	__u32 dstcnt = 0;
	__u32 addrlen = 0;
	struct sockaddr *sa;
	struct cegw_ioctl gwctl;
	struct cegw_job *job = NULL;

	err = copy_from_user(&gwctl, (void __user *)arg, sizeof(gwctl));
	if (err != 0)
		return -EFAULT;

	dstcnt = gwctl.udp_dstcnt;
	addrlen = gwctl.udp_addrlen;

	if (addrlen != sizeof(struct sockaddr_in) && addrlen != sizeof(struct sockaddr_in6))
		return -EAFNOSUPPORT;

	/* ToDo: consider dstcnt maximum */
	job = kmalloc(GFP_KERNEL, sizeof(*job) + dstcnt*addrlen);
	if (job == NULL)
		return -ENOMEM;

	err = copy_from_user(&job->udp_dst, (void __user *)(arg + sizeof(struct cegw_ioctl)), dstcnt*addrlen);
	if (err != 0) {
		err = -EFAULT;
		goto err_free;
	}

	/* */
	if (dstcnt > 0) {
		sa = (struct sockaddr *)job->udp_dst;
		chckfam = sa->sa_family;
	}

	for (i=1; i<dstcnt; i++) {
		sa = (struct sockaddr *)(job->udp_dst + i*addrlen);
		if (sa->sa_family != chckfam) {
			err = -EAFNOSUPPORT;
			goto err_free;
		}
	}

	job->udp_sock = sockfd_lookup(gwctl.udp_sock, &err);
	if (job->udp_sock == NULL)
		goto err_free;

	job->can_sock = sockfd_lookup(gwctl.can_sock, &err);
	if (job->can_sock == NULL)
		goto err_put_udp;

	if (job->can_sock->ops->family != AF_CAN ||
	    job->can_sock->type != SOCK_RAW) {
		err = -EBADF;
		goto err_put_all;
	}

	kref_init(&job->refcount);

	job->udp_dstcnt = dstcnt;
	job->udp_addrlen = addrlen;

	err = cegw_thread_start(job);
	if (err != 0)
		return err; /* cegw_thread_start performs cleaup for us.  */

	file->private_data = job;
	return 0;

err_put_all:
	fput(job->can_sock->file);
err_put_udp:
	fput(job->udp_sock->file);
err_free:
	kfree(job);
	return err;
}

static long cegw_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err;

	switch (cmd) {
		case CEGW_IOCTL_START:
			err = cegw_ioctl_start(file, arg);
			break;
		default:
			err = -EOPNOTSUPP;
			break;
	}

	return err;
}

static const struct file_operations cegw_fops = {
	.owner = THIS_MODULE,
	.open = cegw_open,
	.release = cegw_release,
	.unlocked_ioctl = cegw_ioctl,
	.compat_ioctl = cegw_ioctl,
};

static struct miscdevice cegw_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "canethgw",
	.fops = &cegw_fops
};

static int __init cegw_init(void)
{
	pr_info("can: can-eth gateway\n");
	return misc_register(&cegw_device);
}

static void __exit cegw_exit(void)
{
	misc_deregister(&cegw_device);

	return;
}

module_init(cegw_init);
module_exit(cegw_exit);
