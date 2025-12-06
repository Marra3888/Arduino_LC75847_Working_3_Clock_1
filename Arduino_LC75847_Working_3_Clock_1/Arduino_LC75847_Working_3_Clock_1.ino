// Платформа Arduino Nano или Pro Mini, или ATmega 328Р
// Подключение: Nano SPI пины D13 и D11 и управления D10 (CE_PIN)
// LCD Подключение: фишка CN1 


#include <SPI.h>
#include <ctype.h>
#include <MD_DS3231.h>
#include <Wire.h>

// ===== Интерфейс =====
#define CE_PIN       10
#define LC_ADDRESS   0xA1
#define SPI_MODE_SEL SPI_MODE3
#define SPI_SPEED    200000

// ===== Поле =====
#define NUM_CELLS 16

enum Seg19 : uint8_t {
  SA, SB1, SB2, SC1, SC2, SD, SE1, SE2, SF1, SF2,
  SG1, SG2, SG3, SH1, SH2, SH3, SI1, SI2, SI3, SEG19_COUNT
};
#define SEG_BIT(s) (1UL << (s))

// ===== Карта D-номеров =====
// Логический 0 = самая левая ячейка, 15 = самая правая
// Порядок в строке: A ,B1, B2, C1, C2, D ,E1, E2, F1, F2, G1, G2, G3, H1, H2, H3, I1, I2, I3
const uint16_t CELL_MAP[NUM_CELLS][SEG19_COUNT] = {
  {  45, 33, 34, 35, 36, 44, 52, 51, 50, 53, 55, 42, 47, 41, 46, 54, 43, 48, 56 },  // логич. 0 (бывш. 15)
  {  57, 61, 62, 63, 64, 72, 80, 79, 78, 65, 67, 70, 59, 69, 58, 66, 71, 60, 68 },  // 1 (14)
  {  85, 73, 74, 75, 76, 84, 92, 91, 90, 93, 95, 82, 87, 81, 86, 94, 83, 88, 96 },  // 2 (13)
  {  97,101,102,103,104,  0,112,111,110,113,115,  0, 99,  0, 98,114,  0,100,116 },  // 3 (12)
  { 117,121,122,123,124,132,140,139,138,125,127,130,119,129,118,126,131,120,128 },  // 4 (11)
  { 145,133,134,135,136,144,156,151,150,149,154,142,147,141,146,153,143,148,155 },  // 5 (10)
  { 157,161,162,163,164,172,180,179,178,165,167,170,159,169,158,166,171,160,168 },  // 6 (9)
  { 185,173,174,175,176,184,192,191,190,193,195,182,187,181,186,194,183,188,196 },  // 7 (8)
  { 197,201,202,203,204,212,220,219,218,205,207,210,199,209,198,206,211,200,208 },  // 8 (7)
  { 225,213,214,215,216,224,232,231,230,233,235,222,227,221,226,234,223,228,236 },  // 9 (6)
  { 237,241,242,243,244,252,260,259,258,245,247,250,239,249,238,246,251,240,248 },  // 10 (5)
  { 265,253,254,255,256,264,272,271,270,273,275,262,267,261,266,274,263,268,276 },  // 11 (4)
  { 277,281,282,283,284,292,300,299,298,285,287,290,279,289,278,286,291,280,288 },  // 12 (3)
  { 305,293,294,295,296,304,312,311,310,313,315,302,307,301,306,314,303,308,316 },  // 13 (2)
  { 317,321,322,323,324,332,340,339,338,325,327,330,319,329,318,326,331,320,328 },  // 14 (1)
  { 345,333,334,335,336,344,352,351,350,353,355,342,347,341,346,354,343,348,356 }   // логич. 15 (бывш. 0, правая)
};

// ===== 7‑seg слой для области времени D2..D32 =====
#define seg_A  0
#define seg_B  1
#define seg_C  2
#define seg_D  3
#define seg_E  4
#define seg_F  5
#define seg_G  6

