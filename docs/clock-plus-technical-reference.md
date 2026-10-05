# Clock Plus на STM32L010F4P6

## Технічна документація апаратної частини та firmware

**Статус документа:** первинна технічна редакція
**Дата звірки:** 2026-09-27
**Гілка:** `optimizations`
**Звірений коміт:** `ea0914f` (`Optimize l010 firmware size and idle mode`)
**Цільова плата:** STM32L010F4P6, TSSOP-20
**Проєкт CubeMX:** `l010f4p6.ioc`

Цей документ описує поточну реалізацію Clock Plus: електричну схему,
розведення контактів, периферію, формати даних, алгоритми firmware, режими
живлення, збірку, прошивання та відомі розбіжності між кодом і KiCad.

> Важливо: джерелом істини для поведінки пристрою є поточний код. KiCad-схема
> містить щонайменше одну критичну розбіжність із firmware у підключенні
> 74HC595. Перед виготовленням наступної ревізії плати її треба усунути.

## 1. Джерела даних

Документ складено за такими файлами:

- `Core/Src/main.c` - запуск, системна частота, RTC, головний цикл і стани
  живлення;
- `Core/Src/board.h` - апаратний API та формат 16-бітного LED-дисплея;
- `Core/Src/boards/l010f4p6/board.c` - GPIO, ADC, 74HC595, програмний I2C,
  AHT10, BH1750, батарея, buzzer і STANDBY;
- `Core/Src/boards/l010f4p6/board_config.h` - апаратні контакти, пороги й
  калібрування;
- `Core/Src/boards/l010f4p6/oled_128x32.c` - драйвер OLED 128x32;
- `Core/Src/app/*` - UI, автоматична зміна режимів, відображення, датчики й
  будильники;
- `Core/Src/boards/l010f4p6/README.md` - опис плати й інструкції збірки;
- `l010f4p6.ioc` - конфігурація STM32CubeMX;
- `STM32L010XX_FLASH.ld` - карта Flash/RAM;
- `CMakeLists.txt`, `CMakePresets.json`, `.vscode/*` та `openocd.cfg` - збірка,
  debug і прошивання;
- `docs/Clock i2c l0.pdf` - принципова схема KiCad;
- `docs/Clock i2c l0 pcb.pdf` - дев'ятисторінковий комплект шарів PCB;
- `docs/Clocl i2C l0 16 -2 595/Clock i2c l0.kicad_sch` - вихідна схема;
- `docs/Clocl i2C l0 16 -2 595/Clock i2c l0.kicad_pcb` - вихідна PCB.

## 2. Призначення та склад системи

Clock Plus - автономний бінарний годинник із двома засобами індикації:

- 16 світлодіодів через два послідовно з'єднані 74HC595;
- OLED 0.91 дюйма, 128x32, I2C, сумісний із SSD1306.

Функціональні вузли:

- RTC мікроконтролера від зовнішнього LSE 32.768 kHz;
- три незалежні щоденні будильники;
- активний buzzer через NPN-транзистор;
- AHT10 для температури й вологості;
- BH1750 для освітленості та автоматичної яскравості;
- PIR AM312/HC-SR312 для контролю присутності;
- п'ять кнопок через один ADC-резистивний ladder;
- контроль напруги 18650 через ADC;
- Sleep між обробками та повний STANDBY при зникненні основного
  живлення;
- SWD для програмування та налагодження.

## 3. Мікроконтролер і ресурси

| Параметр | Значення |
| --- | --- |
| MCU | STM32L010F4P6 |
| Ядро | Arm Cortex-M0+ |
| Корпус | TSSOP-20 |
| Flash | 16 KiB, адреса `0x08000000` |
| SRAM | 2 KiB, адреса `0x20000000` |
| Активна частота | MSI range 6, 4.194304 MHz |
| Частота очікування | MSI range 5, 2.097152 MHz |
| RTC clock | LSE 32.768 kHz |
| Системний tick | SysTick, 1 ms під час виконання; компенсація часу Sleep через LPTIM1/LSE |
| Період головного циклу | 20 ms активний, 100 ms із погашеними екранами |
| Мова | C11 із GNU extensions |
| HAL/CMSIS | STM32CubeL0 FW 1.12.4 |

### 3.1 Поточне використання пам'яті

Перевірено збірками Debug і Release 2026-09-27. Через однакову оптимізацію
`-Os` і LTO їхній виконуваний образ має однаковий розмір.

| Область | Використано | Доступно | Частка |
| --- | ---: | ---: | ---: |
| Flash | 14,100 B | 16,384 B | 86.06% |
| RAM разом із резервом heap/stack | 1,776 B | 2,048 B | 86.72% |
| `.text` | 13,592 B | - | - |
| `.rodata` | 256 B | - | - |
| `.data` | 52 B | - | - |
| `.bss` | 188 B | - | - |
| `_user_heap_stack` | 1,536 B | - | - |

Налаштування linker script:

- мінімальний heap: `0x200` = 512 B;
- мінімальний stack: `0x400` = 1024 B;
- запас Flash до межі регіону: 2,284 B;
- запас RAM з урахуванням зарезервованих heap/stack: 272 B.

У firmware не використовується динамічна пам'ять, тому резерв heap можна
переглянути окремою оптимізацією після перевірки всіх бібліотечних залежностей.

