#pragma once

#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

const char* prefixClientID = "EnergyMeterSensor";

const char* sensorCommandTopic = "sensors/energy_meter/command";
const char* sensorCommandEnergy = "energy";
const char* sensorCommandVoltage = "voltage";
const char* sensorCommandCurrent = "current";
const char* sensorCommandPower = "power";
const char* sensorEnergyTopic = "sensors/energy_meter/energy";
const char* sensorVoltageTopic = "sensors/energy_meter/voltage";
const char* sensorCurrentTopic = "sensors/energy_meter/current";
const char* sensorPowerTopic = "sensors/energy_meter/power";

#define HOSTNAME_PREFIX "EnergyMeterSensor-" ///< Hostename. The setup function adds the Chip ID at the end.

#define BUILT_LED 16 //(0x10)
#define LED_ON 0x0
#define LED_OFF 0x1

#define SOFT_UART1_TX 16
#define SOFT_UART1_RX 14

#endif /* #ifndef __CONSTANTS_H__ */
