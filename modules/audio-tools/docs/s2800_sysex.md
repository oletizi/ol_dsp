Note: this is originally from: https://lakai.sourceforge.net/docs/s2000_sysex.html

S2800/S3000/S3200 MIDI System Exclusive Extensions
==================================================

Philosophy
----------

The philosophy adopted when providing MIDI system exclusive support for the S3000 has been one of providing remote control. Ideally, anything that can be done from the S3000 front panel should be able to be done by means of such MIDI system exclusive operations, although in practice, some functions (e.g. templates, etc.) are rather unnecessary. Of the new functions provided, the majority fall within two classes: access to data structures (program/keygroup/sample header, effects, cue-lists, disk directories, etc). and access to miscellaneous variables and functions. In general, the first class concern data that can be stored on disk and the second class refer to operation on the S3000 specifically to provide a greater functionality via MIDI system exclusive, unlike the S1000 which, for example, supports the transfer of sample data via MIDI system exclusive. However, it is intended that the S3000 will support the majority of such S1000/S1100 MIDI system exclusive operations. There are no functions within MIDI system exclusive to provide direct access to and from disk files. Directories and files can be loaded into the S3000 and the data then accessed. However, if external parties wish to get data directly from disk, it is available via SCSI.

Implementation
--------------

The S3000 bears much similarity to the S1000/S1100. Most S1000 MIDI exclusive functions are supported. Additional functions have been added. These new functions are not available on the S1000.

Program, keygroup and Sample headers have been extended to 192 bytes and the system has capacity for 1022 such headers in total. Wherever possible, data in the headers has retained its meaning. However, many S1000 parameters have become obsolete and many have been added to extend the functinality of S1000 parameters. See "Assignable Program Modulation", below.

Assignable Program Modulation is an extension of performance control. Many users will be familiar with the concept of a pitch-bend control on a keyboard affecting the pitch of a remote sound module, and similarly, the modulation wheel controlling the depth of an LFO which is itself altering the pitch. In the S3000, the link between the controller (pitch.bend, wheel, modwheel, aftertouch etc.) and the controlled characteristic (pitch, pan, level, filter frequency etc.) has been made flexible. Consequently, in each program header, for each parameter subject to indicate the source of control (mods####parameters).

### S2800

The S2800 bears much similarity to the S3000. There are only two individual outputs rather than the eight on the S3000.

### S3200

The S3200 bears much similarity to the S3000. There is a second LSI which provides a multimode filter, spectral tilt tone control and dedicated reverb. In addition to the 50 effects "setups", there are 50 "reverb" setups. The direct-to-disk recording facility of the S1100 has been provided and consequently there exists a list of TAKEs to support this.

S3000 family MIDI System Exclusive Extensions
---------------------------------------------

Generally, messages have three portions:

a) Header. This is 12 bytes long.

b) Data. This is sent in a nibbled form, i.e. each data byte is represented by two messages bytes; the bottom nibble of the first containing the bottom 4-bits of the data byte and the bottom nibble of the second containing the top 4-bits of the data byte.

c) End-of-Exclusive (EOX). A single byte, value 0xF7.

Any message which is a request for data will not have a data portion, i.e. it will be 13 bytes long (12-byte header + EOX).

### Structure of Message Header

With the exception of the first and last bytes, all bytes are in the range 0 to 0x7f. Some parameters in the header require more than 7 bits and in these cases two successive message bytes are combined to form a 14-bit quantity.

Byte 0: 0xF0 MIDI System Exclusive Identifier  
Byte 1: 0x47 Akai Manufacturer code  
Byte 2: ? MIDI Exclusive channel  
Byte 3: ? S3000 MIDI System Exclusive function code.  
Function codes that are specific to the S3000 start at 0x20.  
Byte 4: 0x48 S3000 Model identity.  
The S3000 shares the same model as the S1000.  
Bytes 5 &6: ?,? Item index  
The 14 data bits contained in these two bytes select a data item. The type of item selected depends upon the function code, e.g. these two bytes may indicate program number, sample number etc.  
Additionally, the two uppermost bits (bits 6 and 5 of byte 6) have meaning when write operations are being performed (see below).  
Byte 7: ? Selector  
This byte holds additional selection data, e.g. keygroup number, type of miscellaneous data.  
Bytes 8 &9: ?, ? Byte offset into data item  
Bytes 10 &11: ?, ? Number of bytes of data

**Note: Post-change functions**  
After certain parameters have been altered, it may be necessary to run certain internal functions. Also the screen of the S3000 may need to be refreshed. In most cases, this will happen automatically. However, these operations can be postponed until a later time by use of the Item Index. Setting these bits will have the following effect:  
Bit 13 = 1: Postpone screen update  
Bit 12=1: Postpone recalculation program. Note that the machine may be in an undetermined state until the same parameter is sent with this bit cleared.

Currently, one exception to the automated fixing-up process is after renumbering programs. Since the renumber operation may alter the order of the list of programs, no resorting is done until the command to explicitly run the Miscellaneous function BTSORT is sent.

### Operation codes

0x27 Request for Program Header bytes  
0x28 Program Header bytes  
0x29 Request for Keygroup Header bytes  
0x2A Keygroup Header bytes  
0x2B Request for Sample Header bytes  
0x2C Sample Header bytes  
0x2D Request for FX/Reverb bytes  
0x2E FX/Reverb bytes  
0x2F Request for Cue-List bytes  
0x30 Cue-List bytes  
0x31 Request for Take List bytes  
0x32 Take List bytes  
0x33 Request for Miscellaneous bytes  
0x34 Miscellaneous bytes  
0x35 Request Volume List item  
0x36 Volume List item (only used in response to request)  
0x37 Request Harddisk Directory entry  
0x38 Harddisk Directory entry (only used in response to request)

### Request for Program Header bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x27 Operation code  
0x48 S1000 Model identity  
pp,PP Program number  
0x00 Reserved  
oo,oo Byte offset into header  
nn,nn Number of bytes of data  
0xF7 End of Exclusive message