## 4. Архітектура вихідного коду

```text
Core/Src/main.c
  запуск MCU, clock, ADC base, RTC, main loop, power-state orchestration

Core/Src/board.h
  апаратно-незалежні типи та API плати

Core/Src/boards/l010f4p6/
  board.c              GPIO, ADC, PWM, I2C, датчики, battery, standby
  board_config.h       контакти, пороги, калібрування
  oled_128x32.c/.h     OLED SSD1306-compatible

Core/Src/app/
  alarm/               три alarm slots, trigger, buzzer, RTC backup storage
  display/             формування 16-бітної LED-маски
  environment/         планувальник читання AHT10
  ui/                  кнопки, edit states, auto modes
  core/                спільні типи та константи
```

Апаратна залежність зосереджена в `board.c`. Застосунок працює через функції
`Board_*`, але OLED поки що викликається напряму з `main.c`.

## 5. Карта контактів MCU

### 5.1 Карта, яку використовує поточний firmware

| Контакт MCU | Напрямок/периферія | Функція | Електричні примітки |
| --- | --- | --- | --- |
| PA0 | digital input / WKUP1 | наявність основного живлення, wake зі STANDBY | високий = живлення є; низький = підготовка до STANDBY |
| PA1 | ADC_IN1 | п'ять кнопок через resistor ladder | analog, без внутрішньої підтяжки |
| PA2 | GPIO output | 74HC595 SER/DS | serial data, pin 14 регістра |
| PA3 | GPIO output | 74HC595 RCLK/ST_CP | latch, pin 12 |
| PA4 | GPIO output | 74HC595 SRCLK/SH_CP | shift clock, pin 11 |
| PA5 | TIM2_CH1 AF5 | 74HC595 OE | active-low PWM, pin 13 |
| PA6 | GPIO output | активний buzzer через NPN | high = звук увімкнено |
| PA7 | digital input | PIR motion | high = рух; внутрішньої підтяжки немає |
| PA9 | GPIO open-drain | software I2C SCL | OLED, AHT10, BH1750 |
| PA10 | GPIO open-drain | software I2C SDA | OLED, AHT10, BH1750 |
| PA13 | SWDIO | програмування/debug | роз'єм SWD |
| PA14 | SWCLK | програмування/debug | роз'єм SWD |
| PB1 | ADC_IN9 | BAT+ sense | через резистивний дільник |
| PC14 | OSC32_IN | LSE input | кварц 32.768 kHz |
| PC15 | OSC32_OUT | LSE output | кварц 32.768 kHz |
| NRST | reset input | hardware reset/connect-under-reset | зовнішня підтяжка до VDD MCU |
| PB9/BOOT0 | boot selection | штатно притиснутий до GND | очікується normal boot from Flash |

### 5.2 CubeMX проти runtime-конфігурації

У `l010f4p6.ioc` описані PA0, PA1, PA2-PA5, PB1, PC14/PC15 та SWD. Контакти
PA6, PA7, PA9 і PA10 ініціалізуються вручну в board layer і не повністю
представлені в CubeMX. Після повторної генерації CubeMX необхідно перевіряти:

- збереження всіх блоків `USER CODE`;
- `Board_Init()` для PA6, PA7 і програмного I2C;
- CMake source list;
- ручну ініціалізацію ADC, системної частоти та SysTick.

### 5.3 Критична розбіжність KiCad і firmware

Поточна схема `Clock i2c l0.kicad_sch` підписує сигнали так:

| Контакт | KiCad-схема | Поточний firmware |
| --- | --- | --- |
| PA2 | RCLK/ST_CP | SER/DS |
| PA3 | OE | RCLK/ST_CP |
| PA4 | SER/DS | SRCLK/SH_CP |
| PA5 | SRCLK | OE/TIM2_CH1 |

Це не косметична різниця: PA5 потрібен firmware як TIM2_CH1 для PWM. Перед
замовленням PCB треба або перерозвести KiCad під firmware, або синхронно змінити
`board_config.h`, GPIO і TIM2. Поточна перевірена програмна карта наведена в
розділі 5.1.

## 6. Апаратна схема

### 6.1 PCB

Параметри поточного KiCad PCB:

| Параметр | Значення |
| --- | --- |
| Габарит | 120 x 80 mm |
| Шари міді | 2: F.Cu, B.Cu |
| Матеріал | FR4 |
| Товщина плати | 1.6 mm |
| Мідь | 35 um на кожному боці |
| Кількість footprints | 77 |
| Монтажні отвори | 4 x 3.3 mm |
| Координати отворів від origin KiCad | (22.65, 22.65), (22.65, 97.45), (137.35, 22.65), (137.35, 97.35) mm |

У `docs/Clock i2c l0 pcb.pdf` наведено дев'ять сторінок виробничого перегляду
шарів. Джерело PCB має кілька історичних snapshot-файлів `before-*`; актуальним
вважається файл без суфікса.

### 6.2 Основні компоненти зі схеми

