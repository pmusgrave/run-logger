# Run Logger
## Hardware
Logs your distance and time when you walk or run. This device uses a GPS module to track position, and includes an ESP32 WiFi/BLE microcontroller to synchronize data to the cloud automatically, using an extremely simple and familiar user interface. Just like a classic stopwatch, this has a simple 3-button, start, stop, reset interface.

Of course, it's possible to use a smartphone app to log your distance when you run. However, smartphones have a few drawbacks in my opinion:
1. They are expensive. I have dropped phones plenty of times and I don't want to risk that amount of wear on my phone when I exercise. Even if you don't drop the phone, exercise obviously produces a lot of sweat, and this moisture causes a lot of additional wear on phones. A lower cost solution gives me peace of mind that I won't be damaging a costly smartphone.
2. They are relatively heavy. It might not seem like much, but when you're running a long distance, any extra weight tends to add up over time.
3. They are relatively large and bulky. Again, it might not seem like much, but I personally dislike having a phone jostle around in my pocket in these situations. The less encumbrance the better.

This is a low-weight, low-cost, low-power wearable device to keep you focused on the trail in front of you.

## Software
This repository also includes software to log run events in a database and calculate some interesting metrics. For some reason, when I first started keeping track of running times, I used Google Calendar to plan when I would run. So for legacy reasons, some data was initially in Google Calendar and this repository has scripts to pull that data from the Google Calendar API. The software is split into the following components:
* A node server to listen to new events from the ESP32 hardware.
* A "warehouse" script to pull all events from Google Calendar, format them, and send them to the local database.
* An "analytics" script that calculates interesting metrics.