### Receive Program Header bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x28 Operation code  
0x48 S1000 Model identity  
pp,PP Program number  
0x00 Reserved  
oo,oo Byte offset into header  
nn,nn Number of bytes of data  
ln,hn First byte (nibbled)  
..,.. further data  
0xF7 End of Exclusive message

### Request for Keygroup Header bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x29 Operation code  
0x48 S1000 Model identity  
pp,PP Program number  
kk Keygroup  
oo,oo Byte offset into header  
nn,nn Number of bytes of data  
0xF7 End of Exclusive message

### Receive Keygroup Header bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x2A Operation code  
0x48 S1000 Model identity  
pp,PP Program number  
kk Keygroup (0x7f indicates all keygroups in program)  
oo,oo Byte offset into header  
nn,nn Number of bytes of data  
ln,hn First byte (nibbled)  
..,.. further data  
0xF7 End of Exclusive message

### Request for Sample Header bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x2B Operation code  
0x48 S1000 Model identity  
ss,ss Sample number  
0x00 Reserved  
oo,oo Byte offset into header  
nn,nn Number of bytes of data ( ??? typo in specs..? - FN)  
ln,hn First byte (nibbled) (??? same as above..? - FN)  
..,.. Further data  
0xF7 End of Exclusive message

### Receive Sample Header bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x2C Operation code  
0x48 S1000 Model identity  
ss,ss Sample number  
0x00 Reserved  
oo,oo Byte offset into header  
nn,nn Number of bytes of data  
ln,hn First byte (nibbled)  
..,.. Further data  
0xF7 End of Exclusive message

### Request for FX/Reverb bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x2D Operation code  
0x48 S1000 Model identity  
ff,ff Effect number  
bb Selector (0=fx header, 1=fx assign, 2=fx entry, 3=rvb assign, 4=rvb entry)  
oo,oo Byte offset into header  
nn,nn Number of bytes of data  
0xF7 End of Exclusive message

### Received FX/Reverb bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x2E Operation code  
0x48 S1000 Model identity  
ff,ff Effect number  
bb Selector (0=fx header, 1=fx assign, 2=fx entry, 3=rvb assign, 4=rvb entry)  
oo,oo Byte offset into header  
nn,nn Number of bytes of data  
ln,hn First byte (nibbled)  
..,.. Further data  
0xF7 End of Exclusive message

### Request for Cue-List bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x2F Operation code  
0x48 S1000 Model identity  
ff,ff Event number  
bb Selector (0=header, 1=cue event)  
oo,oo Byte offset into structure  
nn,nn Number of bytes of data  
0xF7 End of Exclusive message

### Receive Cue-List bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x30 Operation code  
0x48 S1000 Model identity  
ff,ff Event number  
bb Selector (0=header, 1=cue event)  
oo,oo Byte offset into structure  
nn,nn Number of bytes of data  
ln,hn First byte (nibbled) (this is not in the orig docs - typo? - FN)  
..,.. Further data (see above - FN)  
0xF7 End of Exclusive message

### Request for Take List bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x31 Operation code  
0x48 S1000 Model identity  
ff,ff Take number  
bb Selector (0=header, 1=take)  
oo,oo Byte offset into structure  
nn,nn Number of bytes of data  
0xF7 End of Exclusive message

### Receive Take List bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x32 Operation code  
0x48 S1000 Model identity  
ff,ff Take number  
bb Selector (0=header, 1=take)  
oo,oo Byte offset into structure  
nn,nn Number of bytes of data  
ln,hn First byte (nibbled) (this is not in the orig docs - typo? - FN)  
..,.. Further data (see above - FN)  
0xF7 End of Exclusive message

### Request for Miscellaneous bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x33 Operation code  
0x48 S1000 Model identity  
dd,dd Data Index  
bb Data bank number (1=byte, 2=word, 3=dword, 4=smpte, 5=signed smpte, 6=name, 7=16byteflag)  
0,0 Reserved  
nn,nn Number of bytes of data (1/2/4/5/6/12/16)  
0xF7 End of Exclusive message

### Receive Miscellaneous bytes

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x34 Operation code  
0x48 S1000 Model identity  
dd,dd Data Index  
bb Data ban knumber (1=byte, 2=word, 3=dword, 4=smpte, 5=signed smpte, 6=name, 7=16byteflag)  
0,0 Reserved  
nn,nn Number of bytes of data  
ln,hn First byte (nibbled)  
..,.. Further data  
0xF7 End of Exclusive message

### Request for Volume List entry

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x35 Operation code  
0x48 S1000 Model identity  
ff,ff Entry number  
0 Reserved  
oo,oo Byte offset into structure  
nn,nn Number of bytes of data (1/2/4/5/6/12/16)  
0xF7 End of Exclusive message

### Transmitted Volume List entry

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x36 Operation code  
0x48 S1000 Model identity  
ff,ff Entry number  
0 Reserved  
oo,oo Byte offset into structure  
nn,nn Number of bytes of data  
ln,hn First byte (nibbled)  
..,.. Further data  
0xF7 End of Exclusive message

### Request Harddisk Directory entry

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x37 Operation code  
0x48 S1000 Model identity  
nn,nn Directory entry (0-509)/Item number  
ss Selector (0=volume data, 1=program, 2=sample, 3=cue list, 4=take list, 5=effects file, 6=drum file)  
0x00, 0x00 Reserved  
nn,nn Number of bytes of data(24)  
0xF7 End of Exclusive message

### Transmitted Harddisk Directory entry

0xF0 MIDI System Exclusive identifier  
0x47 Akai Manufacturer code  
cc MIDI Exclusive channel  
0x38 Operation code  
0x48 S1000 Model identity  
nn,nn Directory entry (0-509)/Item number  
ss Selector (0=volume data, 1=program, 2=sample, 3=cue list, 4=take list, 5=effects file, 6=drum file)  
0x00, 0x00 Reserved  
nn,nn Number of bytes of data(24)  
ln,hn First byte (nibbled)  
..,.. Further data  
0xF7 End of Exclusive message