| Позначення | Компонент/номінал | Призначення |
| --- | --- | --- |
| U1 | STM32L010F4Px | MCU |
| U2, U3 | 74HC595, SOIC-16 | 16 LED outputs |
| Y1 | 32.768 kHz crystal | LSE для RTC |
| C7, C8 | 15 pF | load capacitors LSE |
| C1, C2 | 100 nF | локальне decoupling VDD/VDDA |
| C4 | 10 uF | bulk decoupling VDD MCU |
| C3 | polarized, номінал у схемі не заданий | резервне живлення/накопичення |
| R2 | 10 kOhm | NRST pull-up |
| R3 | 220 kOhm | PA0/WKUP pull-down у схемі |
| R5 | 470 kOhm | верхнє плече BAT+ sense |
| R8 | 1 MOhm | нижнє плече BAT+ sense |
| R6, R9 | 4.7 kOhm | pull-up SCL/SDA до VCC peripheral |
| R7 | 10 kOhm | PB9/BOOT0 pull-down до GND |
| R27 | 10 kOhm | pull-up button ladder до VCC peripheral |
| R28-R32 | 1 k, 2 k, 4.7 k, 10 k, 20 kOhm | MODE, SET, UP, DOWN, OFF |
| R10 | 2 kOhm | base resistor buzzer transistor |
| Q1 | 2N2222 | low-side key активного buzzer |
| R11-R26 | 200 Ohm | струмообмеження 16 LED |
| BT1 | 18650 | акумулятор |
| J3 | BMS, 4 pin | BMS connection |
| J4 | buck-boost, 4 pin | перетворювач до 3.3 V |
| J2 | SWD, 5 pin | SWCLK, SWDIO, GND, VDD MCU, NRST |
| J5 | OLED, 4 pin | SDA, SCL/SCK, VCC peripheral, GND |
| J6 | GY-302/BH1750, 4 pin | VCC, GND, SCL, SDA |
| J7 | AHT10, 4 pin | VCC, GND, SCL, SDA |
| J1 | AM312/HC-SR312, 3 pin | PIR output, VCC, GND |
| BZ1 | active buzzer | звуковий сигнал |

На схемі D1-D4 та R1 мають загальні позначення без конкретного part number або
номіналу. Їх треба конкретизувати в BOM перед виробництвом.

### 6.3 Живлення

Схема розділяє щонайменше такі мережі:

- `+3.3V` - основний вихід buck-boost;
- `VCC периферія` - OLED, сенсори, 74HC595, LED, PIR і buzzer;
- `VDD MCU` - VDD та VDDA мікроконтролера;
- `Bms+`, `Bms-` - акумуляторна сторона BMS;
- `GND`.

Між основним живленням, периферією та MCU використано діоди D1-D4, резистор R1
і полярний накопичувальний конденсатор C3. Ідея вузла - не допустити живлення
вимкненої периферії через GPIO/I2C і дати RTC/MCU час коректно перейти в
STANDBY.

Firmware додатково переводить зовнішні лінії в analog/no-pull перед STANDBY,
щоб зменшити leakage і back-powering. Діодна розв'язка не скасовує вимоги, що
сигнал на вході MCU не повинен перевищувати VDD MCU більш ніж допускає
datasheet.

### 6.4 Decoupling і LSE

- C1 і C2: 100 nF біля VDD та VDDA;
- C4: 10 uF на `VDD MCU`;
- LSE: Y1 32.768 kHz між PC14 і PC15;
- C7 і C8: по 15 pF від кожного виводу кварцу до GND;
- доріжки LSE повинні бути короткими, симетричними й віддаленими від LED/PWM;
- точне значення load capacitors слід перевірити за `CL` конкретного кварцу та
  паразитною ємністю PCB. 15 pF - значення поточної схеми, а не універсальне
  правило.

## 7. RTC і календар

RTC тактується від зовнішнього LSE:

| Налаштування | Значення |
| --- | --- |
| LSE | 32.768 kHz |
| Drive | `RCC_LSEDRIVE_HIGH` |
| Формат часу | 24 години |
| Async prescaler | 127 |
| Sync prescaler | 255 |
| RTC output | disabled |

Під час запуску `App_RTC_Init()`:

1. відкриває доступ до backup domain;
2. скидає backup domain, якщо RTC clock source не LSE;
3. запускає LSE через `HAL_RCC_OscConfig()`;
4. перевіряє `LSERDY`;
5. вибирає LSE для RTC та вмикає RTC;
6. викликає `HAL_RTC_Init()`.

Якщо запуск LSE або RTC не вдався, функція повертає 0, але `main()` зараз
ігнорує результат і продовжує запуск. Окремої індикації LSE failure немає.

Поточний час читається раз на 1000 ms. За вимогою STM32 HAL після
`HAL_RTC_GetTime()` одразу викликається `HAL_RTC_GetDate()` для коректного
розблокування shadow registers.

Рік зберігається як `00..99` та інтерпретується UI як 2000-2099. Високосний
рік визначається через `year % 4 == 0`, що коректно для цього діапазону.
`Board_WriteDate()` записує weekday як Monday; weekday у UI не використовується
і не обчислюється з дати.

## 8. 16-LED дисплей і 74HC595

Використано два каскадно з'єднані 74HC595. Firmware передає спочатку `board2`
від MSB до LSB, потім `board1` від MSB до LSB, після чого формує імпульс latch.

### 8.1 Формат `ClockDisplay_t`

