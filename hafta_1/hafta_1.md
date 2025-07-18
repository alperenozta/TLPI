
---

# Process Creation

`fork()` system call, bir parent processin yeni bir child process oluşturması için kullanılır. Bu işlem sonucunda child process parent processin bire bir kopyasını oluşturur. Child process parentin stack, data, heap ve text bölümlerini tamamen kopyalamaya olanak tanır. Gerekirse çocuk süreç, kendi PID’sini `getpid()` ile ve ebeveyninin PID’sini `getppid()` ile alabilir.

`exit(status)` fonksiyonu bir süreci sonlandırır; böylece süreç tarafından kullanılan tüm kaynaklar (bellek, açık dosya tanımlayıcıları vb.) çekirdek tarafından tekrar kullanılmak üzere serbest bırakılır. `status` argümanı, sürecin sonlandırma durumunu belirleyen bir tamsayıdır. Ebeveyn süreç, `wait()` sistem çağrısını kullanarak bu durumu öğrenebilir. `fork()` sistem çağrısından sonra parent veya child processlerinden sadece bir tanesi sonlanabilir; diğer süreç `_exit()` kullanılarak sonlandırılabilir.

`wait(&status)` sistem çağrısının iki amacı vardır. Birincisi, eğer bu sürecin bir çocuğu henüz `exit()` ile sonlanmadıysa, `wait()` bu süreçlerden biri sonlanana kadar ebeveyn süreci askıya alır. İkincisi, çocuğun sonlandırma durumu, `status` argümanı aracılığıyla geri döndürülür.

`execve(pathname, argv, envp)` sistem çağrısı, belirtilen programı (`pathname`) bir süreçteki belleğe yükler; argüman listesi `argv` ve çevresel değişken listesi `envp` ile birlikte. Mevcut program metni (kod kısmı) atılır ve yığın, veri ve heap bölümleri yeni program için sıfırdan oluşturulur. Bu işleme genellikle bir programın "exec edilmesi" denir.

---

### Örnek `fork()` yapısı kullanımı

```c
pid_t childPid; /* Used in parent after successful fork() to record PID of child */
switch (childPid = fork()) {
    case -1: /* fork() failed */
        /* Handle error */
    case 0: /* Child of successful fork() comes here */
        /* Perform actions specific to child */
    default: /* Parent comes here after successful fork() */
        /* Perform actions specific to parent */
}
```

---

# Daemons

Daemonlar belirli görevleri yerine getirmek için yazılırlar. Uzun ömürlüdürler, genellikle sistem başlatıldığında oluşturulur ve kapatılana kadar devam ederler. Arka planda çalışırlar ve kontrol terminali yoktur; bu sebeple kernel daemona hiçbir job control yapmaz veya terminalle ilgili sinyaller göndermez.

---

## Daemon Creation

1. `fork()` ile bir child process oluşturulur, parent süreç `exit()` ile sonlandırılır; bu sayede child process init processin alt processi haline gelir. Child Processin grup lideri olmaması garanti edilir. Eğer bir process session lider olursa `setsid()` yapılamaz.

2. Child process `setsid()` çağırarak yeni bir oturum ve process group başlatır; bu sayede processin hiçbir terminalle ilişkilendirilmemiş olur.

3. Eğer elde edilen daemonun terminal açma ihtimali varsa ikinci kez `fork()` işlemi uygulanır. Process kontrol terminali edinmesin diye ikinci fork() ile process session lider olamayacağı garantilenmiş olur.

4. `umask(0)` çağrılarak varsayılan dosya izin kısıtlaması sıfırlanır. Oluşturulan dosyaların erişim izinleri tam olarak istenilen gibi ayarlanabilir.

5. `chdir("/")` ile çalışma dizini kök dizine ayarlanır. Bunun nedeni, daemon'lar uzun süre çalıştığı için, mevcut dizin başka bir dosya sistemindeyse bu dosya sistemi `unmount` (ayrılma) edilemez. Eğer çalışma dizini `/` olursa böyle bir sorun yaşanmaz.

   Alternatif olarak, daemon kendisine özel bir klasöre de geçebilir, yeter ki bu klasörün bulunduğu dosya sistemi hiçbir zaman `unmount` edilmesin.

6. Dosya tanıtıcıları (file descriptor) kapatılır (özellikle 0, 1, 2: `stdin`, `stdout`, `stderr`). Terminalle bağlantı koparılır, gereksiz kaynaklar boşaltılır, `unmount` engelleri kaldırılır.

7. `/dev/null` açılır ve `stdin`, `stdout`, `stderr` ona yönlendirilir (`dup2()` ile). I/O işlemlerinin hata üretmemesi sağlanır. Açık kalan dosya tanıtıcılarının yanlış kullanılmasının önüne geçilir.

---

### Using SIGHUP to Reinitialize a Daemon

Daemonlar uzun süreli çalıştıkları için bazı zorluklar doğar:

* Yapılandırma dosyasını (config) sadece başlatıldıklarında okurlar. Çalışırken değişiklikleri fark etmezler.

* Log dosyalarını sürekli açık tuttukları için, zamanla aşırı büyüyerek disk alanını tüketebilirler.

Bu iki sorunu çözmek için **SIGHUP** sinyali kullanılır.

#### SIGHUP Nedir?

Aslen, bir terminal bağlantısı kesildiğinde ilgili sürece gönderilen bir sinyaldir.

Ancak daemon’lar kontrol terminali olmadığı için bu sinyali kendiliğinden almazlar.