### Accessing Program Header bytes

To associate numbers with program names, it is advised that the S1000 RPLIST (request list of resident program names) command be used. The machine holds sequential numbers, starting at zero for items in this list and these numbers should be used to identify a specific program.

### Structure of S3000 Program Header

Parameter: KGRP1@  
Offset: 1 byte  
Field size: 2 bytes  
Range: Block address  
Description: Block address of first keygroup (internal use)

Parameter: PRNAME  
Offset: 3 bytes  
Field size: 12 bytes  
Range: String of characters  
Description: Name of program

Parameter:PRGNUM  
Offset: 15 bytes  
Field size: 1 byte  
Range: 0 to 128  
Description: MIDI program number  
After sending data to this parameter, Miscellaneous function BTSORT should be triggered to resort the list of programs into order and to flag active programs.

Parameter: PMCHAN  
Offset: 16 bytes  
Field size: 1 byte  
Range: 255 signifies OMNI, 0 to 15 indicate MIDI channel  
Description: MIDI channel

Parameter: POLYPH  
Offset: 17 bytes  
Field size: 1 byte  
Range: 0 to 31 (these represent polyphony values of 1 to 32)  
Description: Depth of polyphony

Parameter: PRIORT  
Offset: 18 bytes  
Field size: 1 byte  
Range: 0=low, 1=norm, 2=high, 3=hold  
Description: Priority of voices playing this program

Parameter: PLAYLO  
Offset: 19 bytes  
Field size: 1 byte  
Range: 21 to 127 represents A1 to G8  
Description: Lower limit of play range

Parameter: PLAYHI  
Offset: 20 bytes  
Field size: 1 byte  
Range: 21 to 127 represents A1 to G8  
Description: Upper limit of play range

Parameter: OSHIFT  
Offset: 21 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: OUTPUT  
Offset: 22 bytes  
Field size: 1 byte  
Range: 255 indicates OFF  
On S3200: 0 to 7 indicates outputs 1 to 8, 8 indicates FX, 9 indicates RVB and 10 indicates R+F.  
On S3000: 0 to 7 indicates outputs 1 to 8, 8 indicates FX.  
On S2800: 0 and 1 indicates outputs 1 and 2, 2 indicates FX.  
Description: Individual output routing. This parameter also controls send to effects section.

Parameter: STEREO  
Offset: 23 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Left and right output levels

Parameter: PANPOS  
Offset: 24 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Balance between left and right outputs

Parameter: PRLOUD  
Offset: 25 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Basic loudness of this program

Parameter: V\_LOUD  
Offset: 26 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Note-on velocity dependence of loudness

Parameter: K\_LOUD  
Offset: 27 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: P\_LOUD  
Offset: 28 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: PANRAT  
Offset: 29 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Speed of LFO2

Parameter: PANDEP  
Offset: 30 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Depth of LFO2

Parameter: PANDEL  
Offset: 31 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Delay in growth of LFO2

Parameter: K\_PANP  
Offset: 32 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: LFORAT  
Offset: 33 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Speed of LFO1

Parameter: LFODEP  
Offset: 34 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Depth of LFO1

Parameter: LFODEL  
Offset: 35 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Delay in growth of LFO1

Parameter: MWLDEP  
Offset: 36 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Amount of control of LFO1 depth by Modwheel

Parameter: PRSDEP  
Offset: 37 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Amount of control of LFO1 depth by Aftertouch

Parameter: VELDEP  
Offset: 38 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Amount of control of LFO1 depth by Note-On velocity

Parameter: B\_PTCH  
Offset: 39 bytes  
Field size: 1 byte  
Range: 0 to 24 semitones  
Description: Range of increase of Pitch by bendwheel

Parameter: P\_PTCH  
Offset: 40 bytes  
Field size: 1 byte  
Range: -12 to +12 semitones  
Description: Amount of control of Pitch by Pressure

Parameter: KXFADE  
Offset: 41 bytes  
Field size: 1 byte  
Range: 0 represents OFF, 1 represents ON  
Description: Keygroup crossfade enable

Parameter: GROUPS  
Offset: 42 bytes  
Field size: 1 byte  
Range: 1 to 99 (Read-only)  
Description: Number of keygroups. To change the number of keygroups in a program, the KDATA and DELK commands should be used.

Parameter: TPNUM  
Offset: 43 bytes  
Field size: 1 byte  
Range: 0 to 127  
Description: Temporary program number (internal use)

Parameter: TEMPER  
Offset: 44 bytes  
Field size: 12 bytes  
Range: -50 to +50 cents  
Description: Key temperament C, C#, D, D# etc.

Parameter: ECHOUT  
Offset: 56 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: MW\_PAN  
Offset: 57 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: COHERE  
Offset: 58 bytes  
Field size: 1 byte  
Range: 1  
Description: Not used

Parameter: DESYNC  
Offset: 59 bytes  
Field size: 1 byte  
Range: 0 represents OFF, 1 represents ON  
Description: Enable de-synchronisation of LFO1 across notes

Parameter: PLAW  
Offset: 60 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: VASSOQ  
Offset: 61 bytes  
Field size: 1 byte  
Range: 0 represents OLDEST, 1 represents QUIETEST  
Description: Criterion by which voices are stolen

Parameter: SPLOUD  
Offset: 62 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Reduction in loudness due to soft pedal

Parameter: SPATT  
Offset: 63 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Stretch of attack due to soft pedal

Parameter: SPFILT  
Offset: 64 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Reduction of filter frequency due to soft pedal

Parameter: PTUNO  
Offset: 65 bytes  
Field size: 2 bytes  
Range: -50.00 to +50.00 (fraction is binary)  
Description: Tuning offset of program

Parameter: K\_LRAT  
Offset: 67 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: K\_LDEP  
Offset: 68 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: K\_LDEL  
Offset: 69 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: VOSCL  
Offset: 70 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Level sent to Individual outputs/effects

