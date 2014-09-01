msgflow
=========

msgflow is a tool to convert logs into text diagram (much like UML interaction diagram).

Usages:
   see test files for usage details.

Examples
--------------

```
[sys1] ---->   [sys2] 00001  MSG_0  0x00010002 0x00030004
[sys2] ---->   [sys3] 00002  MSG_1  0x00010002 0x00030004
[sys3] ---->   [sys1] 00003  MSG_2  0x00010002 0x00030004
```
into:

```
sys1   sys2   sys3   
|----->|      |     MSG_0   [0x00010002 0x00030004]
|      |----->|     MSG_1   [0x00010002 0x00030004]
|<------------|     MSG_2   [0x00010002 0x00030004]
```

------------------------------

```
xxxxxxxxxxxx
[sys1] ---->   [sys2] 00001  MSG_0  0x00010002 0x00030004
yyyyyyyyyyyy
zzzzzzzzzzzz
[sys2] ---->   [sys3] 00002  MSG_1  0x00010002 0x00030004
[sys3] ---->   [sys1] 00003  MSG_2  0x00010002 0x00030004
kkkkkkkkkkkk
```

into

```
sys3   sys2   sys1   
  |      |<-----|     MSG_0   [1] [0x00010002 0x00030004]
  |<-----|      |     MSG_1   [2] [0x00010002 0x00030004]
  |------------>|     MSG_2   [3] [0x00010002 0x00030004]
[0] xxxxxxxxxxxx
[1] yyyyyyyyyyyy
[1] zzzzzzzzzzzz
[3] kkkkkkkkkkkk
```