| Байт/біт | Значення у звичайних режимах |
| --- | --- |
| `board1 bit0` | верхній ряд, вага 32 |
| `board1 bit1` | верхній ряд, вага 16 |
| `board1 bit2` | верхній ряд, вага 8 |
| `board1 bit3` | верхній ряд, вага 4 |
| `board1 bit4` | верхній ряд, вага 2 |
| `board1 bit5` | верхній ряд, вага 1 |
| `board1 bit6` | нижній ряд, вага 32 |
| `board1 bit7` | нижній ряд, вага 16 |
| `board2 bit0` | нижній ряд, вага 8 |
| `board2 bit1` | нижній ряд, вага 4 |
| `board2 bit2` | нижній ряд, вага 2 |
| `board2 bit3` | нижній ряд, вага 1 |
| `board2 bit4` | T/time; у alarm mode статус selected slot |
| `board2 bit5` | D/date; у alarm mode старший біт номера slot |
| `board2 bit6` | E/environment; у alarm mode молодший біт номера slot |
| `board2 bit7` | alarm indicator |

### 8.2 Значення режимів

- **TIME:** верхній ряд - години у binary, нижній - хвилини;
- **DATE:** верхній ряд - місяць, нижній - день;
- **ENVIRONMENT:** верхній ряд - температура у binary; нижній - шкала
  вологості;
- **ALARM:** два ряди - час вибраного будильника; mode LEDs кодують номер і
  enabled state;
- **BATTERY:** верхній ряд вимкнений, нижній - шкала з 0..6 LED.

Шкала вологості реалізована так:

| RH | Кількість LED |
| ---: | ---: |
| `<=30%` | 0 |
| 40% | 1 |
| 50% | 2 |
| 60% | 3 |
| 70% | 4 |
| 80% | 5 |
| `>=90%` | 6 |

LED шкали заповнюються від молодшої ваги: 1, 2, 4, 8, 16, 32.

### 8.3 PWM яскравості

OE регістрів активний низьким рівнем. TIM2_CH1 налаштований вручну:

| Регістр | Значення |
| --- | --- |
| PSC | 7 |
| ARR | 99 |
| CCR1 | 0..100 |
| PWM mode | mode 1, preload enabled |
| Polarity | inverted через `CC1P` |

При 4.194304 MHz PWM має приблизно 5.24 kHz; у режимі очікування дисплей уже
вимкнений, тому зміна core clock не впливає на видиму яскравість.

BH1750 задає 10 дискретних рівнів у діапазоні 2-50%.

## 9. Програмний I2C

Шина реалізована bit-bang GPIO, без апаратного I2C, DMA та interrupt transfer.

| Параметр | Значення |
| --- | --- |
| SCL | PA9 |
| SDA | PA10 |
| GPIO mode | open-drain output |
| Внутрішні pull-up | увімкнені |
| Зовнішні pull-up у KiCad | 4.7 kOhm на кожній лінії |
| Software delay | 8 коротких loop iterations на edge |
| Clock stretching | не підтримується явно |
| Bus arbitration | не підтримується |

На одній шині працюють:

| Пристрій | 7-bit address |
| --- | ---: |
| OLED | `0x3C`, fallback `0x3D` |
| AHT10 | `0x38` |
| BH1750 | `0x23` |

OLED має власну копію low-level I2C-функцій, а AHT10/BH1750 використовують
реалізацію з `board.c`. Доступ до шини не арбітрується mutex/state machine, але
всі операції виконуються послідовно з одного main loop.

## 10. OLED 128x32

Поточний драйвер орієнтований на SSD1306-compatible 128x32:

| Параметр | Значення |
| --- | --- |
| Розмір | 128 x 32 px |
| Сторінки | 4 сторінки по 8 px |
| Power-on delay | 80 ms |
| Probe retries | 3 |
| Text chunk | 24 bytes/columns |
| Мінімальний redraw interval | 200 ms |

Драйвер:

- пробує адресу `0x3C`, потім `0x3D`;
- виконує bus recovery перед probe;
- має вбудований 5x7 font;
- масштабує текст 2x у два OLED pages;
- кешує останні відображені значення й не перерисовує незмінний екран;
- групує символи в I2C chunks до 24 bytes;
- очищає весь framebuffer OLED при переходах до/з alarm і battery та при
  переході між alarm list/edit;
- під час звичайного оновлення часу/дати пише лише потрібні області.

Поточна відрисовка синхронна. Окремої черги дрібних OLED jobs і функції
`Process()` немає; довгі clear/battery operations блокують main loop на час
I2C-транзакцій.

Формати:

| Mode | OLED output |
| --- | --- |
| TIME | `HH:MM:SS`; `A` у правому нижньому куті, якщо є enabled alarm |
| DATE | `DD.MM.YY` |
| ENVIRONMENT | `Txx Hxx` або `T-- H--` |
| ALARM list | `*A1 HH:MM ON`, `-A2 --:-- OFF` тощо |
| ALARM edit | один slot великим шрифтом, `..` над hours/minutes |
| BATTERY | `BAT xx.x%` та смуга 120 px |
| Active alarm | `ALARM n`, запис один раз до завершення alarm |

## 11. AHT10

| Параметр | Значення |
| --- | --- |
| Address | `0x38` |
| Bus recovery | 9 SCL pulses та STOP перед initialization |
| Soft reset | `BA`, після 40 ms очікування; далі 20 ms до init command |
| Init command | `E1 08 00` |
| Init delay | 40 ms після init command |
| Measure command | `AC 33 00` |
| Conversion wait | 80 ms |
| Busy timeout | 200 ms після завершення measure command |
| Response | 6 bytes |
| Мінімальний interval | 15 s |
| Busy flag | byte 0, bit `0x80` |

