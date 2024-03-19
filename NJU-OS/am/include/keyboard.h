#ifndef KEYBOARD_H
#define KEYBOARD_H
#include<am.h>
#include<amdev.h>
#include<klib-macros.h>
#include<lock.h>

#define KEY(keyname) AM_KEY_##keyname
typedef int keycode;

static spinlock_t keyboard_lock=unlocked;

bool keyboard_state=0;

// void keyboard_init(){
//   spinlock_lock(&keyboard_lock);
//   keyboard_state=io_read(AM_INPUT_CONFIG).present;
//   spinlock_unlock(&keyboard_lock);
//   return;
// }

keycode read_keyboard(){
  if(keyboard_state==0){  
      spinlock_lock(&keyboard_lock);
      AM_INPUT_KEYBRD_T info={0};
      ioe_read(AM_INPUT_KEYBRD,&info);
      spinlock_unlock(&keyboard_lock);
      if(info.keydown==0)return KEY(NONE);
      else return info.keycode;
  }
  return KEY(NONE);
}

#endif
