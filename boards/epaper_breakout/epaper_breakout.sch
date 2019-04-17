EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:switches
LIBS:relays
LIBS:motors
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:epaper_fpc24
LIBS:epaper_breakout-cache
EELAYER 25 0
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
NoConn ~ 9250 1500
$Comp
L L L1
U 1 1 5C4B6FD9
P 4250 3050
F 0 "L1" V 4200 3050 50  0000 C CNN
F 1 "10uH" V 4325 3050 50  0000 C CNN
F 2 "Inductors_SMD:L_1210_HandSoldering" H 4250 3050 50  0001 C CNN
F 3 "" H 4250 3050 50  0001 C CNN
	1    4250 3050
	0    1    1    0   
$EndComp
$Comp
L Earth #PWR01
U 1 1 5C4B704E
P 3900 3400
F 0 "#PWR01" H 3900 3150 50  0001 C CNN
F 1 "Earth" H 3900 3250 50  0001 C CNN
F 2 "" H 3900 3400 50  0001 C CNN
F 3 "" H 3900 3400 50  0001 C CNN
	1    3900 3400
	1    0    0    -1  
$EndComp
$Comp
L C C1
U 1 1 5C4B709E
P 3900 3250
F 0 "C1" H 3925 3350 50  0000 L CNN
F 1 "4.7uF" H 3925 3150 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 3938 3100 50  0001 C CNN
F 3 "" H 3900 3250 50  0001 C CNN
	1    3900 3250
	1    0    0    -1  
$EndComp
Wire Wire Line
	3900 3100 3900 3050
Wire Wire Line
	3750 3050 4100 3050
$Comp
L C C2
U 1 1 5C4B7142
P 4900 2600
F 0 "C2" H 4925 2700 50  0000 L CNN
F 1 "4.7uF" H 4925 2500 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 4938 2450 50  0001 C CNN
F 3 "" H 4900 2600 50  0001 C CNN
	1    4900 2600
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 5C4B7234
P 5150 3800
F 0 "R2" V 5230 3800 50  0000 C CNN
F 1 "3" V 5150 3800 50  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 5080 3800 50  0001 C CNN
F 3 "" H 5150 3800 50  0001 C CNN
	1    5150 3800
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 5C4B7292
P 4600 3800
F 0 "R1" V 4680 3800 50  0000 C CNN
F 1 "10k" V 4600 3800 50  0000 C CNN
F 2 "Resistors_SMD:R_0805_HandSoldering" V 4530 3800 50  0001 C CNN
F 3 "" H 4600 3800 50  0001 C CNN
	1    4600 3800
	1    0    0    -1  
$EndComp
Wire Wire Line
	5150 3450 5150 3650
Connection ~ 5150 3450
Connection ~ 4600 3550
Wire Wire Line
	4600 3950 4600 4100
Wire Wire Line
	4600 4100 5150 4100
Wire Wire Line
	5150 4100 5150 3950
$Comp
L Earth #PWR02
U 1 1 5C4B775E
P 4900 4150
F 0 "#PWR02" H 4900 3900 50  0001 C CNN
F 1 "Earth" H 4900 4000 50  0001 C CNN
F 2 "" H 4900 4150 50  0001 C CNN
F 3 "" H 4900 4150 50  0001 C CNN
	1    4900 4150
	1    0    0    -1  
$EndComp
Wire Wire Line
	4900 4150 4900 4100
Connection ~ 4900 4100
Wire Wire Line
	4900 2450 4900 2250
Wire Wire Line
	4550 2250 5300 2250
$Comp
L D D1
U 1 1 5C4B78B5
P 4550 2100
F 0 "D1" H 4550 2200 50  0000 C CNN
F 1 "MBR0530" H 4600 2000 50  0000 C CNN
F 2 "Diodes_SMD:D_SOD-123" H 4550 2100 50  0001 C CNN
F 3 "" H 4550 2100 50  0001 C CNN
	1    4550 2100
	0    -1   -1   0   
$EndComp
$Comp
L D D2
U 1 1 5C4B7951
P 5300 2100
F 0 "D2" H 5300 2200 50  0000 C CNN
F 1 "MBR0530" H 5250 2000 50  0000 C CNN
F 2 "Diodes_SMD:D_SOD-123" H 5300 2100 50  0001 C CNN
F 3 "" H 5300 2100 50  0001 C CNN
	1    5300 2100
	0    1    1    0   
