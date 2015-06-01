EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
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
LIBS:kicadlib
LIBS:rgb_banner-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 3
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
L TST P1
U 1 1 556C686B
P 4350 3500
F 0 "P1" H 4350 3800 40  0000 C CNN
F 1 "+5V" H 4350 3750 30  0000 C CNN
F 2 "kicadlib:TEST_0.100" H 4350 3500 60  0001 C CNN
F 3 "" H 4350 3500 60  0000 C CNN
	1    4350 3500
	1    0    0    -1  
$EndComp
$Comp
L TST P2
U 1 1 556C69BB
P 4050 3650
F 0 "P2" H 4050 3950 40  0000 C CNN
F 1 "GND" H 4050 3900 30  0000 C CNN
F 2 "kicadlib:TEST_0.100" H 4050 3650 60  0001 C CNN
F 3 "" H 4050 3650 60  0000 C CNN
	1    4050 3650
	1    0    0    -1  
$EndComp
$Comp
L TST P3
U 1 1 556C69DB
P 3750 3800
F 0 "P3" H 3750 4100 40  0000 C CNN
F 1 "DIN" H 3750 4050 30  0000 C CNN
F 2 "kicadlib:TEST_0.100" H 3750 3800 60  0001 C CNN
F 3 "" H 3750 3800 60  0000 C CNN
	1    3750 3800
	1    0    0    -1  
$EndComp
$Comp
L TST P6
U 1 1 556C69FA
P 7050 3800
F 0 "P6" H 7050 4100 40  0000 C CNN
F 1 "DOUT" H 7050 4050 30  0000 C CNN
F 2 "kicadlib:TEST_0.100" H 7050 3800 60  0001 C CNN
F 3 "" H 7050 3800 60  0000 C CNN
	1    7050 3800
	1    0    0    -1  
$EndComp
Text Label 4300 3600 2    60   ~ 0
+5V
Text Label 3700 3900 2    60   ~ 0
d_in
Text Label 7100 3900 0    60   ~ 0
d_out
Wire Wire Line
	3750 3800 3750 3900
Wire Wire Line
	4050 3750 4050 3650
Wire Wire Line
	4350 3600 4350 3500
$Comp
L TST P4
U 1 1 556C71D9
P 6450 3500
F 0 "P4" H 6450 3800 40  0000 C CNN
F 1 "+5V" H 6450 3750 30  0000 C CNN
F 2 "kicadlib:TEST_0.100" H 6450 3500 60  0001 C CNN
F 3 "" H 6450 3500 60  0000 C CNN
	1    6450 3500
	1    0    0    -1  
$EndComp
$Comp
L TST P5
U 1 1 556C71DF
P 6750 3650
F 0 "P5" H 6750 3950 40  0000 C CNN
F 1 "GND" H 6750 3900 30  0000 C CNN
F 2 "kicadlib:TEST_0.100" H 6750 3650 60  0001 C CNN
F 3 "" H 6750 3650 60  0000 C CNN
	1    6750 3650
	1    0    0    -1  
$EndComp
Text Label 6500 3600 0    60   ~ 0
+5V
Wire Wire Line
	6450 3600 6450 3500
Text Label 4000 3750 2    60   ~ 0
gnd
$Sheet
S 5600 3500 750  1600
U 556C8A1A
F0 "digit2" 60
F1 "digit2.sch" 60
F2 "gnd" I R 6350 3750 60 
F3 "+5V" I R 6350 3600 60 
F4 "d_in_1" I L 5600 4050 60 
F5 "d_out_all" I R 6350 3900 60 
F6 "d_in_2" I L 5600 4350 60 
F7 "d_in_3" I L 5600 4650 60 
F8 "d_out_1" I L 5600 4200 60 
F9 "d_out_2" I L 5600 4500 60 
F10 "d_out_3" I L 5600 4800 60 
F11 "d_in_4" I L 5600 4950 60 
$EndSheet
Wire Wire Line
	3700 3900 4450 3900
Wire Wire Line
	4000 3750 4450 3750
Wire Wire Line
	4300 3600 4450 3600
Connection ~ 4350 3600
Connection ~ 4050 3750
Connection ~ 3750 3900
Text Label 6800 3750 0    60   ~ 0
gnd
Wire Wire Line
	6350 3600 6500 3600
Connection ~ 6450 3600
Wire Wire Line
	6350 3750 6800 3750
Wire Wire Line
	6750 3750 6750 3650
Wire Wire Line
	6350 3900 7100 3900
Wire Wire Line
	7050 3900 7050 3800
Connection ~ 6750 3750
Connection ~ 7050 3900
$Sheet
S 4450 3500 750  1600
U 556C8146
F0 "digit1" 60
F1 "digit1.sch" 60
F2 "gnd" I L 4450 3750 60 
F3 "+5V" I L 4450 3600 60 
F4 "d_in_all" I L 4450 3900 60 
F5 "d_out_3" I R 5200 4650 60 
F6 "d_out_4" I R 5200 4950 60 
F7 "d_in_3" I R 5200 4800 60 
F8 "d_in_2" I R 5200 4500 60 
F9 "d_out_2" I R 5200 4350 60 
F10 "d_in_1" I R 5200 4200 60 
F11 "d_out_1" I R 5200 4050 60 
$EndSheet
Wire Wire Line
	5200 4050 5600 4050
Wire Wire Line
	5600 4200 5200 4200
Wire Wire Line
	5200 4350 5600 4350
Wire Wire Line
	5600 4500 5200 4500
Wire Wire Line
	5200 4650 5600 4650
Wire Wire Line
	5600 4800 5200 4800
Wire Wire Line
	5200 4950 5600 4950
$EndSCHEMATC