#define SEG7_BIT(s) (1U << (s))

// 7‑seg шрифт: биты A..G
const uint8_t FONT7_DIGIT[10] = {
  // gfedcba
  SEG7_BIT(seg_A) | SEG7_BIT(seg_B) | SEG7_BIT(seg_C) | SEG7_BIT(seg_D) | SEG7_BIT(seg_E) | SEG7_BIT(seg_F),             // 0
  SEG7_BIT(seg_B) | SEG7_BIT(seg_C),                                                                                     // 1
  SEG7_BIT(seg_A) | SEG7_BIT(seg_B) | SEG7_BIT(seg_D) | SEG7_BIT(seg_E) | SEG7_BIT(seg_G),                               // 2
  SEG7_BIT(seg_A) | SEG7_BIT(seg_B) | SEG7_BIT(seg_C) | SEG7_BIT(seg_D) | SEG7_BIT(seg_G),                               // 3
  SEG7_BIT(seg_B) | SEG7_BIT(seg_C) | SEG7_BIT(seg_F) | SEG7_BIT(seg_G),                                                 // 4
  SEG7_BIT(seg_A) | SEG7_BIT(seg_C) | SEG7_BIT(seg_D) | SEG7_BIT(seg_F) | SEG7_BIT(seg_G),                               // 5
  SEG7_BIT(seg_A) | SEG7_BIT(seg_C) | SEG7_BIT(seg_D) | SEG7_BIT(seg_E) | SEG7_BIT(seg_F) | SEG7_BIT(seg_G),             // 6
  SEG7_BIT(seg_A) | SEG7_BIT(seg_B) | SEG7_BIT(seg_C),                                                                   // 7
  SEG7_BIT(seg_A) | SEG7_BIT(seg_B) | SEG7_BIT(seg_C) | SEG7_BIT(seg_D) | SEG7_BIT(seg_E) | SEG7_BIT(seg_F) | SEG7_BIT(seg_G), // 8
  SEG7_BIT(seg_A) | SEG7_BIT(seg_B) | SEG7_BIT(seg_C) | SEG7_BIT(seg_D) | SEG7_BIT(seg_F) | SEG7_BIT(seg_G)              // 9
};

// Позиции:
// pos = 0 — десятки часов (Hour High)
// pos = 1 — единицы часов (Hour Low)
// pos = 2 — десятки минут (Minute High)
// pos = 3 — единицы минут (Minute Low)

// D‑номера A..G для каждой позиции
// (-1 = сегмент не используется или не заведен)
const int16_t TIME_SEG_DN[4][7] = {
  // pos 0: Hour High
  // A,    B,    C,    D,    E,    F,    G
  { 39,   38,   40,  -1,   -1,   -1,   39 },   // подправишь, если знаешь точные D для G/E/D

  // pos 1: Hour Low
  { 30,   18,   32,   28,   27,   26,   31 },

  // pos 2: Minute High
  { 10,   11,   12,   24,   20,   22,   23 },

  // pos 3: Minute Low
  { 7,    8,    4,    16,   15,   14,   3  }
};

// D‑номер двоеточия
const uint16_t TIME_COLON_DN = 19;

// ===== ИКОНКИ / МАРКЕРЫ =====
enum IconId : uint8_t {
  ICON_RDM,        // D2

  // Временные маркеры / подчёркивания вокруг F и D
  ICON_F_LOW,      // D9   (_F)
  ICON_TIRE,       // D13  (TIRE)
  ICON_D_MID,      // D21  (_D)
  ICON_F_RIGHT,    // D25  (F_)
  ICON_TIRE_RIGHT, // D29  (TIRE_)
  ICON_D_RIGHT,    // D37  (D_)

