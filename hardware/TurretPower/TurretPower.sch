EESchema Schematic File Version 4
LIBS:TurretPower-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Device:R_US R1
U 1 1 5BE919DE
P 4150 1450
F 0 "R1" V 4000 1350 50  0000 L CNN
F 1 "22" V 4000 1500 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 4190 1440 50  0001 C CNN
F 3 "~" H 4150 1450 50  0001 C CNN
	1    4150 1450
	0    1    1    0   
$EndComp
$Comp
L Device:D_Zener D1
U 1 1 5BE91AA6
P 4150 1550
F 0 "D1" H 4150 1700 50  0000 L CNN
F 1 "3V3" H 3950 1700 50  0000 L CNN
F 2 "Diode_THT:D_A-405_P7.62mm_Horizontal" H 4150 1550 50  0001 C CNN
F 3 "~" H 4150 1550 50  0001 C CNN
	1    4150 1550
	-1   0    0    1   
$EndComp
$Comp
L Connector:Screw_Terminal_01x02 J1
U 1 1 5BE91EEC
P 3550 1450
F 0 "J1" H 3400 1800 50  0000 C CNN
F 1 "5V1_Power_In" H 3600 1700 50  0000 C CNN
F 2 "TerminalBlock:TerminalBlock_bornier-2_P5.08mm" H 3550 1450 50  0001 C CNN
F 3 "~" H 3550 1450 50  0001 C CNN
	1    3550 1450
	-1   0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x03_Male J2
U 1 1 5BE91FEE
P 4800 1450
F 0 "J2" H 4750 1800 50  0000 R CNN
F 1 "3_Way_Power_Out" H 4750 1700 50  0000 R CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical" H 4800 1450 50  0001 C CNN
F 3 "~" H 4800 1450 50  0001 C CNN
	1    4800 1450
	-1   0    0    -1  
$EndComp
Text HLabel 3450 1450 0    50   Input ~ 0
5V1_In
Text HLabel 3450 1550 0    50   UnSpc ~ 0
GND
Text HLabel 4900 1350 2    50   Output ~ 0
5V1_Power_Out
Text HLabel 4900 1450 2    50   Output ~ 0
3V3_Power_Out
Text HLabel 4900 1550 2    50   UnSpc ~ 0
GND
Wire Wire Line
	3450 1550 3750 1550
Wire Wire Line
	3450 1450 3750 1450
Wire Wire Line
	3750 1550 4000 1550
Connection ~ 3750 1550
Wire Wire Line
	3750 1450 4000 1450
Connection ~ 3750 1450
Wire Wire Line
	3750 1450 3750 1350
Wire Wire Line
	3750 1550 3750 1650
Wire Wire Line
	3750 1650 4600 1650
Wire Wire Line
	4600 1650 4600 1550
Wire Wire Line
	4600 1350 4900 1350
Wire Wire Line
	4600 1450 4900 1450
Wire Wire Line
	4600 1550 4900 1550
Connection ~ 4600 1550
Wire Wire Line
	4300 1450 4300 1550
Connection ~ 4600 1450
Wire Wire Line
	4300 1450 4600 1450
Connection ~ 4300 1450
Connection ~ 4600 1350
Wire Wire Line
	3750 1350 4600 1350
$EndSCHEMATC