$EndComp
Connection ~ 4900 2250
Wire Wire Line
	5300 1950 5300 1750
$Comp
L Earth #PWR03
U 1 1 5C4B79D4
P 5300 1750
F 0 "#PWR03" H 5300 1500 50  0001 C CNN
F 1 "Earth" H 5300 1600 50  0001 C CNN
F 2 "" H 5300 1750 50  0001 C CNN
F 3 "" H 5300 1750 50  0001 C CNN
	1    5300 1750
	-1   0    0    1   
$EndComp
Text GLabel 9250 3500 0    60   Input ~ 0
PREVGH
Text GLabel 9200 4100 3    60   Input ~ 0
VCOM
Wire Wire Line
	9200 3800 9200 4100
Wire Wire Line
	9200 3800 9250 3800
Text GLabel 8800 3850 0    60   Input ~ 0
PREVGL
Wire Wire Line
	8250 3700 9250 3700
Wire Wire Line
	9100 3700 9100 3850
Wire Wire Line
	9100 3850 8800 3850
Connection ~ 9100 3700
Wire Wire Line
	9200 4000 8250 4000
Connection ~ 9200 4000
Wire Wire Line
	9250 3600 8250 3600
Wire Wire Line
	9250 3400 8250 3400
Wire Wire Line
	9250 3300 8250 3300
Wire Wire Line
	9250 3200 8250 3200
Wire Wire Line
	9250 3100 7800 3100
$Comp
L Earth #PWR04
U 1 1 5C4B84B2
P 7800 3100
F 0 "#PWR04" H 7800 2850 50  0001 C CNN
F 1 "Earth" H 7800 2950 50  0001 C CNN
F 2 "" H 7800 3100 50  0001 C CNN
F 3 "" H 7800 3100 50  0001 C CNN
	1    7800 3100
	0    1    1    0   
$EndComp
Wire Wire Line
	9250 3000 8250 3000
Wire Wire Line
	8800 2900 9250 2900
Wire Wire Line
	8900 2900 8900 3000
Connection ~ 8900 3000
Text GLabel 8800 2900 0    60   Input ~ 0
3V3
Connection ~ 8900 2900
Text GLabel 9250 1600 0    60   Input ~ 0
GDR
Text GLabel 9250 1700 0    60   Input ~ 0
RESE
Text GLabel 5400 3450 2    60   Input ~ 0
RESE
Text GLabel 4450 3550 0    60   Input ~ 0
GDR
Wire Wire Line
	4450 3550 4600 3550
Text GLabel 4550 1950 1    60   Input ~ 0
PREVGL
$Comp
L D D3
U 1 1 5C4B8EAC
P 5550 3050
F 0 "D3" H 5550 3150 50  0000 C CNN
F 1 "MBR0530" H 5550 2950 50  0000 C CNN
F 2 "Diodes_SMD:D_SOD-123" H 5550 3050 50  0001 C CNN
F 3 "" H 5550 3050 50  0001 C CNN
	1    5550 3050
	-1   0    0    1   
$EndComp
Text GLabel 6150 3050 2    60   Input ~ 0
PREVGH
$Comp
L C_Small C6
U 1 1 5C4B97DC
P 8150 3000
F 0 "C6" V 8100 3050 50  0000 L CNN
F 1 "1uF" V 8100 2850 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8150 3000 50  0001 C CNN
F 3 "" H 8150 3000 50  0001 C CNN
	1    8150 3000
	0    1    1    0   
$EndComp
Wire Wire Line
	8050 3000 7800 3000
Connection ~ 7800 3100
$Comp
L C_Small C7
U 1 1 5C4B9978
P 8150 3200
F 0 "C7" V 8100 3250 50  0000 L CNN
F 1 "1uF" V 8100 3050 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8150 3200 50  0001 C CNN
F 3 "" H 8150 3200 50  0001 C CNN
	1    8150 3200
	0    1    1    0   
$EndComp
$Comp
L C_Small C8
U 1 1 5C4B99B3
P 8150 3300
F 0 "C8" V 8100 3350 50  0000 L CNN
F 1 "1uF" V 8100 3150 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8150 3300 50  0001 C CNN
F 3 "" H 8150 3300 50  0001 C CNN
	1    8150 3300
	0    1    1    0   
$EndComp
$Comp
L C_Small C9
U 1 1 5C4B99F1
P 8150 3400
F 0 "C9" V 8100 3450 50  0000 L CNN
F 1 "1uF" V 8100 3250 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8150 3400 50  0001 C CNN
F 3 "" H 8150 3400 50  0001 C CNN
	1    8150 3400
	0    1    1    0   
