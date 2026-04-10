# Catputer Desktop

`Catputer Desktop` is the new desktop-side mirror for Catputer.

Current scope:
- portrait/mobile-first window layout
- top-half stage, bottom-half chat and interaction panels
- local multi-cat warehouse
- orange and purple cat variants
- same cat sprite frames as the handheld device
- matching stage-style background, time bar, and status overlay
- local pet stats and actions
- local outing and souvenir box
- local chat panel
- photo preview from `assets/photos`
- local save/load
- device sync tab for listening to Catputer broadcast and pushing text commands
- town-sync flow for moving one selected cat back to the handheld
- desktop cat house with living room / bedroom split
- per-cat memories, prompts, and light companionship scenes

The desktop app now supports real handheld sync, cat warehouse management, and desktop-only companionship logic.

Run locally with Godot 4.6:

```powershell
<path-to-godot>\Godot_v4.6-stable_mono_win64.exe --path .\desktop\CatputerGodot
```

Planned next:
- refine the handheld-sync scene and warehouse flow
- deepen companionship scenes without overcomplicating the UI
- keep the desktop app as the richer "companion brain"