Parameter: VSSCL  
Offset: 71 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: LEGATO  
Offset: 72 bytes  
Field size: 1 byte  
Range: 0 represents OFF, 1 represents ON  
Description: Mono legato mode enable

Parameter: B\_PTCHD  
Offset: 73 bytes  
Field size: 1 byte  
Range: 0 to 12 semitones  
Description: Range of decrease of Pitch by bendwheel

Parameter: B\_MODE  
Offset: 74 bytes  
Field size: 1 byte  
Range: 0 represents NORMAL mode, 1 represents HELD mode  
Description: Bending of held notes

Parameter: TRANSPOSE  
Offset: 75 bytes  
Field size: 1 byte  
Range: -50 to +50 semitones  
Description: Shift pitch of incoming MIDI

**Values used to represent Modulation Sources**

0: No Source  
1: Modwheel  
2: Bend  
3: Pressure  
4: External  
5: Note-on velocity  
6: Key  
7: LFO1  
8: LFO2  
9: Env1  
10: Env2  
11: !Modwheel (Instantaneous value of modwheel at note-on)  
12: !Bend (Instantaneous value of bendwheel at note-on)  
13: !External (Instantaneous value of MIDI controller at note-on)  
14: Env3

Parameter: MODSPAN1  
Offset: 76 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: First source of assignable modulation of pan position

Parameter: MODSPAN2  
Offset: 77 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: Second source of assignable modulation of pan

Parameter: MODSPAN3  
Offset: 78 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: Third source of assignable modulation of pan

Parameter: MODSAMP1  
Offset: 79 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: First source of assignable modulation of loudness

Parameter: MODSAMP2  
Offset: 80 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: Second source of assignable modulation of loudness

Parameter: MODSLFOT  
Offset: 81 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: Source of assignable modulation of LFO1 speed

Parameter: MODSLFOL  
Offset: 82 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: Source of assignable modulation of LFO1 depth

Parameter: MODSLFOD  
Offset: 83 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: Source of assignable modulation of LFO1 delay

Parameter: MODSFILT1  
Offset: 84 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: First source of assignable modulation of filter frequency

Parameter: MODSFILT2  
Offset: 85 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: Second source of assignable modulation of filter frequency

Parameter: MODSFILT3  
Offset: 86 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: Third source of assignable modulation of filter frequency

Parameter: MODSPITCH  
Offset: 87 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: Source of assignable modulation of pitch

Parameter: MODSAMP3  
Offset: 88 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation Sources" above  
Description: Third source of assignable modulation of loudness

Parameter: MODVPAN1  
Offset: 89 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of pan by assignable source 1

Parameter: MODVPAN2  
Offset: 90 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of pan by assignable source 2

Parameter: MODVPAN3  
Offset: 91 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of pan by assignable source 3

Parameter: MODVAMP1  
Offset: 92 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of loudness by assignable source 1

Parameter: MODVAMP2  
Offset: 93 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of loudness by assignable source 2

Parameter: MODVLFOR  
Offset: 94 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of LFO1 speed

Parameter: MODVLVOL  
Offset: 95 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of LFO1 depth

Parameter: MODVLFOD  
Offset: 96 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of LFO1 delay

Parameter: LFO1WAVE  
Offset: 97 bytes  
Field size: 1 byte  
Range: 0 represents Triangle, 1 represents Sawtooth, 2 represents Square  
Description: LFO1 waveform

Parameter: LFO2WAVE  
Offset: 98 bytes  
Field size: 1 byte  
Range: 0 represents Triangle, 1 represents Sawtooth, 2 represents Square  
Description: LFO2 waveform

Parameter: MODSLFLT2\_1  
Offset: 99 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation sources" above  
Description: First source of assignable modulation of filter 2 frequency (only used on S3200).

Parameter: MODSLFLT2\_2  
Offset: 100 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation sources" above  
Description: Second source of assignable modulation of filter 2 frequency (only used on S3200).

Parameter: MODSLFLT2\_3  
Offset: 101 bytes  
Field size: 1 byte  
Range: See "Values used to represent Modulation sources" above  
Description: Third source of assignable modulation of filter 2 frequency (only used on S3200).

Parameter: lfo2trig  
Offset: 102 bytes  
Field size: 1 byte  
Range:  
Description: Retrigger mode for LFO2

Parameter: Reserved  
Offset: 103 bytes  
Field size: 7 bytes  
Range:  
Description: Not used

Parameter: PORTIME  
Offset: 110 bytes  
Field size: 1 byte  
Range:  
Description: PORTAMENTO TIME

Parameter: PORTYPE  
Offset: 111 bytes  
Field size: 1 byte  
Range:  
Description: PORTAMENTO TYPE

Parameter: PORTEN  
Offset: 112 bytes  
Field size: 1 byte  
Range:  
Description: PORTAMENTO ON/OFF

**S2000/S3000XL/S3200XL Parameters**

Parameter: PFXCHAN  
Offset: 113 bytes  
Field size: 1 byte  
Range: 0 to 4  
Description: Effects Bus Select  
0 = OFF  
1 = FX1  
2 = FX2  
3 = RV3  
4 = RV4

Parameter: PFXSLEV  
Offset: 114 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Not used

### Accessing Keygroup Header bytes

Each program will contain at least one keygroup. The actual number of keygroups contained in a program is given by the GROUPS parameter of the program header. Keygroups do not have names; they are identified by numbers in the range 0 to (GROUPS-1).

Parameter: KGIDENT  
Offset: 0 bytes  
Field size: 1 byte  
Range: 2  
Description: Block identifier (internal use)

Parameter: NXTKG@  
Offset: 1 bytes  
Field size: 2 bytes  
Range: Block address  
Description: Next keygroup block address (internal use)

Parameter: LONOTE  
Offset: 3 bytes  
Field size: 1 byte  
Range: 21 to 127 represents A1 to G8  
Description: Lower limit of keyrange

