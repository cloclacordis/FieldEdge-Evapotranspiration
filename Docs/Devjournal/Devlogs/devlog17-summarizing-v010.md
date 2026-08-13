# devlog17. Подведение итогов для v0.1.0

*A short status/wrap-up entry. Tags the current state as v0.1.0 (not a git tag), tallies the development process, and presents a summary table plus a full test-coverage table mapping TC1–53 to the modules they exercise. Previews the two next stages: a near-term MCU smoke test (compile the existing computational core for Arm Cortex-M4F, verify UART output, without yet building real sensor drivers) and the larger v0.2.0 effort (real sensors, RTOS, EEPROM persistence, daily scheduler).*

* * *

## Введение

В этом девлоге подведем некоторые итоги первого этапа разработки нашего проекта - этапа, в пределах которого была разработана, проверена и оттестирована полная вычислительная модель эталонной эвапотранспирации по методу Пенмана-Монтейта (FAO56, eq. 6) от чтения мок-сенсоров до итоговых значений *ETo* и *ETc*.

Версия фиксируется как ***v0.1.0***.

> *UPD:* будет зафиксирована позже - после имплементации правок, описанных в **devlog18**, и после подготовки документации.

* * *

## Маршрут разработки

Работа велась множественными шагами, которые для документирования процесса были сгруппированы в 16 крупных шагов. Каждый шаг - атомарная единица разработки: новый модуль или пара модулей, обновление оркестрации, тесты. Ни один шаг не оставил проект в нерабочем состоянии: на каждой итерации - условно "каждый час работы" - все программа должна была находиться в компилируемом состоянии. Этот принцип был выбран с самого начала и мы следовали ему строго, несмотря на некоторые издержки такого подхода. Последовательность разработки вычислительных блоков так или иначе отражала структуру уравнения Пенмана-Монтейта (eq. 6) "снизу вверх" - сначала все входящие величины уравнения, деривативы, затем - само уравнение.

* * *

## Состояние v0.1.0

| Параметр | Значение |
|---|---|
| Версия | *v0.1.0* |
| Стандарт | *FAO56 1998* |
| Язык | *C11*, портируемый |
| Платформа | ПК (*Linux x86-64, GCC*) |
| Сборка | *CMake 4.1, FetchContent (Unity)* |
| Тесты | 53 теста, 0 провалов, 0 проигнорировано |
| Покрытие уравнений | *eq*. 6, 7, 8, 11-13, 17, 21-25, 35-40, 42, 47, 56 |
| Сенсоры | *mock*-константы (ПК-этап) |
| Время | `time(NULL)` - для *J* и *lux timestamp* |
| Хранение состояния | нет - сброс при каждом запуске |

* * *

### Тестирование

Как уже сказано, все тестовые сценарии *TC1-TC53* пройдены. Ниже - сводная таблица покрытия:

| Тесты | Модуль |
|---|---|
| *TC1-TC5* | Температура воздуха, *e(T), Δ* |
| *TC6-TC11* | Валидация дней *J* и широт |
| *TC12-TC13* | Перевод координат *DMS* -> *rad* |
| *TC14-TC16* | Внеземная радиация *Ra* (eq. 21) |
| *TC17-TC19* | Накопитель солнечного сияния |
| *TC20-TC24* | Солнечная и *clear-sky* радиация (*Rs, Rso*) |
| *TC25-TC26* | *Date provider* |
| *TC27* | Эмуляция опроса сенсора освещенности |
| *TC28* | Инициализация *LocationData* |
| *TC29-TC32* | Чистая радиация (*Rns, Rnl, Rn*) |
| *TC33* | Накопитель влажности |
| *TC34-TC38* | Атмосферное давление *P, γ* |
| *TC39-TC40* | Давление пара *es, ea* |
| *TC41-TC46* | Скорость ветра *u2* (eq. 47) |
| *TC47-TC53* | Эвапотранспирация *ETo, ETc* (eq. 6, eq. 56) |

* * *

## Следующие шаги

Следующий *ближайший* шаг - *smoke*-тест на микроконтроллере: не вполне портирование - для начала просто запуск вычислительной модели на процессоре МК. Мы хотим, прежде чем переходить к следующему большому этапу - к разработке встраиваемой части нашего ПО, - скомпилировать вычислительную модель под *Arm Cortex-M4F* процессор, вывести результат в терминал хост-ПК через *UART*, чтобы убедиться, что портирование не потребует изменений в вычислительных модулях.

Следующий *крупный* этап - разработка версии ***v0.2.0*** - МК-версии нашего ПО с реальными сенсорами (драйверами), системой реального времени, *EEPROM*-персистентностью, суточным планировщиком и др. Этому этапу будут посвящены отдельные девлоги, будут отдельно сформированы требования и проч.

* * *

## Обновление вывода в `main.c`

Исключительно ради более приятного внешнего вида мы обновили блок кода в файле оркестрации, который отвечает за вывод данных в терминал:

