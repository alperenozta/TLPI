// pktfilter.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>


// ===== ioctl arayüzü =====
#define PKTFILT_IOC_MAGIC   'p'
#define PKTFILT_IOC_SET_TCP_BLOCK   _IOW(PKTFILT_IOC_MAGIC, 1, int)   // arg: tcp_port
#define PKTFILT_IOC_SET_UDP_BLOCK   _IOW(PKTFILT_IOC_MAGIC, 2, int)   // arg: udp_port
#define PKTFILT_IOC_GET_STATS       _IOR(PKTFILT_IOC_MAGIC, 3, struct pkt_stats)
#define PKTFILT_IOC_CLEAR_STATS     _IO(PKTFILT_IOC_MAGIC, 4)

struct pkt_stats {
    u64 total;
    u64 tcp_pass;
    u64 udp_pass;
    u64 tcp_drop;
    u64 udp_drop;
};

// ===== global durum =====
static struct nf_hook_ops nfho;

static atomic64_t g_total      = ATOMIC64_INIT(0);
static atomic64_t g_tcp_pass   = ATOMIC64_INIT(0);
static atomic64_t g_udp_pass   = ATOMIC64_INIT(0);
static atomic64_t g_tcp_drop   = ATOMIC64_INIT(0);
static atomic64_t g_udp_drop   = ATOMIC64_INIT(0);

static int g_block_tcp_port = 0;  // 0 => blok yok
static int g_block_udp_port = 0;

static spinlock_t g_cfg_lock;     // blocklanan portlar için

// ===== char device =====
#define DEV_NAME   "pktfilter"
static dev_t devno;
static struct cdev cdev_obj;
static struct class *dev_class;

// ===== netfilter hook =====
static unsigned int hook_fn(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
    const struct iphdr *iph;
    atomic64_inc(&g_total);

    if (!skb)
        return NF_ACCEPT;

    iph = ip_hdr(skb);
    if (!iph)
        return NF_ACCEPT;

    if (iph->protocol == IPPROTO_TCP) {
        const struct tcphdr *tcph;
        int block_port;

        // TCP header
        tcph = (const struct tcphdr *)skb_transport_header(skb);
        if (!tcph)
            return NF_ACCEPT;

        spin_lock(&g_cfg_lock);
        block_port = g_block_tcp_port;
        spin_unlock(&g_cfg_lock);

        if (block_port > 0 && ntohs(tcph->dest) == block_port) {
            atomic64_inc(&g_tcp_drop);
            // pr_info("DROP TCP dst port %d\n", block_port);
            return NF_DROP;
        } else {
            atomic64_inc(&g_tcp_pass);
            return NF_ACCEPT;
        }
    } else if (iph->protocol == IPPROTO_UDP) {
        const struct udphdr *udph;
        int block_port;

        udph = (const struct udphdr *)skb_transport_header(skb);
        if (!udph)
            return NF_ACCEPT;

        spin_lock(&g_cfg_lock);
        block_port = g_block_udp_port;
        spin_unlock(&g_cfg_lock);

        if (block_port > 0 && ntohs(udph->dest) == block_port) {
            atomic64_inc(&g_udp_drop);
            // pr_info("DROP UDP dst port %d\n", block_port);
            return NF_DROP;
        } else {
            atomic64_inc(&g_udp_pass);
            return NF_ACCEPT;
        }
    }

    return NF_ACCEPT;
}

