// Bu dosya SADECE modül meta bilgisini taşır.
// Böylece modpost 'license' bilgisini kesin görür.
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alperen Oztas");
MODULE_DESCRIPTION("Simple packet filter using Netfilter hooks");
MODULE_VERSION("0.1");
