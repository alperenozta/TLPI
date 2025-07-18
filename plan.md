---

```markdown
# 🎓 30 Günlük Staj Projesi Planı

## 📌 Proje Başlığı

**Ağ Trafiği ve Sistem Kaynaklarını İzleyen CLI + Daemon + Kernel Modül Tabanlı Sistem**

Amaç: Embedded Linux ortamında ağ trafiğini ve sistem kaynaklarını izleyen, CLI üzerinden yönetilebilen bir yapı kurmak.

---

## 🧩 Bileşenler

- **Kernel Modül (Netfilter tabanlı)**: Gelen paketleri filtreler ve istatistik toplar.
- **Daemon**: Kernel ile `ioctl` üzerinden haberleşir, sistem kaynaklarını (`/proc`) izler, log tutar.
- **CLI**: Daemon’la IPC üzerinden iletişim kurar, komutlar verir ve sonuçları kullanıcıya sunar.

---

## 📅 Haftalık Plan

### 🔵 Hafta 1 (Gün 1–5): Linux Süreçler ve Daemon Geliştirme

#### 🔧 Aktiviteler
- Embedded Linux giriş (serial konsol, telnet, busybox, mesh).
- `fork`, `exec`, `waitpid`, `setsid` kullanarak daemon yaz.
- `/proc/stat`, `/proc/meminfo` dosyalarından verileri oku.
- 5 saniyede bir sistem kaynaklarını loglayan daemon geliştir.

#### 📘 Kaynaklar
- TLPI Chapter 6 – Process Creation  
- TLPI Chapter 9 – Process Termination  
- TLPI Chapter 37 – Daemon Processes  
- TLPI Chapter 4 – File I/O  
- `/proc` dosya sistemi: [Linux Kernel Documentation - procfs](https://docs.kernel.org/filesystems/proc.html)

---

### 🔵 Hafta 2 (Gün 6–10): CLI ve IPC (UNIX Sockets)

#### 🔧 Aktiviteler
- UNIX domain sockets ile IPC geliştir.
- `select()` ile daemon’ın birden fazla client’ı yönetmesi sağlanır.
- CLI komutları: `show cpu`, `show mem`, `log level info`, `help` vb.

#### 📘 Kaynaklar
- TLPI Chapter 56 – UNIX Domain Sockets  
- TLPI Chapter 63 – I/O Multiplexing (select, poll, epoll ..)  
- TLPI Chapter 44 – Signals & Logging  
- TLPI Chapter 5 – File Descriptors  

---

### 🔵 Hafta 3 (Gün 11–17): Netfilter Kernel Modül Geliştirme

#### 🔧 Aktiviteler
- Basit bir Linux Kernel Modül yaz (init + exit).
- `NF_INET_PRE_ROUTING` ile paketleri say.
- TCP/UDP paket istatistiklerini tut, port engelleme işlemi uygula.
- `procfs` veya `ioctl` ile kullanıcı alanına bilgi taşı.

#### 📘 Kaynaklar
- Linux Device Drivers (LDD3) Chapter 2 – Building and Loading Modules  
- Netfilter Hook API: [https://www.netfilter.org](https://www.netfilter.org/)  
- TLPI Chapter 35 – /proc Interface  
- TLPI Chapter 57 – ioctl() Calls  
- https://www.fikridal.com/iptablesnetfilter-nasil-calisir/
- https://www.digitalocean.com/community/tutorials/a-deep-dive-into-iptables-and-netfilter-architecture
- https://github.com/wangm8/Netfilter-Kernel-Module
---

### 🔵 Hafta 4 (Gün 18–22): Kernel ↔ Daemon ↔ CLI Entegrasyonu

#### 🔧 Aktiviteler
- Daemon, kernel modülden `ioctl()` ile verileri çeker.
- CLI üzerinden gelen komutları işler:  
  `show traffic`, `block port`, `unblock port`
- Daemon, kernel modüle komutları yollar ve loglar.

#### 📘 Kaynaklar
- TLPI Chapter 57 – ioctl()  
- TLPI Chapter 55 – Client-Server Architectures  
- TLPI Chapter 4 – File I/O  
- LDD3 Chapter 3 – Character Devices  

---

### 🟢 Hafta 5 (Gün 23–27): Test, Loglama, Stabilite

#### 🔧 Aktiviteler
- Hatalı komut, bağlantı kopması gibi senaryoları test et.
- `log_level`, `error.log`, `debug.log` dosyaları ayrıştırılır.
- Reboot sonrası daemon yeniden başlıyor mu kontrol edilir.
- Log formatı sadeleştirilir, timestamp eklenir.

#### 📘 Kaynaklar
- TLPI Chapter 13 – Timers and Sleeping  
- TLPI Chapter 49 – Logging and syslog()  
- Bash scripting: `logger`, `sleep`, `cron`

---

### 🟢 Hafta 6 (Gün 28–30): Paketleme, Sunum ve Sonuç

#### 🔧 Aktiviteler
- CLI yardım sistemi (`--help`, `--version`) tamamlanır.
- Proje belgelenir: `README.md`, kullanım örnekleri, sistem diyagramı
- PC veya cihaz canlı demo yapılır.

#### 📘 Kaynaklar
- TLPI Chapter 62 – Building and Installing Software  
- TLPI Appendix A – Programming Tools  
- Markdown + Mermaid diyagramları (opsiyonel)

---

## 📂 Proje Dosya Yapısı Önerisi

```

monitor_project/
├── daemon/
│   └── resmon_daemon.c
├── cli/
│   └── resmon_cli.c
├── kernel/
│   └── netmon_ko.c
├── include/
│   └── ioctl_defs.h
├── Makefile
├── README.md
└── log/
└── traffic.log

```

---

## 📈 Öğrenim Kazanımları

| Alan               | Kazanım                                      |
|--------------------|-----------------------------------------------|
| Kernel Development | Netfilter hook, ioctl, modül yazımı           |
| IPC & CLI          | UNIX domain sockets, kullanıcı arayüzü        |
| Sistem İzleme      | CPU, RAM okuma, ağ trafiği sayımı             |
| Hata Yönetimi      | Loglama, test, hata toleransı                 |
| Otomasyon          | Makefile, yeniden başlatılabilir süreçler     |

---

Referans

https://www.markdownguide.org/cheat-sheet/