  ICON_SIGNAL,     // D6
  ICON_SCAN,       // D17
  ICON_RPT,        // D49
  ICON_COMA_UP,    // D77
  ICON_TIRE_2,     // D89  (TIRE__)
  ICON_F_2,        // D109 (F__)
  ICON_D_2,        // D137 (D__)
  ICON_KRAPKA,     // D152
  ICON_TP,         // D177
  ICON_TA,         // D189
  ICON_NEW,        // D217
  ICON_TRACK,      // D229
  ICON_A_SEL,      // D257
  ICON_FOLDER,     // D269
  ICON_ST,         // D297
  ICON_DISK,       // D309
  ICON_WMA,        // D337
  ICON_MP3,        // D349
  ICON_CATEGORY,   // D357
  ICON_CD2,        // D358
  ICON_CD1,        // D359
  ICON_CD4,        // D360
  ICON_CH,         // D361
  ICON_CD3,        // D362
  ICON_CD6,        // D363
  ICON_CD5,        // D364,

  ICON_COUNT
};

// D‑номера соответствуют твоей таблице
const uint16_t ICON_DN[ICON_COUNT] = {
  2,    // ICON_RDM

  9,    // ICON_F_LOW      (_F)
  13,   // ICON_TIRE       (TIRE)
  21,   // ICON_D_MID      (_D)
  25,   // ICON_F_RIGHT    (F_)
  29,   // ICON_TIRE_RIGHT (TIRE_)
  37,   // ICON_D_RIGHT    (D_)

  6,    // ICON_SIGNAL
  17,   // ICON_SCAN
  49,   // ICON_RPT
  77,   // ICON_COMA_UP
  89,   // ICON_TIRE_2
  109,  // ICON_F_2
  137,  // ICON_D_2
  152,  // ICON_KRAPKA
  177,  // ICON_TP
  189,  // ICON_TA
  217,  // ICON_NEW
  229,  // ICON_TRACK
  257,  // ICON_A_SEL
  269,  // ICON_FOLDER
  297,  // ICON_ST
  309,  // ICON_DISK
  337,  // ICON_WMA
  349,  // ICON_MP3
  357,  // ICON_CATEGORY
  358,  // ICON_CD2
  359,  // ICON_CD1
  360,  // ICON_CD4
  361,  // ICON_CH
  362,  // ICON_CD3
  363,  // ICON_CD6
  364   // ICON_CD5
};

 // Хвосты: [14] = 0x00, [15] = DD (00/01/10/11)
  byte Display_Data_0[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0x00, 0x00}; // DD=00
  byte Display_Data_1[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0x00, 0x01}; // DD=01
  byte Display_Data_2[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0x00, 0x02}; // DD=10
  byte Display_Data_3[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0x00, 0x03}; // DD=11


// ===== Класс драйвера =====
class LC75847 {
public:
 
