# Avoid to include C style standard header file name

Checked the C++ standard 14882, 2017.

In section 20.5.1.2 [header], item 8:
 > $\underline{D.5}$ C standard library headers, describes the effects of using the `name.h`(C header) form in a C++program. $^{172}$

In the comment 172:
 > The ".h" headers dump all their names into the global namespace, whereas the newer forms keep their names in namespacestd. Therefore, **the newer forms are the preferred forms for all uses** except for C++programs which are intended to be strictlycompatible with C.

In Annex D, item 2:
 > These are deprecated features, wheredeprecatedis defined as: Normative for the current edition of thisInternational Standard, but having been identified as a candidate for removal from future revisions. Animplementation may declare library names and entities described in this section with thedeprecatedattribute (10.6.4).

And the  $\underline{D.5}$, marked as `[depr.c.headers]`

**VOICE**: *Don't to be language lawyer, be pragmatic.*

Try <https://gcc.godbolt.org/z/uEYSpH>:

```c++
extern float foo(float);
extern double bar(double);

float test1(float a){return foo(a);}

float test2(float a){return bar(a);}

float test3(double a){return a;}

float test4(float a){return a;}
```

X64 msvc v19.24 generated, flags: /O2

```asm
a$ = 8
float test1(float) PROC                           ; test1, COMDAT
        jmp     float foo(float)                          ; foo
float test1(float) ENDP                           ; test1

a$ = 48
float test2(float) PROC                           ; test2, COMDAT
$LN4:
        sub     rsp, 40                             ; 00000028H
        cvtss2sd xmm0, xmm0
        call    double bar(double)                            ; bar
        cvtsd2ss xmm0, xmm0
        add     rsp, 40                             ; 00000028H
        ret     0
float test2(float) ENDP                           ; test2

a$ = 8
float test3(double) PROC                           ; test3, COMDAT
        cvtsd2ss xmm0, xmm0
        ret     0
float test3(double) ENDP                           ; test3

a$ = 8
float test4(float) PROC                           ; test4, COMDAT
        ret     0
float test4(float) ENDP                           ; test4
```

Conclusion: cast between `double` and `float` is not zero cost, even the cast is implicit.

And compare:

```c++
#include <math.h>
float test1(float a){
    return floor(a);
}
```

```asm
a$ = 48
float test1(float) PROC                           ; test1, COMDAT
$LN4:
        sub     rsp, 40                             ; 00000028H
        cvtss2sd xmm0, xmm0
        call    floor
        cvtsd2ss xmm0, xmm0
        add     rsp, 40                             ; 00000028H
        ret     0
float test1(float) ENDP                           ; test1
```

with:

```c++
#include <cmath>
float test1(float a){
    return floor(a);
}
```

```asm
a$ = 8
float test1(float) PROC                           ; test1, COMDAT
        jmp     floorf
float test1(float) ENDP                           ; test1

_Xx$ = 8
float floor(float) PROC                           ; floor, COMDAT
        jmp     floorf
float floor(float) ENDP                           ; floor
```

Question: Shall we still include legacy C standard header files?

Process of discussion:

  1. What's the fact.
  2. Is it right or wrong, correct or incorrect, true or false?
  3. What are the benifits or values?
  4. What are the opnions?

## end
