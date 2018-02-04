# watterpulsecounting - Pulse counting from water utilitymeter

I have utilitity meter for hot and cold water
![Utility meter](doc/JS_Smart.jpg)

The "clock meter" has a reflection part.

detekce otoceni kolecka (toho co vypada jako hodiny v pravo dole)

- je tem takova odraziva ploska (mezi 0 a 6.5),  diky ni funguje docela dobre detekce pres infra red cidlo
- jedno otoceni je jeden litr --> priblizne jsem schopen detekovat 0.5 l pulzy
    - graficky odecteno presneji to je 650ml a 350ml,
    - podle namerenych data 683-689 vs 310-316 (pustil jsem kohoutek a meril jsem casy mezi otackama)
- pokud potece 50l za sekundu
    - 20ms = 1/50 = 0.020 trva jedno otoceni kolecka
    - 13ms = 0.02*0.65  je odraziva cast
    - 7ms = je cerna cast

```
cold_watter_pulse_count,warm_watter_pulse_count,timestamp,timestamp_since_last_send
*s;0;30;74994;1
!S;1;30;75005;111
33 0 75005 60004
!S;1;30;75006;112
*s;1;2;75008;0
*s;1;28;75009;1
```