Parameter: HINOTE  
Offset:4 bytes  
Field size: 1 byte  
Range: 21 to 127 represents A1 to G8  
Description: Upper limit of keyrange

Parameter: KGTUNO  
Offset: 5 bytes  
Field size: 2 bytes  
Range: -50.00 to +50.00 (fraction is binary)  
Description: Keygroup tuning offset

Parameter: FILFRQ  
Offset: 7 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Basic filter frequency

Parameter: K\_FREQ  
Offset: 8 bytes  
Field size: 1 byte  
Range: 0 to 12 semitones  
Description: Key follow of filter frequency

Parameter: V\_FREQ  
Offset: 9 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: P\_FREQ  
Offset: 10 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: E\_FREQ  
Offset: 11 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: ATTAK1  
Offset: 12 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Attack rate of envelope 1

Parameter:DECAY1  
Offset: 13 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Decay rate of envelope 1

Parameter: SUSTN1  
Offset: 14 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Sustain level of envelope 1

Parameter: RELSE1  
Offset: 15 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Release rate of envelope 1

Parameter: V\_ATT1  
Offset: 16 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Note-on velocity dependence of envelope 1 attack rate

Parameter: V\_REL1  
Offset: 17 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Note-on velocity dependence of envelope 1 release rate

Parameter: O\_REL1  
Offset: 18 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Note-off velocity dependence of envelope 1 release rate

Parameter: K\_DAR1  
Offset: 19 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Dependence of envelope 2 decay and release rates on key

Parameter: ATTAK2 or ENV2R1  
Offset: 20 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Attack rate of envelope 2

Parameter: DECAY2 or ENV2R3  
Offset: 21 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Decay rate of envelope 2

Parameter: SUSTN2 or ENV2L3  
Offset: 22 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Sustain level of envelope 2

Parameter: RELSE2 or ENV2R4  
Offset: 23 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Release rate of envelope 2

Parameter: V\_ATT2  
Offset: 24 bytes  
Field size: 1 byte  
Range: -50 to 50  
Description: Dependence of envelope 2 attack on note-on velocity

Parameter: V\_REL2  
Offset: 25 bytes  
Field size: 1 byte  
Range: -50 to 50  
Description: Dependence of envelope 2 release on note-on velocity

Parameter: O\_REL2  
Offset: 26 bytes  
Field size: 1 byte  
Range: -50 to 50  
Description: Dependence of envelope 2 release on note-off velocity

Parameter: K\_DAR2  
Offset: 27 bytes  
Field size: 1 byte  
Range: -50 to 50  
Description: Dependence of envelope 2 decay and release rates on key

Parameter: V\_ENV2  
Offset: 28 bytes  
Field size: 1 byte  
Range: -50 to 50  
Description: Scaling of envelope 2 by note-on velocity

Parameter: E\_PTCH  
Offset: 29 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: VXFADE  
Offset: 30 bytes  
Field size: 1 byte  
Range: 0 represents OFF, 1 represents ON  
Description: Velocity zone crossfade

Parameter: VZONES  
Offset: 31 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: LKXF  
Offset: 32 bytes  
Field size: 1 byte  
Range: 0 to 255  
Description: Calculated left keygroup crossfade factor (internal)

Parameter: RKXF  
Offset: 33 bytes  
Field size: 1 byte  
Range: 0 to 255  
Description: Calculated right keygroup crossfade factor (internal)

**Velocity zone 1**

Parameter: SNAME1  
Offset: 34 bytes  
Field size: 12 bytes  
Range: String of characters  
Description: Sample name used in velocity zone 1

Parameter: LOVEL1  
Offset: 46 bytes  
Field size: 1 byte  
Range: 0 to 127  
Description: Lower limit of velocity range

Parameter: HIVEL1  
Offset: 47 bytes  
Field size: 1 byte  
Range: 0 to 127  
Description: Upper limit of velocity range

Parameter: VTUNO1  
Offset: 48 bytes  
Field size: 2 bytes  
Range: -50.00 to +50.00 (fraction is binary)  
Description: Velocity zone 1 tuning offset

Parameter: VLOUD1  
Offset: 50 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 1 loudness offset

Parameter: VFREQ1  
Offset: 51 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 1 filter frequency offset

Parameter: VPANO1  
Offset: 52 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 1 pan offset

Parameter: ZPLAY1  
Offset: 53 bytes  
Field size: 1 byte  
Range: 0 = As sample  
1 = Loop in release  
2 = Loop til release  
3 = No loops  
4 = Play to sample end  
Description: Type of sample playback in velocity zone 1

Parameter: LVXF1  
Offset: 54 bytes  
Field size: 1 byte  
Range:  
Description: Low velocity crossfade factor (internal use)

Parameter: HVXF1  
Offset: 55 bytes  
Field size: 1 byte  
Range:  
Description: High velocity crossfade factor (internal use)

Parameter: SBADD1  
Offset: 56 bytes  
Field size: 2 bytes  
Range: Block address  
Description: Calculated sample header block address (internal)

**;Velocity zone 2**

Parameter: SNAME2  
Offset: 58 bytes  
Field size: 12 bytes  
Range: String of characters  
Description: Sample name used in velocity zone 2

Parameter: LOVEL2  
Offset: 70 bytes  
Field size: 1 byte  
Range: 0 to 127  
Description: Lower limit of velocity range 2

Parameter: HIVEL2  
Offset: 71 bytes  
Field size: 1 byte  
Range: 0 to 127  
Description: Upper limit of velocity range 2

Parameter: VTUNO2  
Offset: 72 bytes  
Field size: 2 bytes  
Range: -50.00 to +50.00 (fraction is binary)  
Description: Velocity zone 2 tuning offset

Parameter: VLOUD2  
Offset: 74 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 2 loudness offset

Parameter: VFREQ2  
Offset: 75 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 2 filter frequency offset

Parameter: VPANO2  
Offset: 76 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 2 pan offset

