EESchema Schematic File Version 4
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
L Device:R_US R2
U 1 1 5BE91992
P 4650 950
F 0 "R2" H 4718 996 50  0000 L CNN
F 1 "10" H 4718 905 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 4690 940 50  0001 C CNN
F 3 "~" H 4650 950 50  0001 C CNN
	1    4650 950 
	1    0    0    -1  
$EndComp
$Comp
L Device:R_US R3
U 1 1 5BE919B4
P 4900 950
F 0 "R3" H 4968 996 50  0000 L CNN
F 1 "10" H 4968 905 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 4940 940 50  0001 C CNN
F 3 "~" H 4900 950 50  0001 C CNN
	1    4900 950 
	1    0    0    -1  
$EndComp
$Comp
L Device:R_US R1
U 1 1 5BE919DE
P 4150 950
F 0 "R1" H 4218 996 50  0000 L CNN
F 1 "22" H 4218 905 50  0000 L CNN
F 2 "Resistor_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal" V 4190 940 50  0001 C CNN
F 3 "~" H 4150 950 50  0001 C CNN
	1    4150 950 
	1    0    0    -1  
$EndComp
$Comp
L Device:D_Zener D1
U 1 1 5BE91AA6
P 4150 1800
F 0 "D1" V 4104 1879 50  0000 L CNN
F 1 "3V3" V 4195 1879 50  0000 L CNN
F 2 "Diode_THT:D_A-405_P7.62mm_Horizontal" H 4150 1800 50  0001 C CNN
F 3 "~" H 4150 1800 50  0001 C CNN
	1    4150 1800
	0    1    1    0   
$EndComp
$Comp
L Device:D_Zener D2
U 1 1 5BE91AF4
P 4650 1400
F 0 "D2" V 4604 1479 50  0000 L CNN
F 1 "5V1" V 4695 1479 50  0000 L CNN
F 2 "Diode_THT:D_A-405_P7.62mm_Horizontal" H 4650 1400 50  0001 C CNN
F 3 "~" H 4650 1400 50  0001 C CNN
	1    4650 1400
	0    1    1    0   
$EndComp
$Comp
L Device:C_Small C2
U 1 1 5BE91BB9
P 4900 1400
F 0 "C2" H 4992 1446 50  0000 L CNN
F 1 "0.1uF" H 4992 1355 50  0000 L CNN
F 2 "Capacitor_THT:C_Disc_D3.0mm_W1.6mm_P2.50mm" H 4900 1400 50  0001 C CNN
F 3 "~" H 4900 1400 50  0001 C CNN
	1    4900 1400
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C1
U 1 1 5BE91C21
P 4400 1800
F 0 "C1" H 4492 1846 50  0000 L CNN
F 1 "0.1uF" H 4492 1755 50  0000 L CNN
F 2 "Capacitor_THT:C_Disc_D3.0mm_W1.6mm_P2.50mm" H 4400 1800 50  0001 C CNN
F 3 "~" H 4400 1800 50  0001 C CNN
	1    4400 1800
	1    0    0    -1  
$EndComp
$Comp
L Connector:Screw_Terminal_01x02 J1
U 1 1 5BE91EEC
P 3550 1450
F 0 "J1" H 3470 1667 50  0000 C CNN
F 1 "5V1_Power_In" H 3470 1576 50  0000 C CNN
F 2 "TerminalBlock:TerminalBlock_bornier-2_P5.08mm" H 3550 1450 50  0001 C CNN
F 3 "~" H 3550 1450 50  0001 C CNN
	1    3550 1450
	-1   0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x04_Male J2
U 1 1 5BE91FEE
P 5450 1550
F 0 "J2" H 5423 1523 50  0000 R CNN
F 1 "3_Way_Power_Out" H 5423 1432 50  0000 R CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 5450 1550 50  0001 C CNN
F 3 "~" H 5450 1550 50  0001 C CNN
	1    5450 1550
	-1   0    0    -1  
$EndComp
Text HLabel 3450 1450 0    50   Input ~ 0
5V1_In
Text HLabel 3450 1550 0    50   UnSpc ~ 0
GND
Text HLabel 5550 1450 2    50   Output ~ 0
5V1_UnRegulated_Out
Text HLabel 5550 1550 2    50   Output ~ 0
5V1_200mA_Regulated_Out
Text HLabel 5550 1650 2    50   Output ~ 0
3V3_80mA_Regulated_Out
Text HLabel 5550 1750 2    50   UnSpc ~ 0
GND
Wire Wire Line
	5250 1450 5250 800 
Wire Wire Line
	5250 800  4900 800 
Wire Wire Line
	4900 800  4650 800 
Connection ~ 4900 800 
Wire Wire Line
	4650 800  4150 800 
Connection ~ 4650 800 
Wire Wire Line
	4650 1100 4900 1100
Wire Wire Line
	4650 1100 4650 1250
Connection ~ 4650 1100
Wire Wire Line
	4650 1950 4650 1550
Wire Wire Line
	4650 1950 5250 1950
Wire Wire Line
	5250 1950 5250 1750
Wire Wire Line
	4150 1950 4400 1950
Connection ~ 4650 1950
Wire Wire Line
	4150 1650 4400 1650
Wire Wire Line
	4400 1650 4400 1700
Wire Wire Line
	4400 1900 4400 1950
Connection ~ 4400 1950
Wire Wire Line
	4400 1950 4650 1950
Wire Wire Line
	4650 1250 4900 1250
Wire Wire Line
	4900 1250 4900 1300
Connection ~ 4650 1250
Wire Wire Line
	4900 1500 4900 1550
Wire Wire Line
	4900 1550 4650 1550
Connection ~ 4650 1550
Wire Wire Line
	4150 1100 4150 1650
Connection ~ 4150 1650
Wire Wire Line
	4900 1250 5050 1250
Wire Wire Line
	5050 1250 5050 1550
Wire Wire Line
	5050 1550 5250 1550
Connection ~ 4900 1250
Wire Wire Line
	4400 1650 5250 1650
Connection ~ 4400 1650
Wire Wire Line
	3750 1450 3750 800 
Wire Wire Line
	3750 800  4150 800 
Connection ~ 4150 800 
Wire Wire Line
	3750 1550 3750 1950
Wire Wire Line
	3750 1950 4150 1950
Connection ~ 4150 1950
Wire Wire Line
	3450 1550 3750 1550
Wire Wire Line
	3450 1450 3750 1450
Wire Wire Line
	5250 1450 5550 1450
Connection ~ 5250 1450
Wire Wire Line
	5250 1550 5550 1550
Connection ~ 5250 1550
Wire Wire Line
	5250 1650 5550 1650
Connection ~ 5250 1650
Wire Wire Line
	5250 1750 5550 1750
Connection ~ 5250 1750
Connection ~ 3750 1450
Connection ~ 3750 1550
$EndSCHEMATC
