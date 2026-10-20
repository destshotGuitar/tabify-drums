# OpenDrums

Свободный кроссплатформенный барабанный плагин (VST3 / CLAP / LV2 / AU / Standalone)
на JUCE 8. Идея и набор функций — по мотивам Power Drum Kit.

## Что уже есть (v0.1)

- Встроенный кит **GMRockKit** (Hydrogen, GPL): 18 артикуляций, 5 слоёв по силе удара.
- Раскладка нот General MIDI, **MIDI Learn** для любой артикуляции (правый клик по барабану).
- Хай-хэт: закрытый / полуоткрытый / открытый / педаль, взаимное заглушение (choke).
- Фото установки (вид сверху), которое реагирует на удары: подсветка и волна по пластику,
  качание и блеск тарелок, открытие хэта. По нему можно играть мышью; Splash, Cowbell
  и Clap, которых нет на фото, — пэды слева.
- Микшер: 10 каналов + мастер, громкость, панорама, компрессор, Mute/Solo, индикаторы уровня.
- Масштабируемое окно, сохранение состояния в проекте DAW.

### Как играть мышью

| Где кликать | Что звучит |
|---|---|
| Малый: центр / обод / корпус | Snare / Rimshot / Side Stick |
| Хэт (малая правая тарелка): центр / край / Shift+клик | Closed / Open / Semi-Open (Pedal — через правый клик) |
| Райд (левая тарелка): поле / купол | Ride / Bell |
| Пэды слева | Splash / Cowbell / Clap |
| Правый клик по любому барабану | Все артикуляции, MIDI Learn, сброс нот |

### Ноты по умолчанию (GM)

Kick 36, 35 · Snare 38 · Rimshot 40 · Side Stick 37 · Clap 39 · HH Closed 42 ·
HH Pedal 44 · HH Open 46 · HH Semi 23 · Tom 1 48, 50 · Tom 2 45, 47 ·
Floor Tom 41, 43 · Crash 49, 57 · Ride 51 · Bell 53 · Ride 24" 59 · Splash 55 · Cowbell 56

## Сборка

```bash
git clone --recursive <repo> && cd drumkit
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target OpenDrums_All OpenDrums_CLAP
```

Результат — в `build/OpenDrums_artefacts/Release/{VST3,CLAP,LV2,Standalone}`.

Зависимости на Fedora:

```bash
sudo dnf install cmake ninja-build gcc-c++ alsa-lib-devel pipewire-jack-audio-connection-kit-devel \
  freetype-devel fontconfig-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel
```

Без `alsa-lib-devel` плагины собираются, но у Standalone не будет звука.

Windows и macOS собираются в GitHub Actions (`.github/workflows/build.yml`) — артефакты
на каждой сборке.

## Проверка

```bash
cmake --build build --target OpenDrumsTestHost OpenDrumsSnapshot
build/OpenDrumsTestHost_artefacts/Release/OpenDrumsTestHost build/OpenDrums_artefacts/Release/VST3/OpenDrums.vst3 out
build/OpenDrumsSnapshot out
```

`OpenDrumsTestHost` грузит VST3 как DAW, проверяет каждую артикуляцию, слои, сохранение
состояния и пишет `out/beat.wav`. `OpenDrumsSnapshot` сохраняет скриншоты интерфейса.

## Планы

- Библиотека грувов и сбивок (Groove MIDI Dataset, CC-BY 4.0), синхронно с темпом DAW,
  перетаскивание MIDI в дорожку.
- Генератор сбивок.
- Раздельные выходы по каналам, EQ и ревербератор.
- Пресеты, режим электронных барабанов (CC4 для хэта, choke тарелок по aftertouch).
- Загрузка других китов (Hydrogen `.h2drumkit`, DrumGizmo).
- Перевод интерфейса.

## Лицензия

Код — AGPLv3 (требование открытой лицензии JUCE). Сэмплы GMRockKit — GPL
(Glen MacArthur / Sebastian Moors, из проекта Hydrogen). Фото установки
(`resources/ui/kit.jpg`) — права нужно подтвердить перед публикацией.
