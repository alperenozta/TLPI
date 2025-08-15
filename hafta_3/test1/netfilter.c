#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/inet.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define PROC_NAME "userlist"

static char *msg;
static char *allip;
static char *iniplist[50];
static char *outiplist[50];

static int ipindex = 0;
static int in_index = 0, out_index = 0;

static struct nf_hook_ops nfho_in, nfho_out;

void printinfo(void) {
    int i;
    printk(KERN_INFO "\n");

    if (in_index == 0)
        printk(KERN_INFO "Not blocking any incoming network packet\n");
    else {
        printk(KERN_INFO "Blocking incoming packets from:\n");
        for (i = 0; i < in_index; i++)
            printk(KERN_INFO "%s\n", iniplist[i]);
    }

    if (out_index == 0)
        printk(KERN_INFO "Not blocking any outgoing network packet\n");
    else {
        printk(KERN_INFO "Blocking outgoing packets to:\n");
        for (i = 0; i < out_index; i++)
            printk(KERN_INFO "%s\n", outiplist[i]);
    }
}

static ssize_t read_proc(struct file *filp, char __user *buf, size_t count, loff_t *offp) {
    size_t len = strlen(msg);
    if (*offp >= len)
        return 0;

    if (count > len - *offp)
        count = len - *offp;

    if (copy_to_user(buf, msg + *offp, count))
        return -EFAULT;

    *offp += count;
    return count;
}

static ssize_t write_proc(struct file *filp, const char __user *buf, size_t count, loff_t *offp) {
    if (count > 100)
        return -EFAULT;

    if (copy_from_user(msg, buf, count))
        return -EFAULT;

    if (copy_from_user(&allip[ipindex], buf, count))
        return -EFAULT;

    msg[count] = '\0';

    if (msg[0] == 'p') {
        printinfo();
    } else if (msg[0] == 'r') {
        in_index = out_index = ipindex = 0;
        printk(KERN_INFO "All IP addresses cleared\n");
    } else {
        if (msg[0] == '0' || msg[0] == '2') {
            iniplist[in_index] = kmalloc(count, GFP_KERNEL);
            if (!iniplist[in_index])
                return -ENOMEM;
            allip[ipindex + count - 1] = 0;
            strcpy(iniplist[in_index], &allip[ipindex + 2]);
            printk(KERN_INFO "Blocking incoming from: %s\n", iniplist[in_index]);
            in_index++;
        }
        if (msg[0] == '1' || msg[0] == '2') {
            outiplist[out_index] = kmalloc(count, GFP_KERNEL);
            if (!outiplist[out_index])
                return -ENOMEM;
            allip[ipindex + count - 1] = 0;
            strcpy(outiplist[out_index], &allip[ipindex + 2]);
            printk(KERN_INFO "Blocking outgoing to: %s\n", outiplist[out_index]);
            out_index++;
        }
    }

    ipindex += count;
    return count;
}

static struct proc_ops proc_fops = {
    .proc_read = read_proc,
    .proc_write = write_proc,
};

unsigned int hook_func_in(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct iphdr *ip_header;
    char source[16];
    int i;

    if (!skb)
        return NF_ACCEPT;

    ip_header = ip_hdr(skb);
    snprintf(source, 16, "%pI4", &ip_header->saddr);

    for (i = 0; i < in_index; i++) {
        if (strcmp(source, iniplist[i]) == 0) {
            printk(KERN_INFO "Dropping incoming packet from %s\n", source);
            return NF_DROP;
        }
    }
    return NF_ACCEPT;
}

unsigned int hook_func_out(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct iphdr *ip_header;
    char destination[16];
    int i;

    if (!skb)
        return NF_ACCEPT;

    ip_header = ip_hdr(skb);
    snprintf(destination, 16, "%pI4", &ip_header->daddr);

    for (i = 0; i < out_index; i++) {
        if (strcmp(destination, outiplist[i]) == 0) {
            printk(KERN_INFO "Dropping outgoing packet to %s\n", destination);
            return NF_DROP;
        }
    }
    return NF_ACCEPT;
}

static int __init nf_module_init(void) {
    if (!proc_create(PROC_NAME, 0666, NULL, &proc_fops)) {
        printk(KERN_ERR "Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    msg = kmalloc(100, GFP_KERNEL);
    if (!msg)
        return -ENOMEM;

    allip = kmalloc(1000, GFP_KERNEL);
    if (!allip)
        return -ENOMEM;

    nfho_in.hook = hook_func_in;
    nfho_in.hooknum = NF_INET_LOCAL_IN;
    nfho_in.pf = PF_INET;
    nfho_in.priority = NF_IP_PRI_FIRST;

    nfho_out.hook = hook_func_out;
    nfho_out.hooknum = NF_INET_LOCAL_OUT;
    nfho_out.pf = PF_INET;
    nfho_out.priority = NF_IP_PRI_FIRST;

    nf_register_net_hook(&init_net, &nfho_in);
    nf_register_net_hook(&init_net, &nfho_out);

    printk(KERN_INFO "Netfilter module loaded.\n");
    return 0;
}

static void __exit nf_module_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    nf_unregister_net_hook(&init_net, &nfho_in);
    nf_unregister_net_hook(&init_net, &nfho_out);

    kfree(msg);
    kfree(allip);

    while (in_index--)
        kfree(iniplist[in_index]);
    while (out_index--)
        kfree(outiplist[out_index]);

    printk(KERN_INFO "Netfilter module unloaded.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ziqi Yang");
module_init(nf_module_init);
module_exit(nf_module_exit);