Parameter: ZPLAY2  
Offset: 77 bytes  
Field size: 1 byte  
Range: 0 = As sample  
1 = Loop in release  
2 = Loop til release  
3 = No loops  
4 = Play to sample end  
Description: Type of sample playback in velocity zone 2

Parameter: LVXF2  
Offset: 78 bytes  
Field size: 1 byte  
Range:  
Description: Low velocity crossfade factor (internal use)

Parameter: HVXF2  
Offset: 79 bytes  
Field size: 1 byte  
Range:  
Description: High velocity crossfade factor (internal use)

Parameter: SBADD2  
Offset: 80 bytes  
Field size: 2 bytes  
Range: Block address  
Description: Calculated sample header block address (internal)

**;Velocity zone 3**

Parameter: SNAME3  
Offset: 82 bytes  
Field size: 12 bytes  
Range: String of characters  
Description: Sample name used in velocity zone 3

Parameter: LOVEL3  
Offset: 94 bytes  
Field size: 1 byte  
Range: 0 to 127  
Description: Lower limit of velocity range 3

Parameter: HIVEL3  
Offset: 95 bytes  
Field size: 1 byte  
Range: 0 to 127  
Description: Upper limit of velocity range 3

Parameter: VTUNO3  
Offset: 96 bytes  
Field size: 2 bytes  
Range: -50.00 to +50.00 (fraction is binary)  
Description: Velocity zone 3 tuning offset

Parameter: VLOUD3  
Offset: 98 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 3 loudness offset

Parameter: VFREQ3  
Offset: 99 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 3 filter frequency offset

Parameter: VPANO3  
Offset: 100 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 3 pan offset

Parameter: ZPLAY3  
Offset: 101 bytes  
Field size: 1 byte  
Range: 0 = As sample  
1 = Loop in release  
2 = Loop til release  
3 = No loops  
4 = Play to sample end  
Description: Type of sample playback in velocity zone 3

Parameter: LVXF3  
Offset: 102 bytes  
Field size: 1 byte  
Range:  
Description: Low velocity crossfade factor (internal use)

Parameter: HVXF3  
Offset: 103 bytes  
Field size: 1 byte  
Range:  
Description: High velocity crossfade factor (internal use)

Parameter: SBADD3  
Offset: 104 bytes  
Field size: 2 bytes  
Range: Block address  
Description: Calculated sample header block address (internal)

**;Velocity zone 4**

Parameter: SNAME4  
Offset: 106 bytes  
Field size: 12 bytes  
Range: String of characters  
Description: Sample name used in velocity zone 4

Parameter: LOVEL4  
Offset: 118 bytes  
Field size: 1 byte  
Range: 0 to 127  
Description: Lower limit of velocity range 4

Parameter: HIVEL4  
Offset: 119 bytes  
Field size: 1 byte  
Range: 0 to 127  
Description: Upper limit of velocity range 4

Parameter: VTUNO4  
Offset: 120 bytes  
Field size: 2 bytes  
Range: -50.00 to +50.00 (fraction is binary)  
Description: Velocity zone 4 tuning offset

Parameter: VLOUD4  
Offset: 122 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 4 loudness offset

Parameter: VFREQ4  
Offset: 123 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 4 filter frequency offset

Parameter: VPANO4  
Offset: 124 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Velocity zone 4 pan offset

Parameter: ZPLAY4  
Offset: 125 bytes  
Field size: 1 byte  
Range: 0 = As sample  
1 = Loop in release  
2 = Loop til release  
3 = No loops  
4 = Play to sample end  
Description: Type of sample playback in velocity zone 4

Parameter: LVXF4  
Offset: 126 bytes  
Field size: 1 byte  
Range:  
Description: Low velocity crossfade factor (internal use)

Parameter: HVXF4  
Offset: 127 bytes  
Field size: 1 byte  
Range:  
Description: High velocity crossfade factor (internal use)

Parameter: SBADD4  
Offset: 128 bytes  
Field size: 2 bytes  
Range: Block address  
Description: Calculated sample header block address (internal)

**;Keygroup common**

Parameter: KBEAT  
Offset: 130 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Fixed rate detune

Parameter: AHOLD  
Offset: 131 bytes  
Field size: 1 byte  
Range: 0 represents OFF, 1 represents ON  
Description: Remain in attack phase until first loop encountered

**;More Zone stuff**

Parameter: CP1  
Offset: 132 bytes  
Field size: 1 byte  
Range: 0 represents TRACK, 1 represents CONST  
Description: Constant pitch flag for velocity zone 1

Parameter: CP2  
Offset: 133 bytes  
Field size: 1 byte  
Range: 0 represents TRACK, 1 represents CONST  
Description: Constant pitch flag for velocity zone 2

Parameter: CP3  
Offset: 134 bytes  
Field size: 1 byte  
Range: 0 represents TRACK, 1 represents CONST  
Description: Constant pitch flag for velocity zone 3

Parameter: CP4  
Offset: 135 bytes  
Field size: 1 byte  
Range: 0 represents TRACK, 1 represents CONST  
Description: Constant pitch flag for velocity zone 4

Parameter: VZOUT1  
Offset: 136 bytes  
Field size: 1 byte  
Range: 0 to 10 for S3000, S3200; 0 to 4 for S2800  
Description: Individual output offset for velocity zone 1

Parameter: VZOUT2  
Offset: 137 bytes  
Field size: 1 byte  
Range: 0 to 10 for S3000, S3200; 0 to 4 for S2800  
Description: Individual output offset for velocity zone 2

Parameter: VZOUT3  
Offset: 138 bytes  
Field size: 1 byte  
Range: 0 to 10 for S3000, S3200; 0 to 4 for S2800  
Description: Individual output offset for velocity zone 3

Parameter: VZOUT4  
Offset: 139 bytes  
Field size: 1 byte  
Range: 0 to 10 for S3000, S3200; 0 to 4 for S2800  
Description: Individual output offset for velocity zone 4