Humidity і temperature вилучаються як 20-бітні значення. Результат округлюється
до цілих одиниць:

- humidity обмежується 0..100% RH;
- temperature обмежується 0..99 C;
- від'ємні temperature зараз примусово стають 0 C;
- LED-дисплей додатково обмежує temperature до 60.

Measurement розділена на start і read, тому 80 ms очікування не виконується
через блокувальний `HAL_Delay`. Відлік починається після завершення measure
command, а не до initialization. Busy response залишає measurement pending
для наступної спроби scheduler; після 200 ms дані позначаються invalid і стан
initialization скидається. При помилці запису/читання драйвер також повторно
ініціалізує сенсор із bus recovery та soft reset.

## 12. BH1750

| Параметр | Значення |
| --- | --- |
| Address | `0x23` |
| Power on | `0x01` |
| Reset | `0x07` |
| Mode | continuous high-resolution, `0x10` |
| Read interval | 5 s |
| Lux conversion | `raw * 5 / 6`, приблизно `raw / 1.2` |

Мапування яскравості:

| Параметр | Значення |
| --- | ---: |
| Темний поріг | 2 lux |
| Верхній поріг | 400 lux |
| Рівні | 10 |
| Мінімальна яскравість | 2% |
| Максимальна яскравість | 50% |
| Hysteresis | 5 lux |

У разі відсутності/помилки BH1750 зберігається попередня PWM brightness.

## 13. Кнопки через ADC ladder

Схема:

```text
VCC peripheral -- 10k -- PA1 / ADC_IN1
                           |
                           +-- MODE -- 1k   -- GND
                           +-- SET  -- 2k   -- GND
                           +-- UP   -- 4.7k -- GND
                           +-- DOWN -- 10k  -- GND
                           +-- OFF  -- 20k  -- GND
```

ADC threshold values:

| Button | Максимальне значення ADC |
| --- | ---: |
| MODE | 470 |
| SET | 950 |
| UP | 1630 |
| DOWN | 2330 |
| OFF | 3360 |
| NONE | `>3360` |

ADC 12-bit, full scale 4095. Для кожного читання firmware виконує дві
конверсії та використовує останню. Подія звичайної кнопки формується на зміні
`NONE -> button`; auto-repeat для UP/DOWN не реалізований. OFF обробляється
окремо для long press.

Керування:

- MODE: `time -> date -> environment -> alarm -> battery -> time`;
- SET: перехід між edit fields;
- UP/DOWN: зміна значення або вибір alarm slot;
- OFF short: вихід із edit або toggle selected alarm;
- OFF hold 2 s: якщо щось enabled - вимкнути всі; інакше ввімкнути всі
  configured alarms;
- будь-яка кнопка під час звучання лише вимикає buzzer; її UI-команда
  відкидається до повного відпускання.

## 14. Будильники

### 14.1 Модель

- три slots: A1, A2, A3;
- кожен має `configured`, `enabled`, `hour`, `minute`;
- seconds не налаштовуються;
- trigger перевіряється кожні 20 ms (100 ms у режимі очікування) проти кешованого RTC часу, який
  оновлюється раз на секунду;
- hardware RTC Alarm interrupt не використовується;
- однакова hour/minute не запускається повторно в межах тієї самої хвилини;
- редагування або повторне ввімкнення slot очищає trigger guard;
- наступного дня той самий alarm знову спрацьовує, бо guard очищується після
  зміни minute.

### 14.2 Звуковий pattern

| Параметр | Ticks | Час при 50 Hz |
| --- | ---: | ---: |
| Buzzer on | 5 | 100 ms |
| Пауза між beep | 7 | 140 ms |
| Один beep step | 12 | 240 ms |
| Beeps у group | 3 | - |
| Group pause | 45 | 900 ms |
| Повний group | 81 | 1.62 s |
| Максимальна тривалість alarm | 3000 | 60 s |

Використовується активний buzzer: firmware лише подає/знімає DC керування на
PA6. Генерування звукової частоти PWM не потрібне.

### 14.3 Формат RTC backup storage

Firmware резервує 16 bytes у BKP0R-BKP3R і використовує перші 12 bytes.
Packing little-endian по чотири bytes на 32-bit register.

| Byte | Вміст |
| ---: | --- |
| 0 | signature `0xA7` |
| 1 | version `0x01` |
| 2 | A1 flags: bit0 configured, bit1 enabled |
| 3 | A1 hour |
| 4 | A1 minute |
| 5..7 | A2 flags/hour/minute |
| 8..10 | A3 flags/hour/minute |
| 11 | checksum |

Checksum: сума bytes 0..10 modulo 256, потім XOR із `0x5A`.

Під час завантаження образ читається до трьох разів. Якщо signature, version
або checksum неправильні, alarm slots очищуються в RAM. Невалідний образ не
перезаписується автоматично до наступної зміни налаштувань.

## 15. Battery sense і ADC

### 15.1 Апаратний дільник

KiCad показує:

```text
Bms+ -- 470k -- PB1 / ADC_IN9 -- 1M -- GND
```