  void begin(int chipEnabledPin, int address) {
    _chipEnabledPin = chipEnabledPin;
    _address = address;

    pinMode(_chipEnabledPin, OUTPUT);
    digitalWrite(_chipEnabledPin, HIGH); // CE HIGH в простое

    SPI.begin();
    SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE_SEL));

    clear();
  }

  // Очистка D‑бит
  void clear() {
    memset(Display_Data_0, 0, 14);
    memset(Display_Data_1, 0, 15);
    memset(Display_Data_2, 0, 15);
    memset(Display_Data_3, 0, 15);
    _print();
  }

  void display() { _print(); }

  // ===== Печать текста (16 ячеек) =====
  // firstCell — логический индекс 0..15 (0 — крайняя левая)
  void printText(uint8_t firstCell, const char* s, bool clearTail = true) {
    if (!s) return;
    uint8_t logical = firstCell;
    while (*s && logical < NUM_CELLS) {
      putChar(logical++, *s++);
    }
    if (clearTail && logical < NUM_CELLS)
      // clearCells(logical, NUM_CELLS - logical);
    display();
  }

  void putChar(uint8_t logicalCell, char ch) {
    setCellMask(logicalCell, glyph19(ch));
  }

  void clearCells(uint8_t firstLogical, uint8_t count) {
    for (uint8_t i = 0; i < count && (firstLogical + i) < NUM_CELLS; ++i)
      setCellMask(firstLogical + i, 0);
  }

  // Включить/выключить все сегменты всех 16 ячеек
  void lightAll(bool on) {
    for (uint8_t cell = 0; cell < NUM_CELLS; ++cell) {
      uint32_t mask = on ? ((1UL << SEG19_COUNT) - 1) : 0;
      setCellMask(cell, mask);
    }
    display();
  }

  void setD(uint16_t Dn, bool on) {
    if (Dn < 1 || Dn > 420) return;

    uint8_t* blk;
    uint16_t startD;
    if (Dn <= 108)      { blk = Display_Data_0; startD = 1;   }
    else if (Dn <= 212) { blk = Display_Data_1; startD = 109; }
    else if (Dn <= 316) { blk = Display_Data_2; startD = 213; }
    else                { blk = Display_Data_3; startD = 317; }

    uint16_t pos = (uint16_t)(Dn - startD);
    uint8_t  bi  = pos >> 3;
    uint8_t  bb  = pos & 7;
    if (bi > 13) return;

    if (on)  blk[bi] |=  (uint8_t)(1 << bb);
    else     blk[bi] &= (uint8_t)~(1 << bb);
  }

  // Логический индекс 0..15 -> та же строка CELL_MAP (0..15)
  // Направление справа налево
  // void setCellMask(uint8_t logicalCell, uint32_t mask19) {
  //   if (logicalCell >= NUM_CELLS) return;
  //   const uint16_t* row = CELL_MAP[logicalCell];
  //   for (uint8_t s = 0; s < SEG19_COUNT; ++s) {
  //     uint16_t dn = row[s];
  //     if (!dn) continue;
  //     setD(dn, (mask19 & SEG_BIT(s)) != 0);
  //   }
  // }

// Направление слева направо
  void setCellMask(uint8_t logicalCell, uint32_t mask19) {
  if (logicalCell >= NUM_CELLS) return;
  uint8_t phys = (uint8_t)(NUM_CELLS - 1 - logicalCell);
  const uint16_t* row = CELL_MAP[phys];  // 0 логич -> 15 физ, 15 логич -> 0 физ
  for (uint8_t s = 0; s < SEG19_COUNT; ++s) {
    uint16_t dn = row[s];
    if (!dn) continue;
    setD(dn, (mask19 & SEG_BIT(s)) != 0);
  }
}

