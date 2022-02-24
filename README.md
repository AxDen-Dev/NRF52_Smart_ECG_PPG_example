
# NRF52832 Smart ECG PPG example

-------------------------

<table>
<tr align="center">
  <td> TOP </td>
</tr>
  <tr align="center">
    <td><img src="./assets/axden_ppg_ecg_board_top.jpg"></td>
  </tr>
  <tr align="center">
    <td> BOTTOM </td>
  </tr>
    <tr align="center">
      <td><img src="./assets/board_bottom.jpeg"></td>
    </tr>
</table>

-------------------------

Bluetooth Smart ECG PPG collects key vital signs information such as ECG, heart rate, SPO2, and temperature using AxDen's Aggregator Platform.
<br>
This is an example provided to quickly test various service scenarios that require communication between Android, iOS, and Aggregator.
<br>
<br>
Related hardware kits can be purchased from the Naver Smart Store.
<br>
[Purchase Link : naver smart store](https://smartstore.naver.com/axden)
<br>
<br>
You can purchase it by contacting sales@axden.io or [www.axden.io](http://www.axden.io/)
<br>

-------------------------

### Key features and functions.

MCU | Description
:-------------------------:|:-------------------------:
NRF52832 | BLE 5.0 / 5.1 / 5.2 / 5.3

Sensors | Description
:-------------------------:|:-------------------------:
MAX30003 | ECG(심전도), Heart rate(심박수) sensor
MAX30101 | SPO2(산소포화도), Heart rate(심박수) sensor
SI7051 | Temperature sensor

*If you need a body temperature measurement, please contact development@axden.io.*

The NRF52832 Smart ECG PPG example collects and transmits key vital signs information such as ECG (Ecardiogram), Heartrate (Heart Rate), SPO2 (Oxygen Saturated), and Temperature using Bluetooth communication.
<br>
<br>
Interwork with the AxDen Aggregator Platform to check sensor information on the Web and Mobile without building infrastructure such as servers and DBs.
<br>
<br>
Learn Edge AI using sensor information stored in the AxDen Aggregator Platform.
<br>

-------------------------

### Terminal & Android Application
<br>

terminal baudrate : 9600
<br>

[Donwload Android Application](https://play.google.com/store/apps/details?id=io.axden.module.example.axden_ble_module_example)
<br>

<table>
  <tr align="center">
    <td> Terminal </td>
  </tr>
  <tr align="center">
    <td><img src="./assets/axden_ecg_ppg_terminal.png"></td>
  </tr>
</table>

<table>
  <tr align="center">
    <td> </td>
    <td> Android </td>
    <td> </td>
  </tr>
  <tr align="center">
    <td><img src="./assets/axden_ppg_ecg_scan.jpg"></td>
    <td><img src="./assets/axden_ppg_ecg_0.jpg"></td>
    <td><img src="./assets/axden_ppg_ecg_1.jpg"></td>
  </tr>
</table>

<table>
  <tr align="center">
    <td>ECG Graph</td>
  </tr>
  <tr align="center">
    <td><img src="./assets/axden_ecg_graph.png"></td>
  </tr>
</table>

<table>
  <tr align="center">
    <td>PPG Graph</td>
  </tr>
  <tr align="center">
    <td><img src="./assets/PPG_Output.png"></td>
  </tr>
</table>

-------------------------

### How to check using AXDEN Aggregator Platform

Register the MAC Address of the device after signing up as a member on the AXDEN Aggregator Platform website.
<br>

Enter COMPANY ID nad DEVCIE ID provided on the AXDEN Aggregator Platform website into COMPANY_ID and DEVCIE_ID in the Protocol.h header file.
<br>

[AXDEN Aggregator Platfrom](http://project.axden.io/)
<br>

`#define COMPANY_ID 0`
<br>

`#define DEVICE_TYPE 0`
<br>

Complie and flash.
<br>
<br>
Check whether COMPANY_ID and DEVICE_ID are applied correctly through the terminal
<br>
<br>

Sensor information can be found on the Web or Mobile.
<br>

-------------------------

### Note
<br>

Works with SoftDevice S132 v7.2.0, provided with SDK 17.1.0.
<br>

To compile it, clone the repository in the [SDK]/examples/ble_peripheral folder.

-------------------------

### [SDK Download](https://github.com/AxDen-Dev/NRF52_Ping_pong_example)

-------------------------


### [Project import](https://github.com/AxDen-Dev/NRF52_Ping_pong_example)

-------------------------


### [Eclipse setting](https://github.com/AxDen-Dev/NRF52_Ping_pong_example)

-------------------------
