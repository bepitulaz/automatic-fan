Automatic FAN
=============

FAN that can be set with lower and upper temperature-thresholds so it can adjust
its FAN speed. The controller is ATMega8535 interfaced with LM35 that sampled with
built-in ADC. The FAN can be connected to [H-Bridge](http://en.wikipedia.org/wiki/H_bridge) driver.

## Compile it

The `source.c` is compiled with [CodeVisionAVR](http://hpinfotech.ro/html/cvavr.htm).
