#include "digivice_notify.h"

#include <notification/notification_messages.h>

// Red blink + short buzz: a VIRUX showed up while scanning.
const NotificationSequence digivice_sequence_virux = {
    &message_red_255,
    &message_vibro_on,
    &message_delay_100,
    &message_vibro_off,
    &message_red_0,
    &message_delay_100,
    &message_red_255,
    &message_delay_100,
    &message_red_0,
    NULL,
};

const NotificationSequence digivice_sequence_evolution = {
    &message_red_255,
    &message_blue_255,
    &message_note_c5,
    &message_delay_100,
    &message_note_e5,
    &message_delay_100,
    &message_note_g5,
    &message_delay_100,
    &message_note_c6,
    &message_delay_250,
    &message_sound_off,
    &message_red_0,
    &message_blue_0,
    NULL,
};

// Cyan stays on while the attack meter charges; the hit sequence clears it.
const NotificationSequence digivice_sequence_charge = {
    &message_red_0,
    &message_green_255,
    &message_blue_255,
    &message_do_not_reset,
    NULL,
};

const NotificationSequence digivice_sequence_hit = {
    &message_green_0,
    &message_blue_0,
    &message_red_255,
    &message_vibro_on,
    &message_delay_50,
    &message_vibro_off,
    &message_delay_50,
    &message_red_0,
    NULL,
};

const NotificationSequence digivice_sequence_win = {
    &message_green_255,
    &message_note_e5,
    &message_delay_100,
    &message_note_g5,
    &message_delay_100,
    &message_note_c6,
    &message_delay_250,
    &message_sound_off,
    &message_green_0,
    NULL,
};