Це дає теоретичний коефіцієнт `1M / (470k + 1M) = 0.6803`. Firmware, однак,
відновлює напругу батареї за виміряною калібрувальною парою:

```text
2140 mV на PB1 відповідає 3650 mV батареї
battery_mV = sense_mV * 3650 / 2140
```

Ефективний калібрувальний коефіцієнт становить 0.5863 і не збігається з
ідеальним дільником 470k/1M. Це може враховувати фактичну плату, допуски,
помилку монтажу або інший установленний номінал. Перед серійною ревізією треба
повторно виміряти BAT+, PB1 і VDD та узгодити схему з константами.

### 15.2 ADC implementation

- ADC1 налаштовується напряму через регістри, без HAL ADC driver;
- channels: ADC_IN1, ADC_IN9 та internal VREFINT;
- 12-bit result, range 0..4095;
- увімкнений low-frequency mode;
- sampling time — 160.5 ADC clock cycles (`ADC_SMPR_SMPR`), включно з VREFINT;
- виконується hardware calibration;
- кожен channel читається двічі, використовується другий sample;
- sense voltage коригується через factory-calibrated VREFINT;
- напруга батареї вимірюється раз на 500 ms; low-battery check та percentage
  display використовують спільне кешоване значення;
- перед STANDBY ADC, VREFINT і regulator вимикаються.

### 15.3 Low-battery state

| Параметр | Значення |
| --- | ---: |
| Вхід у low battery | `<2900 mV` |
| Вихід із low battery | `>=3100 mV` |
| Confirm samples | 3 |
| Measurement interval | 500 ms |
| Hysteresis | 200 mV |
| Alarm LED blink | 1 s |

Підтвердження рахує лише нові вимірювання, а не повторні читання кешу.
Постійне перетинання порога підтверджується приблизно за 1–1.5 s.

Low-battery state не є STANDBY. У ньому:

- дисплеї та зовнішня індикація вимикаються;
- червоний alarm LED блимає раз на секунду;
- alarm trigger і buzzer продовжують працювати;
- кнопки продовжують читатися, тому alarm можна вимкнути;
- повернення вище 3.1 V відновлює активну частоту й time mode.

### 15.4 State-of-charge curve

Процент обчислюється piecewise-linear interpolation:

| Battery, mV | Charge, % |
| ---: | ---: |
| 4100 | 100 |
| 3800 | 90 |
| 3700 | 82 |
| 3600 | 73 |
| 3500 | 62 |
| 3400 | 50 |
| 3300 | 36 |
| 3200 | 22 |
| 3100 | 10 |
| 2900 | 0 |

Фільтр percentage: IIR `new = (old * 15 + sample + 8) / 16`. Відображене
значення оновлюється не частіше ніж раз на 2 s і лише при зміні не менше 2.0
percentage points. Значення 0% і 100% застосовуються одразу.

## 16. PIR і Sleep

PA7 читається як digital input без pull-up/pull-down. PIR повинен видавати
логічний рівень у межах 0..VDD MCU.

`userActivity` активний, якщо виконується хоча б одна умова:

- PA7 high;
- натиснута будь-яка кнопка;
- активне редагування;
- звучить alarm.

Без активності firmware очікує завершення повного фактичного циклу
`time -> date -> environment -> time`. Після цього:

- очищує LED і OLED;
- вимикає buzzer;
- знижує MSI з 4.194304 до 2.097152 MHz;
- збільшує інтервал обробки з 20 до 100 ms та спить між обробками;
- не читає AHT10/BH1750 і не оновлює OLED, бо повертається з `App_Tick()`
  одразу після перевірки motion/button/alarm.

Між обробками ядро входить у звичайний Sleep через `WFI`; STOP не
використовується. LPTIM1 від LSE 32.768 kHz генерує одноразове пробудження через
залишок інтервалу. Якщо обробка вже зайняла весь інтервал, наступна починається
одразу. SysTick зупиняється лише на час Sleep, а фактичний час LPTIM1 додається
до HAL tick із накопиченням дробових мілісекунд. Під час виконання коду SysTick
працює з кроком 1 ms, тому HAL delays і timeouts зберігають цей крок.
Handler замаскований до відновлення HAL tick і SysTick, але дозволений у NVIC
pending IRQ розбудить WFI або не дозволить заснути, якщо вже pending перед
WFI. Пробудження не залежить від event latch WFE або `SEVONPEND`.
Після читання лічильника LPTIM1
скидається, щоб не очищувати його flags поза ISR (ES0483). Під час
синхронізації ARR із LSE SysTick продовжує працювати. Якщо LSE не готовий,
використовується Sleep із пробудженням від звичайного SysTick кожні 1 ms.
Очікування синхронізації ARR обмежене timeout.

PA7, ADC-кнопки, battery та alarm перевіряються після кожного пробудження.
EXTI для PIR не використовується. Коротке натискання менш як 100 ms у режимі
очікування може бути пропущене. При виявленні активності частота відновлюється,
інтервал повертається до 20 ms, UI переходить до TIME. Low-battery mode також
має інтервал 20 ms, щоб зберігався ритм buzzer. RTC читається раз на 1 s.
ADC, TIM2 PWM і живлення сенсорів цей Sleep path не вимикає.

## 17. Повний STANDBY

