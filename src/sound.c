#include "sound.h"

#include "game_registry.h"
#include "paths.h"
#include "wmain.h"

static struct {
  Sound sound_nand_act;
  Sound sound_success;
  Sound sound_click1;
  Sound sound_click2;
  Sound sound_click3;
  Sound sound_oops;
  Sound sound2;
  float base_volume;
} C = {0};

void sound_init() {
  C.base_volume = .2f;
  C.sound_nand_act = load_sound_asset("sounds/nand_act.wav");
  C.sound_success = load_sound_asset("sounds/success.wav");
  C.sound_click1 = load_sound_asset("sounds/click.wav");
  C.sound_click2 = load_sound_asset("sounds/paintact.wav");
  C.sound_click3 = load_sound_asset("sounds/paintact2.wav");
  C.sound_oops = load_sound_asset("sounds/oops.wav");
  C.sound2 = load_sound_asset("sounds/click.wav");
  SetSoundVolume(C.sound_nand_act, .1);
  SetSoundVolume(C.sound2, C.base_volume);
  SetSoundVolume(C.sound_click1, C.base_volume);
  SetSoundVolume(C.sound_click2, C.base_volume);
  SetSoundVolume(C.sound_click3, C.base_volume);
  SetSoundVolume(C.sound_success, C.base_volume);
  SetSoundVolume(C.sound_oops, C.base_volume);
}

void play_sound_click() {
  if (is_paint_sound_on()) PlaySound(C.sound_click1);
}

void play_sound_paint(int al) {
  if (is_paint_sound_on() && !IsSoundPlaying(C.sound_click2)) {
    SetSoundPitch(C.sound_click2, 1 << al);
    PlaySound(C.sound_click2);
  }
}

void play_sound_nand() {
  // SetSoundPitch(C.sound_nand, ev.sound);
  if (is_circuit_sound_on()) {
    PlaySound(C.sound_nand_act);
  }
}

void play_sound_oops() { PlaySound(C.sound_oops); }

void play_sound_level_complete() { PlaySound(C.sound_success); }