Parameter: VSS1  
Offset: 140 bytes  
Field size: 2 bytes  
Range: -9999 to +9999 data points  
Description: Start point dependence on note-on velocity for sample in velocity zone 1

Parameter: VSS2  
Offset: 142 bytes  
Field size: 2 bytes  
Range: -9999 to +9999 data points  
Description: Start point dependence on note-on velocity for sample in velocity zone 2

Parameter: VSS3  
Offset: 144 bytes  
Field size: 2 bytes  
Range: -9999 to +9999 data points  
Description: Start point dependence on note-on velocity for sample in velocity zone 3

Parameter: VSS4  
Offset: 146 bytes  
Field size: 2 bytes  
Range: -9999 to +9999 data points  
Description: Start point dependence on note-on velocity for sample in velocity zone 4

Parameter: KV\_LO  
Offset: 148 bytes  
Field size: 1 byte  
Range: 0  
Description: Not used

Parameter: FILQ  
Offset: 149 bytes  
Field size: 1 byte  
Range: 0 to 15  
Description: Resonance of filter 1

Parameter: L\_PTCH  
Offset: 150 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of pitch by LFO1

Parameter: MODVFILT1  
Offset: 151 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of filter frequency by assignable source 1

Parameter: MODVFILT2  
Offset: 152 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of filter frequency by assignable source 2

Parameter: MODVFILT3  
Offset: 153 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of filter frequency by assignable source 3

Parameter: MODVPITCH  
Offset: 154 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of pitch by assignable source

Parameter: MODVAMP3  
Offset: 155 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of loudness by assignable keygroup source

Parameter: ENV2L1  
Offset: 156 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Level of envelope 2 at end attack phase (phase 1)

Parameter: ENV2R2  
Offset: 157 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Rate during phase 2 of envelope 2

Parameter: ENV2L2  
Offset: 158 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Level of envelope 2 at end of phase 1

Parameter: ENV2L4  
Offset: 159 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Final envelope 2 level

Parameter: kgmute  
Offset: 160 bytes  
Field size: 1 byte  
Range: 0ffh = off, mute groups 0 to 31  
Description: Keygroup mute group

Parameter: PFXCHAN  
Offset: 161 bytes  
Field size: 1 byte  
Range: 0 to 4  
Description: Effects bus select  
0 = OFF  
1 = FX1  
2 = FX2  
3 = RV3  
4 = RV4

Parameter: PFXSLEV  
Offset: 162 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Effects send level

Parameter: Reserved  
Offset: 163 bytes  
Field size: 5 bytes  
Range:  
Description: Not used

Parameter: LSI2\_ON  
Offset: 168 bytes  
Field size: 1 byte  
Range: 0 = -6dB, 1 = 0dB  
Description: Route audio through second LSI

Parameter: FLT2GAIN  
Offset: 169 bytes  
Field size: 1 byte  
Range: 0 = -6dB, 1 = 0dB  
Description: Make-up gain of second filter

Parameter: FLT2MODE  
Offset: 170 bytes  
Field size: 1 byte  
Range: 0 = Low-pass, 1 = Band-pass, 2 = High-pass, 3 = EQ  
Description: Mode of second filter

Parameter: FLT2Q  
Offset: 171 bytes  
Field size: 1 byte  
Range: 0 to 31  
Description: Resonance of second filter

Parameter: TONEFREQ  
Offset: 172 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Center frequency of tone section

Parameter: TONESLOP  
Offset: 173 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Slope of tone section

Parameter: MODVFLT2\_1  
Offset: 174 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of second filter frequency by source 1

Parameter: MODVFLT2\_2  
Offset: 175 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of second filter frequency by source 2

Parameter: MODVFLT2\_3  
Offset: 176 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Amount of control of second filter frequency by source 3

Parameter: FIL2FR  
Offset: 177 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Basic second filter frequency

Parameter: K\_FRQ2  
Offset: 178 bytes  
Field size: 1 byte  
Range: -24 to +24 semitones  
Description: Second filter key follow

Parameter: ENV3R1 or ATTAK3  
Offset: 179 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Attack rate of envelope 3

Parameter: ENV3L1  
Offset: 180 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Final level of attack phase (phase 1) of envelope 3

Parameter: ENV3R2  
Offset: 181 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Rate of phase 2 of envelope 3

Parameter: ENV3L2  
Offset: 182 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Final level of phase 2 of envelope 3

Parameter: ENV3R3 or DECAY3  
Offset: 183 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Rate of phase 3 of envelope 3

Parameter: ENV3L3 or SUSTN3  
Offset: 184 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Final level of phase 3 of envelope 3

Parameter: ENV3R4 or RELSE3  
Offset: 185 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Rate of release phase (phase 4) of envelope 3

Parameter: ENV3L4  
Offset: 186 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Final target level of envelope 3

Parameter: V\_ATT3  
Offset: 187 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Dependence of envelope 3 attack rate on note-on velocity

Parameter: V\_REL3  
Offset: 188 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Dependence of envelope 3 release rate on note-on velocity

Parameter: O\_REL3  
Offset: 189 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Dependence of envelope 3 release rate on note-off velocity

Parameter: K\_DAR3  
Offset: 190 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Dependence of envelope 3 release and decay rate on key

Parameter: V\_ENV3  
Offset: 191 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Scaling of envelope 3 by note-on velocity

### S2000/S3000XL/S3200XL Common Parameters

Parameter: KFXCHAN  
Offset: 161 bytes  
Field size: 1 byte  
Range: 0 to 5  
Description: Keygroup override Effects Bus select

0 = PRG (use the global program header selection)  
1 = OFF 2 = FX1  
3 = FX2  
4 = RV3  
5 = RV4

Parameter: KFXSLEV  
Offset: 162 bytes  
Field size: 1 byte  
Range: 0 to 99  
Description: Keygroup override Effects Send level

Accessing Sample Header Bytes
-----------------------------