private:
  int _chipEnabledPin = CE_PIN;
  int _address = LC_ADDRESS;
  int _textSpeed = 2;

  // Передача блоков: CE=LOW -> адрес; CE=HIGH -> 16 байт
  void _print() {
    // Блок 0
    digitalWrite(_chipEnabledPin, LOW);
    SPI.transfer((uint8_t)_address);
    digitalWrite(_chipEnabledPin, HIGH);
    for (uint8_t i = 0; i < 16; ++i) SPI.transfer(Display_Data_0[i]);

    // Блок 1
    digitalWrite(_chipEnabledPin, LOW);
    SPI.transfer((uint8_t)_address);
    digitalWrite(_chipEnabledPin, HIGH);
    for (uint8_t i = 0; i < 16; ++i) SPI.transfer(Display_Data_1[i]);

    // Блок 2
    digitalWrite(_chipEnabledPin, LOW);
    SPI.transfer((uint8_t)_address);
    digitalWrite(_chipEnabledPin, HIGH);
    for (uint8_t i = 0; i < 16; ++i) SPI.transfer(Display_Data_2[i]);

    // Блок 3
    digitalWrite(_chipEnabledPin, LOW);
    SPI.transfer((uint8_t)_address);
    digitalWrite(_chipEnabledPin, HIGH);
    for (uint8_t i = 0; i < 16; ++i) SPI.transfer(Display_Data_3[i]);

    digitalWrite(_chipEnabledPin, LOW); // завершить кадр
    delay(_textSpeed);
  }

  // ===== 19‑сегментный шрифт =====
  static uint32_t glyph19(char ch) {
    ch = toupper((unsigned char)ch);

    switch (ch) {
      // ===== ЦИФРЫ =====

      case '0':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD)  |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SF1) | SEG_BIT(SF2);

      case '1':
        return SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) | SEG_BIT(SH2);

      case '2':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SG3) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SD);

      case '3':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SG2) | SEG_BIT(SG3) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD);

      case '4':
        return SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SG3);

      case '5':
        return SEG_BIT(SA) |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SG3) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD);

      case '6':
        return SEG_BIT(SA) |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SG3) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD)  |
               SEG_BIT(SE1) | SEG_BIT(SE2);

      case '7':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2);

      case '8':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD)  |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SG3);

      case '9':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD)  |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SG3);

      // ===== БУКВЫ (примерный 19‑seg) =====

      case 'A':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SG3);

      case 'B': // как 8 без верхней палки
        return SEG_BIT(SA) |
               SEG_BIT(SB1) |
               SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD)  |
               SEG_BIT(SH1) | SEG_BIT(SI1) |
               SEG_BIT(SG2) | SEG_BIT(SG3);
               

      case 'C':
        return SEG_BIT(SA) |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SD);

      case 'D':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD)  |
               SEG_BIT(SH1) | SEG_BIT(SI1) |
               SEG_BIT(SG2);

      case 'E':
        return SEG_BIT(SA) |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SD)  |
               SEG_BIT(SG1) | SEG_BIT(SG2);

      case 'F':
        return SEG_BIT(SA) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SG1) | SEG_BIT(SG2);

      case 'G':
        return SEG_BIT(SA) |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SD) | SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SG3);

      case 'H':
        return SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SG3);

      case 'I':
        return SEG_BIT(SA) | SEG_BIT(SH1) | SEG_BIT(SI1) | SEG_BIT(SG2) | SEG_BIT(SD);

      case 'J':
        return SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD) | SEG_BIT(SI3) | SEG_BIT(SE1);

      case 'K':
        return SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SH2) | SEG_BIT(SI2) | SEG_BIT(SB1) | SEG_BIT(SC2);

      case 'L':
        return SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SD);

      case 'M':
        return SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SH3) | SEG_BIT(SH2)  | SEG_BIT(SG2);

      case 'N':
        return SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SI2) | SEG_BIT(SH3)  | SEG_BIT(SG2);

      case 'O':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD)  |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SF1) | SEG_BIT(SF2);

      case 'Q':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD)  |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SF1) | SEG_BIT(SF2) | SEG_BIT(SI2);

      case 'P':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SG3);

      case 'R':
        return SEG_BIT(SA) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SG3) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SI2) | SEG_BIT(SC2);

      case 'S':
        return SEG_BIT(SA) |
               SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SG1) | SEG_BIT(SG2) | SEG_BIT(SG3) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SD);

      case 'T':
        return SEG_BIT(SA) |
               SEG_BIT(SI1) | SEG_BIT(SH1) |
               SEG_BIT(SG2);

      case 'U':
        return SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SD);

      case 'V':
        return SEG_BIT(SF2) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SI2) | SEG_BIT(SH3) | SEG_BIT(SG2);

      case 'W':
        return SEG_BIT(SF1) | SEG_BIT(SF2) |
               SEG_BIT(SE1) | SEG_BIT(SE2) |
               SEG_BIT(SB1) | SEG_BIT(SB2) |
               SEG_BIT(SC1) | SEG_BIT(SC2) |
               SEG_BIT(SI2) | SEG_BIT(SI3) | SEG_BIT(SG2);

      case 'X':
        return SEG_BIT(SH3) | SEG_BIT(SH2) |
               SEG_BIT(SI2) | SEG_BIT(SI3) | SEG_BIT(SG2) | SEG_BIT(SF2) | SEG_BIT(SB1) | SEG_BIT(SC2) | SEG_BIT(SE1);

      case 'Y':
        return SEG_BIT(SI1) | 
               SEG_BIT(SH3) | SEG_BIT(SG2) | SEG_BIT(SH2) | SEG_BIT(SF2) | SEG_BIT(SB1);

      case 'Z':
        return SEG_BIT(SA) | SEG_BIT(SH2) |
               SEG_BIT(SB1) | SEG_BIT(SG2) |
               SEG_BIT(SE1) | SEG_BIT(SI3) | SEG_BIT(SD) | SEG_BIT(SF2) | SEG_BIT(SC2);

      case ':':
              // пример: точки H2 и I2 (подправь под своё стекло)
        return SEG_BIT(SH1) | SEG_BIT(SI1);
      
      case '=':
              // пример: точки H2 и I2 (подправь под своё стекло)
        return SEG_BIT(SG1) | SEG_BIT(SG3);

      case ' ':
        return 0;

      default:
        return 0;
    }
  }
};

