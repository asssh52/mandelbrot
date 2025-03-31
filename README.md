
# SIMD-Оптимизация расчёта множества Мандельброт.
В этой работе изучались способы ускорить расчёт множества [Мандельброт](https://ru.wikipedia.org/wiki/Множество_Мандельброта/). Для визуализации использовалась графическая библиотека [SFML](https://www.sfml-dev.org). 

## Цели:

- Написать базовую версию программы, а также версии с развёрнутыми циклами и использованием [intrinsic](https://en.wikipedia.org/wiki/Intrinsic_function) функций.
- Проверить эффективность оптимизатора компилятора. (флаг компиляции -O3)
- Произвести анализ результатов, сравнить скорости выполнения обновлённой программы и первоначальной.

## Оборудование:
- Работа проводилась на удалённом сервере используя подключение через `ssh-туннель` и работу с графикой через XQuartz, характеристики процессора будут описаны ниже.

## Основная часть:
В качестве способа измерения производительности был выбран 'отсчёт' тактов процессора с помощью функции `__rdtsc`, хотя и существует несколько аналогов, таких как: `clock()`, `time()`. Обсудим отдельно каждый из этих способов:

- `time()`, возвращает число секунд истёкшее с 1 января 1970 года. Не подходит нам из-за низкой точности (цены деления) - 1 секунда.
- `clock()`, достойный кандидат, который используется в коде, в графическом режиме, для периодического отображения количества кадров в секунду. Точность зависит от системы, в системах UNIX обычно составляет 1 нс (как и в нашем случае). Он сообщает время прошедшее со старта [процесса](https://learn.microsoft.com/ru-ru/cpp/c-runtime-library/reference/clock?view=msvc-170). В среднем считается менее точным способом для измерения времени, вероятно, из-за того, что эта функция не соответствует стандарту ISO C, указывающего чистое время ЦП, да и задержка при вызове побольше чем у того же `__rdtsc`.
- `__rdtsc`, cоздает инструкцию, которая возвращает метку времени процессора. Метка времени процессора записывает количество циклов часов с момента последнего [сброса](https://learn.microsoft.com/ru-ru/cpp/intrinsics/rdtsc?view=msvc-170).

Вот интересная [статья](https://habr.com/ru/articles/818965/).

Итак, как `__rdtsc`, так и `clock()`, могут быть прерваны внешними процессами из-за системного таймера, как оказалось, эти 'задержки' могут достигать $0,5 * 10^9$ тактов процессора (при общем 'времени измерения' $0,5 * 10^9$ ), то есть примерно $0,2$ секунды, так что большой разницы в этих двух способах измерения на данной системе в наших условиях - нет.

Основной алгоритм обработки точек был следующий:
    
```c++
int evalPoint(float x_arg, float y_arg){
    float x_2 = 0, y_2 = 0;
    float x_y = 0 , x_next = 0, y_next = 0;
    int i = 0;

    for (; i < I_MAX && x_2 + y_2 < RNG_MX; i++){
        x_2 = x_next * x_next;
        y_2 = y_next * y_next;
        x_y = x_next * y_next;

        y_next = 2 * x_y + y_arg;
        x_next = x_2 - y_2 + x_arg;
    }

    if (i == I_MAX) return 0;

    return i;
}
```

Здесь представлена функция, которой на вход подаются координаты исследуемой точки, координаты `x` и `y` преобразуются, пока сумма их квадратов не превысит заданный константой `RNG_MX` радиус в квадрате $(100)$. Если точка не ушла за пределы заданной области за `I_MAX` $(256)$ итераций, то мы считаем, что она принадлежит искомому множеству.

В свою очередь 'evalPoint' вызывается из следующей функции, в которой и происходит небольшой расчёт координат точки исходя из координат пикселя, за этот расчёт отвечают следующие константы:`Y_RES`, `Y_OFF`, `Y_SCL`, `X_RES`, `X_OFF`, `X_SCL`. Эта функция и отвечает за расчёт времени, на вход она получает единственный аргумент `testIter`, отвечающий за увеличение нагрузки путём расчёта каждой точки `testIter` раз.

```c++
unsigned long long evalTime(int testIter){
    unsigned long long start_rdtsc = __rdtsc();

    for (int i = 0; i < testIter; i++){
        for (int y = 0; y < Y_RES; y++){
            float y_arg = ((float)y / Y_RES - Y_OFF) * Y_SCL;

            for (int x = 0; x < X_RES; x++){
                float x_arg = ((float)x / X_RES - X_OFF) * X_SCL;

                volatile int iter = evalPoint(x_arg, y_arg);
            }
        }
    }

    unsigned long long end_rdtsc = __rdtsc();
    unsigned long long time_rdtsc = end_rdtsc - start_rdtsc;

    return time_rdtsc;
}
```

Настало время первой оптимизации. Развернём циклы и посмотрим что сделает оптимизатор компилятора. Для этого была в большей степени видоизменена функция `evalPoint`. 

```c++
inline int evalPoint(float* x_arg, float* y_arg, volatile int* iter){
    float x_2[XMM_FLOAT_SIZE] = {}, y_2[XMM_FLOAT_SIZE] = {};
    float x_y[XMM_FLOAT_SIZE] = {}, x_next[XMM_FLOAT_SIZE] = {}, y_next[XMM_FLOAT_SIZE] = {};

    for (int i_glbl = 0; i_glbl < I_MAX; i_glbl++){

        for (int i = 0; i < XMM_FLOAT_SIZE; i++){
            x_2[i] = x_next[i] * x_next[i];
            y_2[i] = y_next[i] * y_next[i];
            x_y[i] = x_next[i] * y_next[i];
        }

        int cmp[XMM_FLOAT_SIZE] = {};

        for (int i = 0; i < XMM_FLOAT_SIZE; i++){
            if (x_2[i] + y_2[i] < RNG_MX) cmp[i] = 1;
        }

        int mask = 0;
        for (int i = 0; i < XMM_FLOAT_SIZE; i++){
            mask = mask | (cmp[i] << i);
        }

        if (!mask) break;

        for (int i = 0; i < XMM_FLOAT_SIZE; i++){
            iter[i] += cmp[i];
            y_next[i] = 2 * x_y[i] + y_arg[i];
            x_next[i] = x_2[i] - y_2[i] + x_arg[i];
        }
    }

    for (int i = 0; i < XMM_FLOAT_SIZE; i++){
        if (iter[i] == I_MAX) iter[i] = 0;
    }

    return 0;
```

Все действия над координатами происходят в функции выше, они обрабатываются в 'пачках' по `XMM_FLOAT_SIZE`, то есть по $ 8 $ так как наш процессор поддерживает технологию AVX и ширина векторных регистров $ 256 $ бит, то есть помещается $8$ чисел типа float. Программа должна ускориться в $ 8 $ раз? На практике оказалось немного иначе.

Для анализа ситуации я воспользовался сайтом [godbolt.org](https://godbolt.org). Ниже представлен небольшой отрывок из листинга программы с развёрнутыми циклами, сгенерированного на этом сайте. Видно, что никакими SIMD-инструкциями и не пахнет (в оригинале, он в 5 раз больше).

```c++
for (int i = 0; i < XMM_FLOAT_SIZE; i++){
            x_2[i] = x_next[i] * x_next[i];
            y_2[i] = y_next[i] * y_next[i];
            x_y[i] = x_next[i] * y_next[i];
        }
```

```asm
    ...
    vmulss  xmm0, xmm10, xmm10
    vmovss  xmm10, DWORD PTR [rsp-64]
    vmovss  DWORD PTR [rsp-72], xmm1
    vmulss  xmm1, xmm10, xmm10
    vmovss  DWORD PTR [rsp-108], xmm0
    vmulss  xmm0, xmm15, xmm15
    vmulss  xmm15, xmm11, xmm11
    vmulss  xmm11, xmm11, xmm10
    vmovss  xmm10, DWORD PTR [rsp-60]
    vmovss  DWORD PTR [rsp-104], xmm1
    vmulss  xmm1, xmm12, xmm12
    vmovss  DWORD PTR [rsp-56], xmm0
    ...
```

Теперь оценим листинг первой версии программы:

```c++
for (; i < I_MAX && x_2 + y_2 < RNG_MX; i++){
        x_2 = x_next * x_next;
        y_2 = y_next * y_next;
        x_y = x_next * y_next;

        ...
    }
```

```asm
    vmovss  xmm0, DWORD PTR [rbp-12]
    vmulss  xmm0, xmm0, xmm0
    vmovss  DWORD PTR [rbp-4], xmm0

    vmovss  xmm0, DWORD PTR [rbp-16]
    vmulss  xmm0, xmm0, xmm0
    vmovss  DWORD PTR [rbp-8], xmm0

    vmovss  xmm0, DWORD PTR [rbp-12]
    vmulss  xmm0, xmm0, DWORD PTR [rbp-16]
    vmovss  DWORD PTR [rbp-24], xmm0
```
Казалось бы, о каком ускорении тут может идти речь - количество инструкций выросло более чем в $5$ раз! Но числа говорят, что в среднем код стал работать в $4$ раза быстрее! Это связано с прекрасной работой оптимизатора `-O3`, не смотря на то, что он не увидел возможность воспользоваться векторными (SIMD) операциями, он за счёт конвееризации инструкций смог добиться такого. 

Задержка на операциях `vmulss` и `vmovss` на различных архитектурах составляет около $4-6$ тактов процессора, так что результат с теорией сходится.

Осталось самое интересное, SIMD-оптимизация:

```c++
inline void evalPoint(__m256 x_arg, __m256 y_arg, volatile __m256i* iter){
    __m256 x = _mm256_set1_ps(0);
    __m256 y = _mm256_set1_ps(0);
    __m256i i256 = _mm256_set1_epi32(I_MAX);

    _mm256_storeu_si256((__m256i*)&x, (__m256i)x_arg);
    _mm256_storeu_si256((__m256i*)&y, (__m256i)y_arg);

    for (int i_glbl = 0; i_glbl < I_MAX; i_glbl++){

        __m256 x_2 = _mm256_mul_ps(x, x);
        __m256 y_2 = _mm256_mul_ps(y, y);
        __m256 x_y = _mm256_mul_ps(x, y);
        __m256 rad = _mm256_add_ps(x_2, y_2);

        __m256 cmp = _mm256_cmp_ps(rad, mRNG_MX, _CMP_LT_OQ);

        int mask = _mm256_movemask_ps(cmp);

        if (!mask) break;

        *iter = _mm256_sub_epi32(*iter, (__m256i)cmp);

        _mm256_storeu_si256((__m256i*)&x, (__m256i)_mm256_add_ps(_mm256_sub_ps(x_2, y_2), x_arg));
        _mm256_storeu_si256((__m256i*)&y, (__m256i)_mm256_add_ps(_mm256_add_ps(x_y, x_y), y_arg));
    }

    __m256i iter_cmp_bound = _mm256_cmpeq_epi32(*iter, i256);
    for (int i = 0; i < XMM_FLOAT_SIZE; i++){
        if (((int*)&iter_cmp_bound)[i]) ((int*)iter)[i] = 0;
    }
}
```

## Измерения.

И собственно, сами измерения ([вычисления тут](https://docs.google.com/spreadsheets/d/1acxNXa0A9ztoG274Km965IaiOIhCFw_2rN61hcWLlIQ/edit?usp=sharing)):

| Программа       | simple  | simple-O3 | loop_unfolded | loop_unfolded-O3 | intrinsic | intrinsic-O3 |
|----------------|---------|-----------|---------------|------------------|-----------|--------------|
| **Циклы $\cdot10^9$**  | 33,371  | 18,316    | 64,313        | 8,162            | 11,360    | 2,511        |
| **Ускорение**     | 1,000   | 1,822     | 0,519         | 4,088            | 2,938     | 13,290       |
| **Погрешность (%)**     | 0,3   | 0,3     | 0,6         | 2,2            | 2,4     | 2,1        |

## Выводы.

- Были написаны 3 версии программы с разными видами 'наполнения' основной расчётной функции. Картинки и оборудование можно будет посмотреть ниже.
- Оптимизатор `-O3` творит чудеса, независимо от способа написания функции `evalTime`, он ускорил работу в `0.8 - 8` раз.
- Развёртка циклов помогла оптимизатору компилятора увеличить итоговую производительность в 4 раза.
- SIMD-функции позволяют невероятно повысить скорость работы программы, в этой работе был достигнуто число **1230%**, несмотря на их эффективность, за ними кроется и минус - они зависимы от аппаратуры, под которую пишется программа. Например, на другом процессоре, не поддерживающим технологию AVX и AVX2 пришлось бы использовать другие векторные функции.

## Картинки.
![IMAGE 2025-04-01 01:27:15](https://github.com/user-attachments/assets/b74c3663-7f45-4866-89f4-cb92f1caf068)
![IMAGE 2025-04-01 01:27:18](https://github.com/user-attachments/assets/9233d334-b0ca-461b-886a-d275f228b1a4)




## Характеристики процессора.
- description: CPU
- product: QEMU Virtual CPU version 4.2.0
- vendor: Intel Corp.
- physical id: 400
- bus info: cpu@0
- version: 6.58.9
- slot: CPU 0
- size: 2GHz
- capacity: 2GHz
- width: 64 bits
- capabilities: fpu fpu_exception wp vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 syscall nx pdpe1gb rdtscp x86-64 constant_tsc rep_good nopl xtopology cpuid tsc_known_freq pni pclmulqdq ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm cpuid_fault invpcid_single pti ssbd ibrs ibpb fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap xsaveopt arat md_clear
          configuration: cores=1 enabledcores=1 microcode=1 threads=1