```C
    /* *** Output *** */
    #define COL_W 38
    #define SEP " = "
    
    /* Data sources */
    (void)printf("\n=== Data sources ===\n");
    (void)printf("%-*s%s%s\n", COL_W, "Air temperature",
                 SEP, SensorValueSource_ToString(t_sample.source));

    (void)printf("%-*s%s%s\n", COL_W, "Illuminance (daily data)",
                 SEP, SensorValueSource_ToString(sunshine_data.source));
    
    /* Air temperature & saturation vapour pressure */
    (void)printf("\n=== Air temperature and saturation vapour pressure ===\n");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Tmin", SEP, temperature_data.T_min_C, "C");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Tmax", SEP, temperature_data.T_max_C, "C");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Tmean", SEP, temperature_data.T_mean_C, "C");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "e(Tmean)", SEP, e_tmean, "kPa");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "es", SEP, e_s, "kPa");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "delta", SEP, delta, "kPa/C");
    
    /* Atmospheric parameters */
    (void)printf("\n=== Atmospheric parameters ===\n");
    (void)printf("%-*s%s%12.2f kPa (source: %s)\n", COL_W, "P", SEP, atmos_data.P_kPa,
                 (pressure_sample.source == SENSOR_VALUE_MEASURED) ? "sensor" : "model/constant");

    (void)printf("%-*s%s%12.5f %-6s\n", COL_W, "gamma", SEP, atmos_data.gamma_kPa_per_C, "kPa/C");
    (void)printf("%-*s%s%12.1f %-6s\n", COL_W, "RHmax", SEP, humidity_data.RH_max, "%");
    (void)printf("%-*s%s%12.1f %-6s\n", COL_W, "RHmin", SEP, humidity_data.RH_min, "%");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "ea", SEP, ea_kpa, "kPa");
    
    /* Wind speed */
    (void)printf("\n=== Wind speed ===\n");
    (void)printf("%-*s%s%s\n", COL_W, "Source", SEP, SensorValueSource_ToString(wind_sample.source));
    (void)printf("%-*s%s%12.1f %-6s\n", COL_W, "Anemometer height (z)", SEP, wind_data.height_m, "m");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "uzmean", SEP, wind_data.u_z_mean_m_s, "m/s");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "u2 (eq. 47)", SEP, u2, "m/s");
    
    /* Astronomy */
    (void)printf("\n=== Astronomy, at J = %u, phi = %.4f rad = %.2f deg ===\n",
                 day_data.J, location.latitude_rad, location.latitude_rad * (180.0 / PI));

    (void)printf("%-*s%s%12u\n", COL_W, "Current day of year (J)", SEP, current_j);
    (void)printf("%-*s%s%12.4f\n", COL_W, "Inverse relative distance", SEP, day_data.dr);
    
    (void)printf("%-*s%s%12.4f rad (%6.2f deg)\n", COL_W, "Solar declination",
                 SEP, day_data.delta_rad, day_data.delta_rad * (180.0 / PI));

    (void)printf("%-*s%s%12.4f rad\n", COL_W, "Sunset hour angle", SEP, day_data.omega_s_rad);
    (void)printf("%-*s%s%12.2f h\n", COL_W, "Daylight hours (N)", SEP, day_data.N_hours);
    
    /* Extraterrestrial radiation & equivalent evaporation */
    (void)printf("\n=== Extraterrestrial radiation and equivalent evaporation ===\n");
    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Extraterrestrial radiation (Ra)",
                 SEP, ra_data.Ra_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Equivalent evaporation (from Ra_daily)",
                 SEP, ra_data.Ra_daily * 0.408, "mm/day");
    
    /* Solar & clear-sky radiation */
    (void)printf("\n=== Solar and clear-sky radiation ===\n");
    (void)printf("%-*s%s%12.2f\n", COL_W, "Angstrom a_s", SEP, angstrom.a_s);
    (void)printf("%-*s%s%12.2f\n", COL_W, "Angstrom b_s", SEP, angstrom.b_s);
    
    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Solar radiation (Rs)",
                 SEP, solar_radiation.Rs_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Clear-sky radiation (Rso)",
                 SEP, solar_radiation.Rso_daily, "MJ m-2 day-1");
    
    /* Net radiation */
    (void)printf("\n=== Net radiation ===\n");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "ea (actual vapour pressure)", SEP, ea_kpa, "kPa");
    
    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Net shortwave radiation (Rns)",
                 SEP, net_radiation.Rns_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Net longwave radiation (Rnl)",
                 SEP, net_radiation.Rnl_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Net radiation (Rn)",
                 SEP, net_radiation.Rn_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Equivalent evaporation (from Rn_daily)",
                 SEP, net_radiation.Rn_daily * 0.408, "mm/day");
    
    /* Sunshine duration */
    (void)printf("\n=== Sunshine duration ===\n");
    (void)printf("%-*s%s%12.0f %-6s\n", COL_W, "Binarization threshold",
                 SEP, CONFIG_BRIGHT_LUX_THRESHOLD, "lux");

    (void)printf("%-*s%s%12u %-6s\n", COL_W, "Sampling interval",
                 SEP, (unsigned)CONFIG_SAMPLE_PERIOD_SEC, "s");

    (void)printf("%-*s%s%12u\n", COL_W, "Total samples", SEP, sunshine_data.total_samples);
    (void)printf("%-*s%s%12u\n", COL_W, "Bright samples", SEP, sunshine_data.bright_samples);
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Sunshine duration (n)", SEP, sunshine_data.n_hours, "h");
    
    /* Evapotranspiration */
    (void)printf("\n=== Evapotranspiration ===\n");
    (void)printf("%-*s%s%12.3f %-6s\n", COL_W, "ETo (eq. 6, Penman-Monteith)", SEP, eto_mm_day, "mm/day");
    (void)printf("%-*s%s%12.2f\n", COL_W, "Kc (crop coefficient)", SEP, CONFIG_CROP_KC);
    (void)printf("%-*s%s%12.3f %-6s\n", COL_W, "ETc (eq. 56, Kc * ETo)", SEP, etc_mm_day, "mm/day");

    #undef COL_W
    #undef SEP
```