To associate numbers with sample names, it is advised that the S1000 RSLIST (request list of resident sample names) command be used. The machine holds sequential numbers, starting at zero for items in this list and these numbers should be used to identify a specific sample.

### Structure of S3000 Sample header

Parameter: SHIDENT  
Offset: 0 bytes  
Field size: 1 byte  
Range: 3 (Fixed)  
Description: Block identifier

Parameter: SBANDW  
Offset: 1 bytes  
Field size: 1 byte  
Range: 0 represents 10kHz, 1 represents 20kHz  
Description: Sample bandwidth

Parameter: SPITCH  
Offset: 2 bytes  
Field size: 1 byte  
Range: 21 to 127 represents A1 to G8  
Description: Original pitch

Parameter: SHNAME  
Offset: 3 bytes  
Field size: 12 bytes  
Range: String of characters  
Description: Sample name

Parameter: SSRVLD  
Offset: 15 bytes  
Field size: 1 byte  
Range: 0 indicates rate is invalid, 128 indicates rate is valid  
Description: Sample rate validity

Parameter: SLOOPS  
Offset: 16 bytes  
Field size: 1 byte  
Range:  
Description: Number of loops

Parameter: SALOOP  
Offset: 17 bytes  
Field size: 1 byte  
Range:  
Description: First active loop (internal use)

Parameter: SHLOOP  
Offset: 18 bytes  
Field size: 1 byte  
Range:  
Description: Highest loop (internal use)

Parameter: SPTYPE  
Offset: 19 bytes  
Field size: 1 byte  
Range: 0 = Normal looping  
1 = Loop until release  
2 = No looping  
3 = Play to sample end  
Description: Playback type

Parameter: STUNO  
Offset: 20 bytes  
Field size: 2 bytes  
Range:  
Description: Sample tuning offset cent:semi

Parameter: SLOCAT  
Offset: 22 bytes  
Field size: 4 bytes  
Range: Absolute location in Wave memory  
Description: Absolute start address in memory of sample

Parameter: SLNGTH  
Offset: 26 bytes  
Field size: 4 bytes  
Range: Number of data points from start of sample  
Description: Length of sample

Parameter: SSTART  
Offset: 30 bytes  
Field size: 4 bytes  
Range: Number of data points from start of sample  
Description: Offset from start of sample from which playback commences

Parameter: SMPEND  
Offset: 34 bytes  
Field size: 4 bytes  
Range: Number of data points from start of sample  
Description: Offset from start of sample from which playback ceases

**;First Loop**

Parameter: LOOPAT1  
Offset: 38 bytes  
Field size: 4 bytes  
Range: Number of data points from start of sample  
Description: Position in sample of first loop point

Parameter: LLNGTH1  
Offset: 42 bytes  
Field size: 6 bytes  
Range:  
Description: First loop length

Parameter: LDWELL1  
Offset: 48 bytes  
Field size: 2 bytes  
Range: 0 represents No Loop, 9999 = Hold, 1 to 9998 represents Dwell time in milliseconds  
Description: Dwell time of first loop

**;Second Loop**

Parameter: LOOPAT2  
Offset: 50 bytes  
Field size: 4 bytes  
Range: Number of data points from start of sample  
Description: Position in sample of second loop point

Parameter: LLNGTH2  
Offset: 54 bytes  
Field size: 6 bytes  
Range:  
Description: Second loop length

Parameter: LDWELL2  
Offset: 60 bytes  
Field size: 2 bytes  
Range: 0 represents No Loop, 9999 = Hold, 1 to 9998 represents Dwell time in milliseconds  
Description: Dwell time of second loop

**;Third Loop**

Parameter: LOOPAT3  
Offset: 62 bytes  
Field size: 4 bytes  
Range: Number of data points from start of sample  
Description: Position in sample of third loop point

Parameter: LLNGTH3  
Offset: 66 bytes  
Field size: 6 bytes  
Range:  
Description: Third loop length

Parameter: LDWELL3  
Offset: 72 bytes  
Field size: 2 bytes  
Range: 0 represents No Loop, 9999 = Hold, 1 to 9998 represents Dwell time in milliseconds  
Description: Dwell time of third loop

**;Fourth Loop**

Parameter: LOOPAT4  
Offset: 74 bytes  
Field size: 4 bytes  
Range: Number of data points from start of sample  
Description: Position in sample of fourth loop point

Parameter: LLNGTH4  
Offset: 78 bytes  
Field size: 6 bytes  
Range:  
Description: Fourth loop length

Parameter: LDWELL4  
Offset: 84 bytes  
Field size: 2 bytes  
Range: 0 represents No Loop, 9999 = Hold, 1 to 9998 represents Dwell time in milliseconds  
Description: Dwell time of fourth loop

Parameter: SLXY1  
Offset: 86 bytes  
Field size: 4 bytes  
Range:  
Description: Relative loop factors for loop 1

Parameter: SLXY2  
Offset: 98 bytes  
Field size: 4 bytes  
Range:  
Description: Relative loop factors for loop 2

Parameter: SLXY3  
Offset: 110 bytes  
Field size: 4 bytes  
Range:  
Description: Relative loop factors for loop 3

Parameter: SLXY4  
Offset: 122 bytes  
Field size: 4 bytes  
Range:  
Description: Relative loop factors for loop 4

Parameter: SSPARE  
Offset: 134 bytes  
Field size: 1 byte  
Range:  
Description: Used internally

Parameter: SWCOMM  
Offset: 135 bytes  
Field size: 1 byte  
Range:  
Description: Not used

Parameter: SSPAIR  
Offset: 136 bytes  
Field size: 2 bytes  
Range: Block address  
Description: Address of stereo partner (internal use)

Parameter: SSRATE  
Offset: 138 bytes  
Field size: 2 bytes  
Range:  
Description: Sample rate

Parameter: SHLTO  
Offset: 140 bytes  
Field size: 1 byte  
Range: -50 to +50  
Description: Tuning offset of hold loop

Frank Neumann, February 5th, 2002