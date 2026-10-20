# Tabify Drums

[English](README.md) · **Русский**

![Tabify Drums](docs/kit.png)

Свободный кроссплатформенный барабанный плагин (VST3 / CLAP / LV2 / AU / Standalone)
на JUCE 8. Идея и набор функций — по мотивам Power Drum Kit.

## Установка

Готовые сборки для Windows, macOS и Linux — на странице
[Releases](https://github.com/destshotGuitar/tabify-drums/releases). Распакуй архив
и скопируй плагин нужного формата в папку плагинов:

| Формат | Windows | macOS | Linux |
|---|---|---|---|
| VST3 | `C:\Program Files\Common Files\VST3` | `~/Library/Audio/Plug-Ins/VST3` | `~/.vst3` |
| CLAP | `C:\Program Files\Common Files\CLAP` | `~/Library/Audio/Plug-Ins/CLAP` | `~/.clap` |
| LV2 | `%APPDATA%\LV2` | `~/Library/Audio/Plug-Ins/LV2` | `~/.lv2` |
| AU | — | `~/Library/Audio/Plug-Ins/Components` | — |

Сборки не подписаны. На macOS после копирования сними карантин:
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Tabify Drums.vst3"`
(и так же для других форматов). На Windows SmartScreen может предупредить
при запуске Standalone — «Подробнее → Выполнить в любом случае».

Сборка из исходников и установка на Linux одной командой: `scripts/install.sh`.

## Что уже есть (v0.1)

- Два кита, переключаются списком в шапке: **Muldjord Kit** (DrumGizmo, CC BY 4.0,
  по умолчанию) и **GM Rock Kit** (Hydrogen, GPL). 18 артикуляций, 5 слоёв по силе удара;
  чего нет в Muldjord (римшот, педаль и полуоткрытый хэт, сплэш, ковбелл, хлопок) —
  берётся из GM Rock. Авторы — `resources/kits/CREDITS.md`.
- Раскладка нот General MIDI, **MIDI Learn** для любой артикуляции (правый клик по барабану).
- Хай-хэт: закрытый / полуоткрытый / открытый / педаль, взаимное заглушение (choke).
- Фото установки (вид сверху), которое реагирует на удары: подсветка и волна по пластику,
  качание и блеск тарелок, открытие хэта. По нему можно играть мышью; Splash, Cowbell
  и Clap, которых нет на фото, — пэды справа внизу.
- Микшер: 10 каналов + мастер, громкость, панорама, компрессор, Mute/Solo, индикаторы уровня.
- Масштабируемое окно, сохранение состояния в проекте DAW.

### Как играть мышью

| Где кликать | Что звучит |
|---|---|
| Малый: центр / обод / корпус | Snare / Rimshot / Side Stick |
| Хэт (малая левая тарелка): центр / край / Shift+клик / стойка | Closed / Open / Semi-Open / Pedal |
| Райд (большая правая): поле / купол | Ride / Bell |
| Пэды справа внизу | Splash / Cowbell / Clap |
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

Сэмплы — в `resources/kits/<кит>/`. У каждой артикуляции 5 файлов, от тихого
удара к сильному: `-Softest`, `-Soft`, `-Med`, `-Hard`, `-Hardest`. Например, бочка
GM Rock: `GMRockKit/Kick-Softest.wav` … `Kick-Hardest.wav`; у Muldjord к имени
добавлен префикс: `MuldjordKit/Muldjord-Kick-Softest.flac`. Чтобы заменить звук —
положи свои WAV или FLAC с теми же именами (любая частота дискретизации; стерео
сводится в моно). Новый кит добавляется строкой в `kits` в `src/KitDefinition.h`.
Если сэмпл один — скопируй его во все 5 файлов.

Кит из DrumGizmo (drumgizmo.org, многомикрофонные записи) превращается в 5 слоёв
скриптом `tools/import_drumgizmo.py` (сводит микрофоны в моно и выбирает удары):

```bash
tools/import_drumgizmo.py MuldjordKit3/KdrumL/KdrumL.xml \
    --mix KdrumL=1.0,OHL=0.3,OHR=0.3,AmbL=0.2,AmbR=0.2 \
    --prefix Muldjord-Kick --out resources/kits/MuldjordKit --format flac
```

Весь Muldjord Kit пересобирается из архива DrumGizmo командой
`scripts/import_muldjord.sh <папка MuldjordKit3>` — там же видно, какие
микрофоны и в каких пропорциях сведены.

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

Код — AGPLv3 (требование открытой лицензии JUCE). Сэмплы — CC BY 4.0 (Muldjord Kit)
и GPL (GM Rock Kit), подробно в `resources/kits/CREDITS.md`. Фото установки и логотип
Tabify (`resources/ui/`) не входят в AGPL; права на них у их владельцев. 
