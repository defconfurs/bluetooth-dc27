EESchema Schematic File Version 4
LIBS:beacon-pcb-cache
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
L KiCAD-Magic:BT832 IC1
U 1 1 5D229631
P 3250 3150
F 0 "IC1" H 3225 3915 50  0000 C CNN
F 1 "BT832" H 3225 3824 50  0000 C CNN
F 2 "KiCAD Magic:BT832" H 3250 3150 50  0001 C CNN
F 3 "" H 3250 3150 50  0001 C CNN
	1    3250 3150
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0101
U 1 1 5D22969F
P 3800 3700
F 0 "#PWR0101" H 3800 3450 50  0001 C CNN
F 1 "GND" H 3805 3527 50  0000 C CNN
F 2 "" H 3800 3700 50  0001 C CNN
F 3 "" H 3800 3700 50  0001 C CNN
	1    3800 3700
	1    0    0    -1  
$EndComp
Wire Wire Line
	3800 3700 3800 3650
Wire Wire Line
	3800 3650 3750 3650
Wire Wire Line
	3800 3500 3800 3550
Wire Wire Line
	3800 3550 3750 3550
$Comp
L Device:Battery BT1
U 1 1 5D229817
P 1900 3150
F 0 "BT1" H 2008 3196 50  0000 L CNN
F 1 "Battery" H 2008 3105 50  0000 L CNN
F 2 "KiCAD Magic:BAT-2xAA-BC12AAPC" V 1900 3210 50  0001 C CNN
F 3 "~" V 1900 3210 50  0001 C CNN
	1    1900 3150
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0102
U 1 1 5D2298B4
P 1900 3450
F 0 "#PWR0102" H 1900 3200 50  0001 C CNN
F 1 "GND" H 1905 3277 50  0000 C CNN
F 2 "" H 1900 3450 50  0001 C CNN
F 3 "" H 1900 3450 50  0001 C CNN
	1    1900 3450
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 3400 1900 3350
$Comp
L power:+BATT #PWR0103
U 1 1 5D229AC0
P 1900 2850
F 0 "#PWR0103" H 1900 2700 50  0001 C CNN
F 1 "+BATT" H 1915 3023 50  0000 C CNN
F 2 "" H 1900 2850 50  0001 C CNN
F 3 "" H 1900 2850 50  0001 C CNN
	1    1900 2850
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 2900 1900 2950
$Comp
L power:+BATT #PWR0104
U 1 1 5D229B0A
P 3800 3500
F 0 "#PWR0104" H 3800 3350 50  0001 C CNN
F 1 "+BATT" H 3815 3673 50  0000 C CNN
F 2 "" H 3800 3500 50  0001 C CNN
F 3 "" H 3800 3500 50  0001 C CNN
	1    3800 3500
	1    0    0    -1  
$EndComp
$Comp
L Device:C C1
U 1 1 5D229B5C
P 1650 3150
F 0 "C1" H 1535 3196 50  0000 R CNN
F 1 "4.7u" H 1535 3105 50  0000 R CNN
F 2 "Capacitor_SMD:C_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 1688 3000 50  0001 C CNN
F 3 "~" H 1650 3150 50  0001 C CNN
	1    1650 3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	1650 3000 1650 2900
Wire Wire Line
	1650 2900 1900 2900
Wire Wire Line
	1900 2850 1900 2900
Connection ~ 1900 2900
Wire Wire Line
	1650 3300 1650 3400
Wire Wire Line
	1650 3400 1900 3400
Wire Wire Line
	1900 3450 1900 3400
Connection ~ 1900 3400
$Comp
L Connector_Generic:Conn_02x03_Odd_Even J1
U 1 1 5D22A37B
P 4950 2750
F 0 "J1" H 5000 3067 50  0000 C CNN
F 1 "TC2030-CTX-NL" H 5150 3000 50  0000 C CNN
F 2 "Connector:Tag-Connect_TC2030-IDC-NL_2x03_P1.27mm_Vertical" H 4950 2750 50  0001 C CNN
F 3 "~" H 4950 2750 50  0001 C CNN
	1    4950 2750
	-1   0    0    -1  
$EndComp
Wire Wire Line
	4300 3050 4300 2850
$Comp
L power:GND #PWR0105
U 1 1 5D22A89C
P 5200 2900
F 0 "#PWR0105" H 5200 2650 50  0001 C CNN
F 1 "GND" H 5205 2727 50  0000 C CNN
F 2 "" H 5200 2900 50  0001 C CNN
F 3 "" H 5200 2900 50  0001 C CNN
	1    5200 2900
	1    0    0    -1  
$EndComp
Wire Wire Line
	5200 2900 5200 2850
Wire Wire Line
	5200 2850 5150 2850
$Comp
L power:+BATT #PWR0106
U 1 1 5D22AA29
P 5200 2600
F 0 "#PWR0106" H 5200 2450 50  0001 C CNN
F 1 "+BATT" H 5215 2773 50  0000 C CNN
F 2 "" H 5200 2600 50  0001 C CNN
F 3 "" H 5200 2600 50  0001 C CNN
	1    5200 2600
	1    0    0    -1  
$EndComp
Wire Wire Line
	5200 2600 5200 2650
Wire Wire Line
	5200 2650 5150 2650
Wire Wire Line
	5350 2750 5350 3150
Wire Wire Line
	5150 2750 5350 2750
Text Label 4300 3150 0    50   ~ 0
UART_TXD
Text Label 4300 2850 0    50   ~ 0
UART_RXD
Wire Wire Line
	4300 2850 4650 2850
Text Label 4300 2650 0    50   ~ 0
SWDIO
Text Label 4300 2750 0    50   ~ 0
SWCLK
$Comp
L Device:LED_ALT D1
U 1 1 5D2387D2
P 4150 3400
F 0 "D1" V 4188 3282 50  0000 R CNN
F 1 "LED_ALT" V 4097 3282 50  0000 R CNN
F 2 "LED_SMD:LED_0603_1608Metric_Pad1.05x0.95mm_HandSolder" H 4150 3400 50  0001 C CNN
F 3 "~" H 4150 3400 50  0001 C CNN
	1    4150 3400
	0    -1   -1   0   
$EndComp
Wire Wire Line
	3750 2650 4650 2650
Wire Wire Line
	3750 2750 4650 2750
Wire Wire Line
	3750 3050 4300 3050
Wire Wire Line
	3750 3150 5350 3150
Wire Wire Line
	4150 3550 4150 3650
Wire Wire Line
	4150 3650 3800 3650
Connection ~ 3800 3650
$Comp
L Device:R R1
U 1 1 5D239D31
P 3950 2850
F 0 "R1" V 4050 2850 50  0000 C CNN
F 1 "100" V 3950 2850 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric_Pad1.05x0.95mm_HandSolder" V 3880 2850 50  0001 C CNN
F 3 "~" H 3950 2850 50  0001 C CNN
	1    3950 2850
	0    -1   1    0   
$EndComp
Wire Wire Line
	3750 2850 3800 2850
Wire Wire Line
	4100 2850 4150 2850
Wire Wire Line
	4150 2850 4150 3250
$EndSCHEMATC
