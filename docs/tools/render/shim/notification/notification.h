#pragma once
#define RECORD_NOTIFICATION "notification"
typedef struct NotificationApp NotificationApp;
typedef struct {
    int unused;
} NotificationSequence;
void notification_message(NotificationApp* app, const NotificationSequence* sequence);