$EndComp
$Comp
L C_Small C10
U 1 1 5C4B9A2E
P 8150 3600
F 0 "C10" V 8100 3650 50  0000 L CNN
F 1 "1uF" V 8100 3450 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8150 3600 50  0001 C CNN
F 3 "" H 8150 3600 50  0001 C CNN
	1    8150 3600
	0    1    1    0   
$EndComp
$Comp
L C_Small C11
U 1 1 5C4B9A72
P 8150 3700
F 0 "C11" V 8100 3750 50  0000 L CNN
F 1 "1uF" V 8100 3550 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8150 3700 50  0001 C CNN
F 3 "" H 8150 3700 50  0001 C CNN
	1    8150 3700
	0    1    1    0   
$EndComp
$Comp
L C_Small C12
U 1 1 5C4B9AB5
P 8150 4000
F 0 "C12" V 8100 4050 50  0000 L CNN
F 1 "1uF" V 8100 3850 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8150 4000 50  0001 C CNN
F 3 "" H 8150 4000 50  0001 C CNN
	1    8150 4000
	0    1    1    0   
$EndComp
Wire Wire Line
	8050 3200 7800 3200
Connection ~ 7800 3200
Wire Wire Line
	8050 3300 7800 3300
Connection ~ 7800 3300
Wire Wire Line
	8050 3400 7800 3400
Connection ~ 7800 3400
Wire Wire Line
	8050 3600 7800 3600
Connection ~ 7800 3600
Wire Wire Line
	8050 3700 7800 3700
Connection ~ 7800 3700
Wire Wire Line
	8050 4000 7800 4000
Connection ~ 7800 4000
Wire Wire Line
	7800 4000 7800 3000
Text GLabel 3750 3050 0    60   Input ~ 0
3V3
Connection ~ 3900 3050
$Comp
L Earth #PWR05
U 1 1 5C4BAB17
P 7800 1950
F 0 "#PWR05" H 7800 1700 50  0001 C CNN
F 1 "Earth" H 7800 1800 50  0001 C CNN
F 2 "" H 7800 1950 50  0001 C CNN
F 3 "" H 7800 1950 50  0001 C CNN
	1    7800 1950
	0    1    1    0   
$EndComp
Wire Wire Line
	8250 1800 9250 1800
$Comp
L C_Small C4
U 1 1 5C4BABEA
P 8150 1800
F 0 "C4" V 8100 1850 50  0000 L CNN
F 1 "1uF" V 8100 1650 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8150 1800 50  0001 C CNN
F 3 "" H 8150 1800 50  0001 C CNN
	1    8150 1800
	0    1    1    0   
$EndComp
$Comp
L C_Small C5
U 1 1 5C4BACB9
P 8150 1950
F 0 "C5" V 8100 2000 50  0000 L CNN
F 1 "1uF" V 8100 1800 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 8150 1950 50  0001 C CNN
F 3 "" H 8150 1950 50  0001 C CNN
	1    8150 1950
	0    1    1    0   
$EndComp
Wire Wire Line
	9250 1900 8550 1900
Wire Wire Line
	8550 1900 8550 1950
Wire Wire Line
	8550 1950 8250 1950
Wire Wire Line
	7850 2200 9250 2200
Wire Wire Line
	7850 1800 8050 1800
Wire Wire Line
	7800 1950 8050 1950
Wire Wire Line
	7850 1800 7850 2200
