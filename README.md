# LCC_Basic_2servo_2frog_6input_6outputs30action

/*
**Disclaimer and Limitation of Liability**

This sketch (software) has been developed specifically for the **ESP32 Devkit 1** and the **SN65HVD230** CAN transceiver module. It has only been tested on the author’s personal model railway layout.

**The sketch is provided “AS IS” and “AS AVAILABLE”**, without any warranties or guarantees of any kind. The author explicitly disclaims all warranties, whether express, implied, or statutory, including but not limited to any warranties of merchantability, fitness for a particular purpose, accuracy, reliability, or non-infringement.

The author accepts **no responsibility or liability** for:
- Any malfunction, failure, or unexpected behaviour of the sketch
- Damage to hardware, loss of data, or disruption to your layout
- Incompatibility caused by updates to third-party libraries, Arduino core, JMRI, or other software
- Any direct, indirect, incidental, consequential, or punitive damages arising from the use or inability to use this sketch

This code is offered strictly for **educational and hobbyist purposes** to help railway modellers learn how to use the OpenLCB Single Thread Library. It is not intended for commercial use, safety-critical applications, or any situation where failure could cause damage or injury.

By downloading, using, or modifying this sketch, you acknowledge that you assume **all risk** and full responsibility for any outcomes resulting from its use.

The author reserves the right to modify or remove this sketch at any time without notice.
*/

/* ESP32 2 Servo 2 Frog switching 6-Input & 6-Output Combined Node
   ==============================================================
   - 2 Servo with 3 position events, Midpoint frog switching events, 2 target end point events
   - D32 & D33 Servo dedicaated pins
   - D25 & D26 Relay dedicated pins
   - 6 Output channels with multi-effects (Off, On, Flash, Strobe, Random)
   - 6 Input channels with  (Pull-up, Toggle) and Delays
   - CAN Transceiver Pins: RX pin 15, TX pin 2
   ==============================================================
*/