// ===== ioctl =====
static long pktfilter_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int port;
    struct pkt_stats ks;

    switch (cmd) {
    case PKTFILT_IOC_SET_TCP_BLOCK:
        if (copy_from_user(&port, (void __user *)arg, sizeof(port)))
            return -EFAULT;
        if (port < 0 || port > 65535) return -EINVAL;
        spin_lock(&g_cfg_lock);
        g_block_tcp_port = port;
        spin_unlock(&g_cfg_lock);
        pr_info("pktfilter: TCP block port set to %d\n", port);
        return 0;

    case PKTFILT_IOC_SET_UDP_BLOCK:
        if (copy_from_user(&port, (void __user *)arg, sizeof(port)))
            return -EFAULT;
        if (port < 0 || port > 65535) return -EINVAL;
        spin_lock(&g_cfg_lock);
        g_block_udp_port = port;
        spin_unlock(&g_cfg_lock);
        pr_info("pktfilter: UDP block port set to %d\n", port);
        return 0;

    case PKTFILT_IOC_GET_STATS:
        ks.total    = atomic64_read(&g_total);
        ks.tcp_pass = atomic64_read(&g_tcp_pass);
        ks.udp_pass = atomic64_read(&g_udp_pass);
        ks.tcp_drop = atomic64_read(&g_tcp_drop);
        ks.udp_drop = atomic64_read(&g_udp_drop);
        if (copy_to_user((void __user *)arg, &ks, sizeof(ks)))
            return -EFAULT;
        return 0;

    case PKTFILT_IOC_CLEAR_STATS:
        atomic64_set(&g_total, 0);
        atomic64_set(&g_tcp_pass, 0);
        atomic64_set(&g_udp_pass, 0);
        atomic64_set(&g_tcp_drop, 0);
        atomic64_set(&g_udp_drop, 0);
        return 0;

    default:
        return -ENOTTY;
    }
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = pktfilter_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = pktfilter_ioctl,
#endif
};

// ===== init/exit =====
static int __init pktfilter_init(void)
{
    int ret;

    spin_lock_init(&g_cfg_lock);

    // Netfilter hook
    nfho.hook     = hook_fn;
    nfho.hooknum  = NF_INET_PRE_ROUTING;  // gelen paketler
    nfho.pf       = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;

    ret = nf_register_net_hook(&init_net, &nfho);
    if (ret) {
        pr_err("pktfilter: nf_register_net_hook failed: %d\n", ret);
        return ret;
    }

    // Char device + /dev node
    ret = alloc_chrdev_region(&devno, 0, 1, DEV_NAME);
    if (ret) {
        pr_err("pktfilter: alloc_chrdev_region failed: %d\n", ret);
        nf_unregister_net_hook(&init_net, &nfho);
        return ret;
    }

    cdev_init(&cdev_obj, &fops);
    cdev_obj.owner = THIS_MODULE;

    ret = cdev_add(&cdev_obj, devno, 1);
    if (ret) {
        pr_err("pktfilter: cdev_add failed: %d\n", ret);
        unregister_chrdev_region(devno, 1);
        nf_unregister_net_hook(&init_net, &nfho);
        return ret;
    }

    dev_class = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(dev_class)) {
        pr_err("pktfilter: class_create failed\n");
        cdev_del(&cdev_obj);
        unregister_chrdev_region(devno, 1);
        nf_unregister_net_hook(&init_net, &nfho);
        return PTR_ERR(dev_class);
    }

    if (IS_ERR(device_create(dev_class, NULL, devno, NULL, DEV_NAME))) {
        pr_err("pktfilter: device_create failed\n");
        class_destroy(dev_class);
        cdev_del(&cdev_obj);
        unregister_chrdev_region(devno, 1);
        nf_unregister_net_hook(&init_net, &nfho);
        return -EINVAL;
    }

    pr_info("pktfilter: loaded (/dev/%s, major=%d minor=%d)\n",
            DEV_NAME, MAJOR(devno), MINOR(devno));
    return 0;
}

static void __exit pktfilter_exit(void)
{
    device_destroy(dev_class, devno);
    class_destroy(dev_class);
    cdev_del(&cdev_obj);
    unregister_chrdev_region(devno, 1);

    nf_unregister_net_hook(&init_net, &nfho);
    pr_info("pktfilter: unloaded\n");
}

module_init(pktfilter_init);
module_exit(pktfilter_exit);