Connection ~ 7850 1950
Text GLabel 9250 2000 0    60   Input ~ 0
TSCL
Text GLabel 9250 2100 0    60   Input ~ 0
TSDA
Text GLabel 9250 2300 0    60   Input ~ 0
BUSY
Text GLabel 9250 2400 0    60   Input ~ 0
RES#
Text GLabel 9250 2500 0    60   Input ~ 0
DC#
Text GLabel 9250 2600 0    60   Input ~ 0
CS#
Text GLabel 9250 2700 0    60   Input ~ 0
SCLK
Text GLabel 9250 2800 0    60   Input ~ 0
SDIO
Text GLabel 9450 4750 0    60   Input ~ 0
3V3
Text GLabel 9450 4850 0    60   Input ~ 0
GND
Text GLabel 9450 4950 0    60   Input ~ 0
SDIO
Text GLabel 9450 5050 0    60   Input ~ 0
SCLK
Text GLabel 9450 5150 0    60   Input ~ 0
CS#
Text GLabel 9450 5250 0    60   Input ~ 0
DC#
Text GLabel 9450 5350 0    60   Input ~ 0
RES#
Text GLabel 9450 5450 0    60   Input ~ 0
BUSY
Text GLabel 9450 5550 0    60   Input ~ 0
TSCL
Text GLabel 9450 5650 0    60   Input ~ 0
TSDA
$Comp
L Conn_01x10 J1
U 1 1 5C4BBD04
P 9650 5150
F 0 "J1" H 9650 5650 50  0000 C CNN
F 1 "Conn_01x10" H 9650 4550 50  0000 C CNN
F 2 "Socket_Strips:Socket_Strip_Straight_1x10_Pitch2.54mm" H 9650 5150 50  0001 C CNN
F 3 "" H 9650 5150 50  0001 C CNN
	1    9650 5150
	1    0    0    -1  
$EndComp
Wire Wire Line
	5700 3050 6150 3050
$Comp
L C_Small C3
U 1 1 5C4BC690
P 6050 3300
F 0 "C3" H 6060 3370 50  0000 L CNN
F 1 "1uF" H 6060 3220 50  0000 L CNN
F 2 "Capacitors_SMD:C_0805_HandSoldering" H 6050 3300 50  0001 C CNN
F 3 "" H 6050 3300 50  0001 C CNN
	1    6050 3300
	1    0    0    -1  
$EndComp
$Comp
L Earth #PWR06
U 1 1 5C4BC6F7
P 6050 3500
F 0 "#PWR06" H 6050 3250 50  0001 C CNN
F 1 "Earth" H 6050 3350 50  0001 C CNN
F 2 "" H 6050 3500 50  0001 C CNN
F 3 "" H 6050 3500 50  0001 C CNN
	1    6050 3500
	1    0    0    -1  
$EndComp
Wire Wire Line
	6050 3050 6050 3200
Connection ~ 6050 3050
Wire Wire Line
	6050 3400 6050 3500
Text GLabel 10400 4700 0    60   Input ~ 0
3V3
Text GLabel 10400 4850 0    60   Input ~ 0
GND
$Comp
L Earth #PWR07
U 1 1 5C4BCE3F
P 10700 4850
F 0 "#PWR07" H 10700 4600 50  0001 C CNN
F 1 "Earth" H 10700 4700 50  0001 C CNN
F 2 "" H 10700 4850 50  0001 C CNN
F 3 "" H 10700 4850 50  0001 C CNN
	1    10700 4850
	0    -1   -1   0   
$EndComp
$Comp
L +3.3V #PWR08
U 1 1 5C4BCE7D
P 10700 4700
F 0 "#PWR08" H 10700 4550 50  0001 C CNN
F 1 "+3.3V" H 10700 4840 50  0000 C CNN
F 2 "" H 10700 4700 50  0001 C CNN
F 3 "" H 10700 4700 50  0001 C CNN
	1    10700 4700
	0    1    1    0   
$EndComp
Wire Wire Line
	10700 4700 10400 4700
Wire Wire Line
	10700 4850 10400 4850
$Comp
L epaper_fpc24 P1
U 1 1 5C4BDD4E
P 9750 2550
F 0 "P1" H 9750 3750 60  0000 C CNN
F 1 "epaper_fpc24" H 9750 1200 60  0000 C CNN
F 2 "dianwei_fpc20:fpc24" H 9750 2550 60  0001 C CNN
F 3 "" H 9750 2550 60  0001 C CNN
	1    9750 2550
	1    0    0    -1  
$EndComp
$Comp
L Q_NMOS_GSD Q1
U 1 1 5CA56528
P 4800 3250
F 0 "Q1" H 5000 3300 50  0000 L CNN
F 1 "Si1308EDL" H 5000 3200 50  0000 L CNN
F 2 "TO_SOT_Packages_SMD:SC-70-8_Handsoldering" H 5000 3350 50  0001 C CNN
F 3 "" H 4800 3250 50  0001 C CNN
	1    4800 3250
	1    0    0    -1  
$EndComp
Wire Wire Line
	4400 3050 5400 3050
Wire Wire Line
	4900 3050 4900 2750
Connection ~ 4900 3050
Wire Wire Line
	4900 3450 5400 3450
Wire Wire Line
	4600 3250 4600 3650
$EndSCHEMATC
