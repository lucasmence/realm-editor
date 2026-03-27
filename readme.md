# realm-editor

A simplified 2D map editor built with [SFML](https://www.sfml-dev.org/) and [IMGUI](https://github.com/ocornut/imgui).


## Open-source announcement letter

I created this project in 2021 exclusively for creating maps for my first game [Boltcraft](https://store.steampowered.com/app/2005930/Boltcraft/). As I continued using the same engine for subsequent projects, I kept making small changes here and there to adapt it to new features. Even so, from 2021 until now, I haven't changed much in the project's main structure.

You are free to change and edit the source code of this project as you wish.

More experienced C++ developers will notice some anti-patterns along the way; be prepared to deal with global variables and singletons that I used to handle the gameloop while I was learning the language and game development.

## Compatibility

It is currently compatible with the following projects:

- [Boltcraft 2](https://lucasmence.itch.io/boltcraft-ii-redux-edition)
- [Boltcraft](https://store.steampowered.com/app/2005930/Boltcraft/)
- [Protocol Undeath](https://lucasmence.itch.io/protocol-undeath)

## Compilation

The use of **[CMAKE](https://cmake.org/)** is recommended for both Linux and Windows versions; there is also a Visual Studio version available in this repository, provided you move the already compiled libraries to the libs folder inside the project.

## Requirements

The **[SFML 2.5.1 ×64](https://www.sfml-dev.org/download/sfml/2.5.1/)** and **[BOOST 1.7.4](https://www.boost.org/releases/1.74.0/)** versions are required to properly compile the project. 



## Roadmap

With the recurring development of my next projects, I have some plans for Realm-Editor that I will be implementing over time:

- Total HUD conversion to **[IMGUI](https://github.com/SFML/imgui-sfml)**;
- Add preferences configuration;
- Use of **[Autotiling/Bitmasking](https://code.tutsplus.com/how-to-use-tile-bitmasking-to-auto-tile-your-level-layouts--cms-25673t)**;
- Trigger editor, to facilitate the use of syntaxes.
- Integration of the editor within the games themselves.

## License
- Licensed under [CC0 1.0](https://creativecommons.org/public-domain/cc0/)

**[Mence](https://mence.dev/), 2026**