STANDBY запускається незалежно від battery percentage, коли PA0/WKUP1 low.
Перевірка PA0 виконується першою у кожному application tick (20 ms активний,
100 ms у режимі очікування), тому PIR не
може заблокувати перехід при зникненні основного живлення.

Послідовність `Board_EnterStandby()`:

1. очистити 74HC595 та OLED;
2. вимкнути buzzer;
3. зупинити TIM2 PWM і примусово підняти OE;
4. перевести зовнішні GPIO в analog/no-pull;
5. чекати PA0 low до 25 s; якщо не впав - скасувати STANDBY;
6. debounce low 300 ms; якщо сигнал повернувся high - скасувати;
7. вимкнути ADC, VREFINT, ADC regulator і ADC clock;
8. вимкнути fast wake, увімкнути ultra-low-power;
9. очистити debug bit у DBGMCU;
10. перевести решту GPIO в analog/no-pull, не змінюючи PA0;
11. disable WKUP1, очистити WUF до reset, enable WKUP1;
12. `HAL_PWR_EnterSTANDBYMode()`.

Wake від PA0 викликає reset-like startup. RTC і BKP-регістри зберігаються лише
поки backup domain має достатнє живлення. Firmware не використовує RTC alarm
або RTC wakeup timer для виходу зі STANDBY.

## 18. Автоматичні режими та таймінги UI

| Подія | Значення |
| --- | ---: |
| TIME auto duration | 15 s |
| DATE auto duration | 5 s |
| ENV auto duration | 5 s |
| Пауза auto після user action | 30 s |
| Edit inactivity timeout | 60 s |
| Mode/edit blink | 800 ms |
| Empty alarm error | 1 s |
| OLED redraw throttle | 200 ms |
| RTC refresh | 1 s |
| BH1750 read | 5 s |
| AHT10 read | 15 s |

Automatic cycle містить лише TIME, DATE та ENVIRONMENT. ALARM не переходить
автоматично. BATTERY після завершення 30 s user pause використовує default
5-second duration і переходить у TIME.

Під час утримання кнопки 30-second user pause постійно перезапускається. Відлік
фактично починається після відпускання. Через 60 s бездіяльності edit mode
закривається як short OFF.

## 19. Ініціалізація та головний цикл

Порядок startup:

1. custom `HAL_InitTick()`;
2. `HAL_MspInit()`;
3. MSI 4.194304 MHz;
4. base GPIO;
5. direct-register ADC setup;
6. LSE + RTC;
7. board peripherals;
8. OLED probe/init;
9. UI state;
10. alarm backup load;
11. environment scheduler;
12. infinite main loop.

Кожні 20 ms (100 ms у режимі очікування) `App_Tick()` виконує:

1. PA0/STANDBY check;
2. RTC cache refresh, якщо минула 1 s;
3. alarm trigger check;
4. ADC button read і alarm-button suppression;
5. low-battery state check;
6. activity/idle handling;
7. battery percentage read;
8. auto mode update;
9. environment scheduler;
10. buzzer pattern update;
11. BH1750 brightness update;
12. 74HC595 render/write;
13. OLED conditional render;
14. full-auto-cycle idle check.

## 20. Збірка

### 20.1 Вимоги

- CMake 3.22+;
- Ninja;
- GNU Arm Embedded toolchain із prefix `arm-none-eabi-` у `PATH`;
- STM32CubeL0 sources вже збережені в репозиторії.

### 20.2 Команди

Debug:

```bash
cmake --preset Debug
cmake --build --preset Debug
arm-none-eabi-size build/Debug/l010f4p6.elf
```

Release:

```bash
cmake --preset Release
cmake --build --preset Release
arm-none-eabi-size build/Release/l010f4p6.elf
```

Обидві конфігурації використовують `-Os`, `-ffunction-sections`,
`-fdata-sections`, `--gc-sections` і `-flto`. Debug додає `-g3`, Release -
`-g0`. Linker використовує `nano.specs`.

У source list навмисно додано тільки потрібний RTC HAL module. ADC і I2C HAL
drivers не лінкуються, бо ці підсистеми реалізовані напряму.

## 21. Прошивання і debug

### 21.1 OpenOCD

```bash
openocd -f openocd.cfg \
  -c "init; reset halt; program build/Debug/l010f4p6.elf verify; reset run; shutdown"
```

OpenOCD settings:

- ST-Link DAP;
- SWD;
- adapter speed 100 kHz;
- `connect_assert_srst` / connect under reset;
- target work area `0x800` bytes;
- `srst_only srst_nogate`.

### 21.2 STM32CubeProgrammer CLI

```bash
STM32_Programmer_CLI \
  -c port=SWD mode=UR freq=50 \
  -w build/Debug/l010f4p6.elf -v -rst
```

`mode=UR` потрібен для надійного підключення під reset, особливо коли firmware
швидко переходить у STANDBY або стан низького споживання.

### 21.3 VS Code Cortex-Debug

Конфігурація `Debug OpenOCD`:

- device `STM32L010F4P6`;
- executable `build/Debug/l010f4p6.elf`;
- preLaunchTask: Debug build;
- remote timeout 20 s;
- reset run, 100 ms wait, halt, load, reset halt.

Cortex-M0+ цього MCU має обмежену кількість hardware breakpoints. Надлишкові
breakpoints дають OpenOCD error `Can not find free FPB Comparator`.