Bu yüzden daemon’lar SIGHUP sinyalini özel amaçlarla kendileri kullanabilirler:

1. **Yapılandırma Dosyasını "Canlı" Olarak Güncelleme**
   Daemon başlatıldığında bir yapılandırma dosyasını (`/etc/mydaemon.conf`) okur.
   Eğer bu yapılandırma değişirse, normalde daemon’u yeniden başlatmak gerekir.
   Bunun yerine:

   * Konfigürasyon dosyası değiştirilir.
   * Daemon’a SIGHUP sinyali gönderilir:

     ```bash
     kill -HUP <pid>
     ```
   * Daemon bu sinyali yakalayarak konfigürasyonu yeniden okur.

2. **Log Dosyasını Döndürmek (Log Rotation)**
   Sürekli çalışan daemon’lar genellikle log dosyasına yazı yazar.
   Zamanla bu dosya çok büyüyebilir.

#### Tek Log Dosyasının Sürekli Büyümesi

* Log dosyası sürekli aynı dosyada yazılır ve büyür.
* Dosya boyutu artar, zamanla çok büyük dosyalar ortaya çıkar.
* Büyük dosyalar:

  * Disk üzerinde parçalanmaya (fragmentation) daha meyillidir.
  * İşletim sistemi ve dosya sistemi işlemlerinde (örn. yedekleme, arama, analiz) performans sorunları olabilir.
  * Yönetimi zorlaşır (dosyayı açmak ve işlemek zorlaşır).
* Ama toplamda kapladığı alan, kaydedilen veri kadar olur.

#### Çözüm:

* Log dosyasının ismi değiştirilir:

  ```bash
  mv /var/log/my.log /var/log/my.log.1
  ```
* SIGHUP sinyali gönderilir.
* Daemon log dosyasını kapatıp yeniden açar.

Bu şekilde:

* Yeni kayıtlar yeni dosyaya yazılır.
* Eski dosya silinebilir veya arşivlenebilir.

**Not:** Eğer log dosyası sadece `mv` ile yeniden adlandırılır ama hala açık tutuluyorsa, daemon hala eski dosyaya yazmaya devam eder. Bu yüzden sinyal gönderimi şarttır.

---

**Sonuç:**
SIGHUP sinyali sayesinde:

* Daemon'u durdurup yeniden başlatmaya gerek kalmadan konfigürasyon değiştirilebilir.
* Log dosyası döndürülerek disk dolması önlenir.
* Sistem servisleri daha güvenli, kararlı ve yönetilebilir hale gelir.

---

### Logging Messages and Errors Using syslog

Daemon arka planda çalıştığı için diğer programlarda olduğu gibi bağlı olduğu terminale mesaj yazdırması mümkün değildir. Alternatif olarak mesajları özel log dosyalarına kaydedilebilir ama bu yöntem çok fazla log dosyası oluşması ve dosya yönetimin zorlaşmasıdır. Bu sebeple **syslog** sistemi geliştirilmiştir.

---

#### Logging a Message

Bir log mesajı yazmak için `syslog()` fonksiyonu çağrılır.

```c
#include <syslog.h>
void syslog(int priority, const char *format, ...);
```

Bir log mesajı yazmak için `syslog()` fonksiyonu çağrılır. `priority` (öncelik) argümanı, bir **facility** (tesis) değeri ile bir **level** (seviye) değerinin OR işlemi ile birleştirilmesiyle oluşturulur. Facility, mesajı kaydeden uygulamanın genel kategorisini belirtir (Tablo 37-1).

Eğer belirtilmezse, facility daha önce yapılan bir `openlog()` çağrısında belirtilen değeri alır; eğer `openlog()` çağrısı yapılmamışsa varsayılan olarak `LOG_USER` kullanılır.

Level değeri ise mesajın ciddiyetini (önem derecesini) belirtir. (Tablo 37-2)

`syslog()` örnek kullanımı aşağıdaki gibidir;

```c
openlog(argv[0], LOG_PID | LOG_CONS | LOG_NOWAIT, LOG_LOCAL0);
syslog(LOG_ERROR, "Bad argument: %s", argv[1]);
syslog(LOG_USER | LOG_INFO, "Exiting");
```

İlk `syslog()` çağrısında herhangi bir facility belirtilmediği için `openlog` ile varsayılan facility olan `LOG_LOCAL0` kullanılır. İkinci `syslog()` çağrısında ise `LOG_USER` facility’si açıkça belirtilerek `openlog()` ile belirlenen varsayılan değer geçersiz kılınır.

---

#### Closing the log

Loglama işimiz bittikten sonra, `/dev/log` soketi için kullanılan dosya tanımlayıcısını serbest bırakmak amacıyla `closelog()` fonksiyonunu çağırabiliriz. Ancak, bir daemon genellikle sistem günlüğüne sürekli bağlı kalır, bu yüzden çoğu zaman `closelog()` çağrısını atlamak yaygındır.

```c
#include <syslog.h>
void closelog(void);
```

---

#### Filtering log messages

`setlogmask()` fonksiyonu, `syslog()` tarafından yazılan mesajları filtrelemek için kullanılan bir maske ayarlar. Maskenin ayarlarına uymayan bütün loglar atılır. Linux ve diğer birçok UNIX sürümünde `LOG_UPTO()` makrosu da vardır, bu makro belirtilen seviyeye kadar (dahil) tüm mesajları filtreler.

```c
setlogmask(LOG_UPTO(LOG_ERR));
```

---

Bu şekilde daemonlar, loglama ve yapılandırma yönetimini güvenli ve etkili biçimde yapabilirler.

---
