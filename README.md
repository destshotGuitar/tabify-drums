# Tabify Drums

Свободный кроссплатформенный барабанный плагин (VST3 / CLAP / LV2 / AU / Standalone)
на JUCE 8. Идея и набор функций — по мотивам Power Drum Kit.

Установка на Linux одной командой: `scripts/install.sh`.

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
cmake --build build --target TabifyDrums_All TabifyDrums_CLAP
```

Результат — в `build/TabifyDrums_artefacts/Release/{VST3,CLAP,LV2,Standalone}`.

Зависимости на Fedora:

```bash
sudo dnf install cmake ninja-build gcc-c++ alsa-lib-devel pipewire-jack-audio-connection-kit-devel \
  freetype-devel fontconfig-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel
```

Без `alsa-lib-devel` плагины собираются, но у Standalone не будет звука.

Windows и macOS собираются в GitHub Actions (`.github/workflows/build.yml`) — артефакты
на каждой сборке.

## Как менять самому

После любых изменений: `scripts/install.sh` — пересоберёт и установит плагин,
затем в DAW переоткрой плагин (или пересканируй плагины).

### Сэмплы

Все сэмплы — в `resources/kits/GMRockKit/`. У каждой артикуляции 5 файлов, от тихого
удара к сильному: `-Softest`, `-Soft`, `-Med`, `-Hard`, `-Hardest`. Например, бочка:
`Kick-Softest.wav` … `Kick-Hardest.wav`. Чтобы заменить звук — положи свои WAV
с теми же именами (любая частота дискретизации; стерео сводится в моно).
Если сэмпл один — скопируй его во все 5 файлов.

Какие файлы к какому барабану и какие ноты по умолчанию — в таблице
`src/KitDefinition.h` (`articulations`).

### Интерфейс

| Что | Где |
|---|---|
| Фото установки | `resources/ui/kit.jpg` (1920×1088) |
| Логотип | `resources/ui/logo.png` (PNG с прозрачностью) |
| Зоны клика на фото | `src/gui/DrumKitView.cpp`, константы `tom1Head`, `rideOval`… в пикселях фото |
| Цвета | `src/gui/DrumLookAndFeel.h`, `palette` |
| Шапка, вкладки, нижняя панель | `src/PluginEditor.cpp` (`Surface::paint`, `showPage`) |
| Название плагина | `CMakeLists.txt`, `PRODUCT_NAME` |

Если заменишь фото на другое — нужно поправить зоны (центр и радиусы овалов).

## Проверка

```bash
cmake --build build --target TabifyDrumsTestHost TabifyDrumsSnapshot
build/TabifyDrumsTestHost_artefacts/Release/TabifyDrumsTestHost build/TabifyDrums_artefacts/Release/VST3/"Tabify Drums.vst3" out
build/TabifyDrumsSnapshot out
```

`TabifyDrumsTestHost` грузит VST3 как DAW, проверяет каждую артикуляцию, слои, сохранение
состояния и пишет `out/beat.wav`. `TabifyDrumsSnapshot` сохраняет скриншоты интерфейса.

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
