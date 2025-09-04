

---

# Linux Çekirdeğinde Netfilter ve ioctl ile Paket Filtreleme

Bu doküman, sağladığın `pktfilter.c` modülü örneği üzerinden **Netfilter** ve **ioctl** kavramlarını açıklamaktadır.

---

## 1. Genel Bakış

Bu modül basit bir **paket filtreleme** mekanizması uygular:

* **Netfilter hook** kullanarak gelen IPv4 paketlerini yakalar.
* Belirlenen TCP veya UDP hedef portuna giden paketleri engeller (**DROP**).
* Atomik değişkenlerle sayaçlar (toplam, geçen, düşen) tutar.
* Kullanıcı alanı ile iletişim için `/dev/pktfilter` karakter cihazı oluşturur.
* **ioctl** komutları ile engellenecek portları ayarlama, istatistikleri alma ve sayaçları sıfırlama işlemleri yapılır.

---

## 2. Linux Çekirdeğinde Netfilter

### 2.1 Netfilter Nedir?

Netfilter, Linux çekirdeği içinde paket filtreleme, NAT ve benzeri ağ işlemleri için kullanılan bir çerçevedir.
Çekirdek modüllerinin, ağ yığınındaki önceden tanımlanmış noktalara **hook** (kanca) eklemesine izin verir.

### 2.2 Netfilter Hook’ları

Hook’lar, paket işleme hattındaki müdahale noktalarıdır. IPv4 için yaygın hook noktaları:

| Hook Numarası          | Ne Zaman Çağrılır                           |
| ---------------------- | ------------------------------------------- |
| `NF_INET_PRE_ROUTING`  | Yönlendirme kararı verilmeden önce          |
| `NF_INET_LOCAL_IN`     | Yerel makineye gelen paketler               |
| `NF_INET_FORWARD`      | Yönlendirilecek paketler                    |
| `NF_INET_LOCAL_OUT`    | Yerel makineden çıkan paketler              |
| `NF_INET_POST_ROUTING` | Yönlendirme sonrası, gönderimden hemen önce |

Bu modülde:

```c
nfho.hooknum  = NF_INET_PRE_ROUTING;
nfho.pf       = PF_INET;
nfho.priority = NF_IP_PRI_FIRST;
```

* **Tüm gelen IPv4 paketleri** yönlendirme kararından önce yakalanır.
* `priority` değeri, birden fazla hook varsa çalıştırma sırasını belirler.

### 2.3 Hook Fonksiyonu

Hook fonksiyon imzası:

```c
unsigned int hook_fn(void *priv, struct sk_buff *skb,
                     const struct nf_hook_state *state);
```

* `skb`: **Socket buffer**, paketin çekirdek içindeki temsilidir.
* Dönüş değerleri:

  * `NF_ACCEPT`: Paketi kabul et.
  * `NF_DROP`: Paketi düşür.
  * Diğerleri (`NF_STOLEN`, `NF_QUEUE` vb.) ileri seviye kullanım için.

Modüldeki örnek mantık:

```c
if (iph->protocol == IPPROTO_TCP) {
    if (ntohs(tcph->dest) == blocked_port) {
        return NF_DROP;
    }
}
```

---

## 3. Linux Çekirdeğinde ioctl

### 3.1 ioctl Nedir?

`ioctl` (Input/Output Control), aygıtlara **kontrol komutları** göndermek için kullanılan bir sistem çağrısıdır.
Yapılandırılmış veya komut tabanlı veri göndermek için uygundur.

### 3.2 ioctl Komut Kodlama

Linux, **komut numaralarını** kodlamak için makrolar kullanır:

```c
#define PKTFILT_IOC_SET_TCP_BLOCK _IOW('p', 1, int)
#define PKTFILT_IOC_GET_STATS     _IOR('p', 3, struct pkt_stats)
```

* `_IOW`: Kullanıcı → çekirdek veri yazma
* `_IOR`: Çekirdek → kullanıcı veri okuma
* `_IO`: Veri yok
* İlk parametre `'p'`: Sürücüye özel sihirli sayı
* İkinci parametre: Komut numarası
* Üçüncü parametre: Veri tipi

### 3.3 Kullanıcı–Çekirdek Veri Aktarımı

* **Kullanıcıdan çekirdeğe**:

  ```c
  copy_from_user(&port, (void __user *)arg, sizeof(port))
  ```
* **Çekirdekten kullanıcıya**:

  ```c
  copy_to_user((void __user *)arg, &stats, sizeof(stats))
  ```

### 3.4 ioctl İşleyici

`struct file_operations` içinde tanımlanır:

```c
static const struct file_operations fops = {
    .unlocked_ioctl = pktfilter_ioctl,
};
```

Kullanıcı uygulaması şunu çağırdığında:

```c
ioctl(fd, PKTFILT_IOC_SET_TCP_BLOCK, &port);
```

Çekirdek şu fonksiyonu çalıştırır:

```c
static long pktfilter_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
```

Burada `cmd` bir `switch` ile kontrol edilir ve gerekli işlem yapılır.

---

## 4. Bu Modülde Netfilter ve ioctl Birlikte Nasıl Çalışıyor?

1. **Modül Başlatma**

   * `nf_register_net_hook()` ile Netfilter hook’u kaydedilir.
   * `/dev/pktfilter` cihaz düğümü oluşturulur.

2. **Kullanıcı Alanı Kontrolü**

   * Bir CLI aracı (`pktctl`) `/dev/pktfilter`’i açar.
   * `ioctl` ile:

     * TCP/UDP engel portu ayarlanır.
     * Sayaçlar okunur.
     * Sayaçlar sıfırlanır.

3. **Paket İşleme**

   * Her gelen IPv4 paketi `hook_fn()` tarafından işlenir.
   * Spinlock ile engel portu okunur.
   * Paket, engel portu ile eşleşirse düşürülür, aksi halde geçer.
   * Sayaçlar atomik olarak güncellenir.

4. **Modül Kapatma**

   * Cihaz düğümü kaldırılır.
   * Netfilter hook’u kayıttan çıkarılır.

---

## 5. Neden Spinlock ve Atomikler Kullanılıyor?

* **Spinlock**: Kısa süreli kritik bölgelerde (burada engel portu değişkeni) aynı anda hem process bağlamı hem softirq bağlamı erişebileceği için veri tutarlılığını korur.
* **Atomic değişkenler**: Çok çekirdekli sistemlerde sayaçların eşzamanlı artırılmasında veri kaybını önler.

---