// ===== Глобальный объект =====
LC75847 lcd;

// ===== НАСТРОЙКИ БУДИЛЬНИКА =====
const uint8_t ALARM_HOUR   = 7;   // во сколько часов сработает будильник
const uint8_t ALARM_MINUTE = 30;  // во сколько минут
const uint8_t ALARM_SECOND = 0;   // по нулевой секунде

bool alarmTriggeredToday = false;        // флаг: будильник уже сработал (для текущего дня)
bool blinkState = false;           // состояние мигания ICON_SIGNAL
unsigned long lastBlink = 0;
bool colonState = false;

// ===== БЕГУЩИЙ ТЕКСТ ПО ЯЧЕЙКАМ 0..15 (справа налево) =====
const uint16_t SCROLL_INTERVAL = 500;       // скорость прокрутки, мс
unsigned long lastScrollTime = 0;

const char scrollText[] =
  "Topo & Roby -Under The Ice (Mufti Edit)";            // строка для прокрутки

int16_t scrollOffset = 0;                  // смещение в строке
int16_t scrollLen = sizeof(scrollText) - 1; // без '\0'

// ===== СЛУЧАЙНОЕ ВКЛЮЧЕНИЕ ИКОНКИ =====
const uint16_t ICON_RANDOM_INTERVAL = 1000;  // мс между сменами иконки
unsigned long lastIconRandomTime = 0;

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------
void printTimeHHMMSS(uint8_t hours, uint8_t minutes, uint8_t seconds, bool colonOn, uint8_t position = 0) {
  char buf[9];  // "HH:MM:SS\0"

  buf[0] = '0' + (hours / 10);
  buf[1] = '0' + (hours % 10);
  buf[2] = colonOn ? ':' : '=';     // двоеточие или пробел

  buf[3] = '0' + (minutes / 10);
  buf[4] = '0' + (minutes % 10);
  buf[5] = colonOn ? ':' : '=';     // второе двоеточие (между M и S)

  buf[6] = '0' + (seconds / 10);
  buf[7] = '0' + (seconds % 10);
  buf[8] = '\0';

  // выводим слева, в логических ячейках 0..7
  lcd.printText(position, buf);
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Вывод времени HH:MM в область D2..D32
// десятки часов: 0 -> гасим, 1/2 -> показываем
void Clock_Display(uint8_t hours, uint8_t minutes, bool colonOn)
{
  uint8_t hHigh = hours / 10;
  uint8_t hLow  = hours % 10;
  uint8_t mHigh = minutes / 10;
  uint8_t mLow  = minutes % 10;

  // 1) Гасим все сегменты четырёх позиций и двоеточие
  for (uint8_t pos = 0; pos < 4; ++pos) {
    for (uint8_t s = 0; s < 7; ++s) {
      int16_t dn = TIME_SEG_DN[pos][s];
      if (dn > 0) lcd.setD((uint16_t)dn, false);
    }
  }
  lcd.setD(TIME_COLON_DN, false);

  // 2) Десятки часов: 0 -> гасим (digit = -1), 1/2 -> показываем
  int8_t hHighDigit = -1;
  if (hHigh == 1 || hHigh == 2) hHighDigit = (int8_t)hHigh;
  setTimeDigit(0, hHighDigit);

  // 3) Остальные цифры
  setTimeDigit(1, hLow);    // Hour Low
  setTimeDigit(2, mHigh);   // Minute High
  setTimeDigit(3, mLow);    // Minute Low

  // 4) Двоеточие
  lcd.setD(TIME_COLON_DN, colonOn);

  // 5) Отправляем буферы в драйвер
  lcd.display();
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Включить/выключить одну цифру позиции pos (0..3)
void setTimeDigit(uint8_t pos, int8_t digit) {
  if (pos > 3) return;

  uint8_t mask = 0;
  if (digit >= 0 && digit <= 9)
    mask = FONT7_DIGIT[digit];   // нормальная цифра
  else
    mask = 0;                     // "цифра" -1 = всё гасим

  for (uint8_t s = 0; s < 7; ++s) {
    int16_t dn = TIME_SEG_DN[pos][s];
    if (dn <= 0) continue;

    bool on = (mask & SEG7_BIT(s)) != 0;
    lcd.setD((uint16_t)dn, on);
  }
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
void setRtcManual(uint16_t year, uint8_t month, uint8_t day,
                  uint8_t hour, uint8_t minute, uint8_t second)
{
  // Заполняем интерфейсные регистры библиотеки
  RTC.yyyy = year;
  RTC.mm   = month;
  RTC.dd   = day;
  RTC.h    = hour;
  RTC.m    = minute;
  RTC.s    = second;

  // Записываем их в DS3231
  RTC.writeTime();
}

// ======================================================================================================================================================================
// После изменения состояния иконок нужно вызвать lcd.display(); (или обернуть в свою функцию).
// Включить/выключить иконку по enum
void setIcon(IconId icon, bool on) {
  if (icon >= ICON_COUNT) return;
  uint16_t dn = ICON_DN[icon];
  if (dn == 0) return;
  lcd.setD(dn, on);
}

// Включить/выключить иконку по D‑номеру напрямую (если нужно)
void setIconRaw(uint16_t dn, bool on) {
  if (dn < 1 || dn > 420) return;
  lcd.setD(dn, on);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------
void turnOnAllIcons() {
  for (uint8_t i = 0; i < ICON_COUNT; i++) {
    setIcon((IconId)i, true);
  }
  lcd.display();
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
void updateScrollText(uint8_t position = 0) {
  // логическая ячейка 0 — слева, 15 — справа
  for (int cell = position; cell < NUM_CELLS; ++cell) {
    // для движения СПРАВА НАЛЕВО: правый символ = scrollOffset,
    // слева — всё "раньше"
    int16_t idx = scrollOffset - (NUM_CELLS - 1 - cell);

    char ch = ' ';
    if (idx >= 0 && idx < scrollLen) {
      ch = scrollText[idx];
    }

    lcd.putChar(cell, ch);
  }

  lcd.display();
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------
void showRandomIcon() {
  // Погасить все иконки (если хочешь видеть только одну)
  for (uint8_t i = 0; i < ICON_COUNT; i++) {
    setIcon((IconId)i, false);
  }

  // Выбрать случайную иконку из диапазона 0..ICON_COUNT-1
  uint8_t idx = random(0, ICON_COUNT);
  setIcon((IconId)idx, true);

  lcd.display();
}

// ######################################################################################################################################################################
void setup() {
  Wire.begin();
  // Serial.begin(115200);

  // Инициализация RTC
  // RTC.begin();
  // при необходимости – установка времени:
  // RTC.setEpoch( ... );

  // Инициализация дисплея
  lcd.begin(CE_PIN, LC_ADDRESS);

  lcd.clear();

  // Инициализация генератора случайных чисел
  randomSeed(analogRead(0));  // или любой "шумный" пин

  // включить все варианты F/D/TIRE вокруг времени
  // turnOnAllIcons();  // включить все: RDM, _F, TIRE, _D, F_, TIRE_, D_, SIGNAL, SCAN, RPT, COMA_UP, ...

  // РУЧНАЯ установка: ГГГГ, ММ, ДД, ЧЧ, ММ, СС
  // например, 2025‑12‑31 23:59:00
  // setRtcManual(2025, 12, 4, 1, 39, 0);

  // Настройка ежедневного будильника Alarm1 на ALARM_HOUR:ALARM_MINUTE:ALARM_SECOND
  // Заполняем интерфейсные регистры RTC
  RTC.h = ALARM_HOUR;
  RTC.m = ALARM_MINUTE;
  RTC.s = ALARM_SECOND;
  // RTC.dow = 0;      // не используем день недели
  // RTC.dd  = 0;      // не используем дату

  // Записываем время Alarm1 с типом "часы, минуты, секунды совпали"
  RTC.writeAlarm1(DS3231_ALM_HMS);

  // Включаем прерывания от Alarm1 в контроллере DS3231
  RTC.control(DS3231_A1_INT_ENABLE, DS3231_ON);
  RTC.control(DS3231_INT_ENABLE,    DS3231_ON);
  // Сбрасываем флаг будильника
  RTC.control(DS3231_A1_FLAG, DS3231_OFF);
 
}

void loop() {
  if (millis() - lastBlink >= 500) {   // мигаем раз в 0.5 секунды
    lastBlink = millis();
    colonState = !colonState;
    blinkState = !blinkState;

    RTC.readTime();        // обновляем RTC

   uint8_t h = RTC.h;
   uint8_t m = RTC.m;
   uint8_t s = RTC.s;

  // printTimeHHMMSS(h, m, s, colonState, 0);

  // uint8_t h = RTC.h;
  // uint8_t m = RTC.m;

  // часы/минуты по старому 7‑seg‑шрифту в D2..D32
  Clock_Display(h, m, colonState);

  //  lcd.printText(9, "BD");

  // если будильник уже сработал, мигаем ICON_SIGNAL
    if (alarmTriggeredToday) {
      setIcon(ICON_SIGNAL, blinkState);
      lcd.display();
    }

  }

  // === ПРОВЕРКА БУДИЛЬНИКА ===
 // Проверка срабатывания аппаратного будильника
  if (!alarmTriggeredToday && RTC.checkAlarm1()) {
    alarmTriggeredToday = true;
    // флаг A1F библиотека сбросит внутри checkAlarm1()
    // сразу один раз "подмигнём" иконкой
    setIcon(ICON_SIGNAL, false);
    lcd.display();
  }

  // Сброс будильника на новый день (в 00:00:00, например)
  static uint8_t lastDay = 0;
  if (RTC.dd != lastDay) {
    lastDay = RTC.dd;
    alarmTriggeredToday = false;
    // будильник по‑прежнему включён -> ICON_SIGNAL снова постоянно горит
    setIcon(ICON_SIGNAL, true);
    lcd.display();
  }

  // === БЕГУЩИЙ ТЕКСТ СПРАВА НАЛЕВО В ЯЧЕЙКАХ 0..15 ===
  if (millis() - lastScrollTime >= SCROLL_INTERVAL) {
    lastScrollTime = millis();

    // сдвигаем "окно" вправо
    scrollOffset++;
    if (scrollOffset > scrollLen + NUM_CELLS) {
      scrollOffset = 0;   // начать прокрутку заново
    }
  }

      updateScrollText(0);

    // === СЛУЧАЙНАЯ ИКОНКА ===
  if (millis() - lastIconRandomTime >= ICON_RANDOM_INTERVAL) {
    lastIconRandomTime = millis();
    showRandomIcon();
  }
}