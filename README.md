# Audio Library

[English](#english) | [Español](#español)

## English

A [Geode](https://geode-sdk.org/) mod for Geometry Dash 2.2081 that lists the songs and sound effects the game has downloaded, so you can free disk space after deleting levels.

### Features

- Songs and Effects tabs with name, ID, size and file date.
- Age filter ("older than N days") or manual selection.
- Preview button on each row.
- Disk pie chart: green slice for songs, yellow slice for effects.
- Deletion only after a confirmation that shows the count and size.
- English by default, Spanish with the **ES** button.

### Safety

- Original game audio is never listed or deleted.
- Nothing runs in the background.
- Deleting is refused while a level is open.
- Audio that is playing or downloading is skipped.
- Files are removed through the game's own `MusicDownloadManager`, so its records stay in sync.

The date shown is the file's modification date. Geometry Dash does not store when a file was downloaded.

### Platforms

Windows, macOS (Intel and Apple Silicon) and Android (32 and 64 bit).

### Build

You need the [Geode SDK and CLI](https://docs.geode-sdk.org/getting-started/), with the `GEODE_SDK` environment variable set.

```bash
geode sdk install-binaries
geode build
```

For Android, set `ANDROID_NDK_ROOT` and run:

```bash
geode sdk install-binaries -p android64
geode build -p android64
```

Every push also builds all platforms with GitHub Actions (`.github/workflows/multi-platform.yml`). The combined `.geode` file is in the run's **Build Output** artifact.

## Español

Mod de [Geode](https://geode-sdk.org/) para Geometry Dash 2.2081 que lista las canciones y los efectos que el juego descargó, para liberar espacio después de borrar niveles.

### Funciones

- Pestañas de canciones y efectos con nombre, ID, tamaño y fecha del archivo.
- Filtro por antigüedad ("más de N días") o selección manual.
- Botón para escuchar cada fila.
- Gráfico de pastel del disco: la rebanada verde son canciones y la amarilla, efectos.
- Borrado solo después de confirmar, con la cantidad y el tamaño.
- Inglés por defecto, español con el botón **ES**.

### Seguridad

- El audio original del juego nunca aparece ni se borra.
- No hay procesos en segundo plano.
- No se borra nada con un nivel abierto.
- Se omite el audio que está sonando o descargándose.
- Los archivos se eliminan con el `MusicDownloadManager` del propio juego, para que sus registros sigan al día.

La fecha que se muestra es la de modificación del archivo. Geometry Dash no guarda cuándo se descargó.

### Plataformas

Windows, macOS (Intel y Apple Silicon) y Android (32 y 64 bits).

### Compilar

Necesitas el [SDK y el CLI de Geode](https://docs.geode-sdk.org/getting-started/), con la variable de entorno `GEODE_SDK` definida.

```bash
geode sdk install-binaries
geode build
```

Para Android, define `ANDROID_NDK_ROOT` y ejecuta:

```bash
geode sdk install-binaries -p android64
geode build -p android64
```

Cada push también compila todas las plataformas con GitHub Actions (`.github/workflows/multi-platform.yml`). El `.geode` combinado está en el artefacto **Build Output** de esa ejecución.