## 22. Error handling і діагностика

Поточні обмеження:

- `Error_Handler()` вимикає interrupts і входить у нескінченний loop;
- окремого error code на LED/OLED немає;
- failure `App_RTC_Init()` ігнорується в `main()`;
- OLED init failure лише встановлює `oledReady = 0`, LED-дисплей продовжує
  працювати;
- AHT10 failure показує invalid environment;
- BH1750 failure залишає попередню brightness;
- software I2C не має timeout на stuck SCL і не читає clock stretching;
- ADC ready/calibration loops переважно не мають аварійного timeout;
- weekday не обчислюється;
- negative AHT10 temperature не відображається;
- firmware не має brownout telemetry або журналу reset causes.

## 23. Відомі розбіжності та ризики

1. **74HC595 pin mapping:** KiCad не відповідає `board_config.h`; це треба
   виправити до виробництва.
2. **Battery divider calibration:** номінали 470k/1M не відповідають парі
   2140/3650 mV у firmware; потрібне повторне вимірювання.
3. **CubeMX clock report:** `.ioc` може показувати близько 2.097 MHz, але runtime
   стартує на 4.194304 MHz через ручну конфігурацію.
4. **Manual GPIO outside CubeMX:** PA6, PA7, PA9 і PA10 легко втратити під час
   необережної регенерації проєкту.
5. **LSE failure:** немає user-visible fault path, запуск продовжується.
6. **OLED blocking:** оптимізовано кешем і 24-byte chunks, але немає повністю
   асинхронного renderer.
7. **I2C duplication:** OLED і sensors мають дві low-level bit-bang
   реалізації, що збільшує Flash і ускладнює єдине відновлення bus.
8. **RAM margin:** лише 272 B понад зарезервовані heap/stack; великі локальні
   buffers небезпечні.
9. **Flash margin:** близько 2.23 KiB; нові features потребують size check.
10. **Schematic placeholders:** D1-D4, R1 і C3 не мають завершених BOM values.
11. **BOOT0:** очікується зовнішня прив'язка PB9 до GND; її слід перевірити на
    готовій PCB continuity test.
12. **PIR input:** PA7 без pull resistor; sensor output не повинен плавати при
    вимкненій периферії.

## 24. Контрольний список bring-up

### 24.1 До подачі живлення

- перевірити short між `+3.3V`, `VDD MCU`, `VCC peripheral` і GND;
- перевірити орієнтацію U1, U2, U3, D1-D4, C3, BT1 і Q1;
- перевірити фактичний pinout 2N2222 конкретного корпусу;
- перевірити PB9/BOOT0 -> GND;
- перевірити NRST pull-up 10 kOhm;
- перевірити LSE і два 15 pF capacitors;
- перевірити відповідність 74HC595 wiring до firmware, а не лише до KiCad;
- перевірити SCL/SDA pull-up 4.7 kOhm до правильної rail.

### 24.2 Перше живлення

- подати лабораторне 3.3 V з current limit;
- виміряти VDD і VDDA без значної різниці;
- переконатися, що NRST high, BOOT0 low;
- під'єднати SWD із NRST і використати connect under reset;
- прошити й verify ELF;
- перевірити запуск LSE та зміну seconds;
- перевірити обидва 74HC595 тестовим pattern;
- перевірити PWM на PA5/OE;
- просканувати фактичні I2C addresses;
- перевірити всі п'ять ADC button ranges;
- калібрувати BAT+ проти PB1 і показу firmware;
- перевірити PA0 low -> STANDBY та PA0 high -> wake;
- перевірити зникнення peripheral power на відсутність back-power через I2C.

### 24.3 Функціональний regression test

- TIME, DATE, ENV, ALARM, BATTERY на LED та OLED;
- дата 28/29 February для високосного й невисокосного року;
- три alarm slots, persistence після power cycle;
- повторне спрацювання наступного дня;
- зупинка alarm будь-якою кнопкою без виконання її UI action;
- long OFF toggle all;
- low battery 2.9 V та recover 3.1 V;
- buzzer продовжує звучати у low-battery mode;
- один повний auto cycle до Sleep із погашеними екранами;
- LPTIM1/LSE wake через залишок 20/100 ms, відсутність 1 ms SysTick wake під час Sleep;
- wake від PIR і кнопок;
- full STANDBY тільки від PA0 low;
- відновлення RTC і alarm backup після wake.

## 25. Рекомендовані наступні технічні роботи

Пріоритет 1:

- синхронізувати 74HC595 mapping у KiCad і firmware;
- зафіксувати точні part numbers D1-D4, R1 та C3;
- повторно відкалібрувати battery divider мінімум у трьох точках;
- додати явну індикацію LSE/RTC initialization failure.

Пріоритет 2:

- об'єднати дві software-I2C реалізації;
- додати timeout і bus-recovery для всіх I2C transactions;
- винести OLED у неблокувальну чергу малих jobs;
- зменшити linker heap reservation після stack-usage audit;
- додати build-time size limit, щоб Flash/RAM overflow ловився в CI.

Пріоритет 3:

- обчислювати weekday;
- підтримати negative temperature;
- зберігати reset cause для сервісної діагностики;
- додати production self-test для LED, buttons, I2C sensors, buzzer і battery
  ADC.
