#include <8051.h>
#define T0_DAT 65535 - 921
#define TH_0 252

unsigned char timer_buf;
unsigned char send_buf;
__bit t0_flag;

// hh mm ss
unsigned char time[3] = {0, 0, 0};
unsigned char position = 0;
unsigned char direction = 1;
unsigned char skb[4] = {0, 0, 0, 0};
unsigned char last_key;

unsigned int count = 0;

__code unsigned char digits[10] = {0b0111111, 0b0000110, 0b1011011, 0b1001111,
                                   0b1100110, 0b1101101, 0b1111101, 0b0000111,
                                   0b1111111, 0b1101111};

unsigned char led[6] = {1, 3, 1, 2, 1, 0};

__bit __at(0x96) SEG_OFF;

__xdata unsigned char* csds = (__xdata unsigned char*)0xFF30;
__xdata unsigned char* csdb = (__xdata unsigned char*)0xFF38;

unsigned char led_addr[6] = {0b00100000, 0b00010000, 0b00001000,
                             0b00000100, 0b00000010, 0b00000001};

__bit edit_mode = 0;
unsigned char prev_led = 0;
unsigned char prev_dig = 0;

void display(unsigned char i) {
  SEG_OFF = 1;
  *csds = led_addr[i];
  if (edit_mode && count < 450 &&
      (i == (2 - position) * 2 + 1 || i == (2 - position) * 2))
    *csdb = 0b00000000;
  else {
    *csdb = digits[led[i]];
  }
  SEG_OFF = 0;
}

void update_led() {
  led[5] = time[2] % 10;
  led[4] = time[2] / 10;
  led[3] = time[1] % 10;
  led[2] = time[1] / 10;
  led[1] = time[0] % 10;
  led[0] = time[0] / 10;
}

void update_time() {
  if (time[2] < 59) {
    time[2]++;
  } else {
    time[2] = 0;
    if (time[1] < 59) {
      time[1]++;
    } else {
      time[1] = 0;
      if (time[0] < 23) {
        time[0]++;
      } else {
        time[0] = 0;
      }
    }
  }
  update_led();
}

// position 0:ss 1:mm 2:hh
void inc_time(unsigned char position) {
  switch (position) {
    case 0:
      if (time[2] < 59) {
        time[2]++;
      } else {
        time[2] = 0;
      }
      break;
    case 1:
      if (time[1] < 59) {
        time[1]++;
      } else {
        time[1] = 0;
      }
      break;
    case 2:
      if (time[0] < 23) {
        time[0]++;
      } else {
        time[0] = 0;
      }
      break;
  }
  update_led();
}

void dec_time(unsigned char position) {
  switch (position) {
    case 0:
      if (time[2] > 0) {
        time[2]--;
      } else {
        time[2] = 59;
      }
      break;
    case 1:
      if (time[1] > 0) {
        time[1]--;
      } else {
        time[1] = 59;
      }
      break;
    case 2:
      if (time[0] > 0) {
        time[0]--;
      } else {
        time[0] = 23;
      }
      break;
  }
  update_led();
}
// 0 == left, 1 == right
void change_position(unsigned char direction) {
  switch (direction) {
    case 0:
      if (position < 2) {
        position++;
      } else {
        position = 0;
      }
      break;
    case 1:
      if (position > 0) {
        position--;
      } else {
        position = 2;
      }
      break;
  }
}
void move_skb() {
  skb[3] = skb[2];
  skb[2] = skb[1];
  skb[1] = skb[0];
}
void check_keyboard(unsigned char i) {
  move_skb();
  skb[0] = i;
  last_key = i;
  if (skb[0] == skb[1] && skb[0] == skb[2] && skb[0] != skb[3]) {
    switch (i) {
      case 5: {
        edit_mode = 0;
        break;
      }
      case 4: {
        edit_mode = 1;
        break;
      }
      case 3: {
        if (edit_mode) change_position(1);
        break;
      }
      case 2: {
        if (edit_mode) inc_time(position);
        break;
      }
      case 1: {
        if (edit_mode) dec_time(position);
        break;
      }
      case 0: {
        if (edit_mode) change_position(0);
        break;
      }
    }
  }
}

void t0_int(void) __interrupt(1) {
  TH0 = TH_0;
  t0_flag = 1;
}

void main() {
  unsigned char led_i = 0;

  time[0] = led[1] + led[0] * 10;
  time[1] = led[3] + led[2] * 10;
  time[2] = led[5] + led[4] * 10;

  TMOD = 0b00100001;

  TH0 = TH_0;

  t0_flag = 0;
  ET0 = 1;
  EA = 1;
  TR0 = 1;

  while (1) {
    if (t0_flag) {
      t0_flag = 0;

      display(led_i);

      if (P3_5) check_keyboard(led_i);

      if (!P3_5 && last_key == led_i) skb[2] = 9;

      if (led_i < 5) {
        led_i++;
      } else {
        led_i = 0;
      }

      count++;
      // if (count % 301 == 0)
      // skb[2] = 9;  // pozwala na klikniecie tego samego klawisza 3 razy na
      //  sekunde [0, 301, 601] dokładnie co 1/3 sekundy

      if (count > 900) {
        count = 0;
        if (!edit_mode) update_time();
      }
    }
  }
}
