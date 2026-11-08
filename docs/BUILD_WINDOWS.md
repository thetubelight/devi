# Building and running Devi on Windows with VS Code

A complete walkthrough, from a clean Windows machine to a running `devi.exe`. Follow the steps in order.
Every step says what you should see when it worked, so you know when to move on.

**What you'll need:** Windows 10/11 (64-bit), about 3 GB of disk space, and an internet connection.

**What you'll end up with:** `devi.exe`, a UCI chess engine you can play from a chess GUI, plus the unit/perft tests passing.

---

## Step 0: Pick a compiler (decide once)

Devi needs a C++20 compiler and CMake. Choose **one** of these two toolchains:

| | Path A: MSYS2 / MinGW-w64 (GCC) | Path B: Visual Studio Build Tools (MSVC) |
|---|---|---|
| Size | ~1.5 GB | ~3-6 GB |
| Needs PATH edits | Yes (one entry) | No |
| Output binary | `build\devi.exe` | `build\Release\devi.exe` |
| Debugging in VS Code | Included (gdb) | Via CMake Tools "Debug" button |
| Recommended for | Most people, simplest to reason about | If you already have Visual Studio |

If unsure, use **Path A**. Paths A and B are alternatives; don't do both.

---

## Step 1: Install Git

1. Download **Git for Windows** from https://git-scm.com/download/win and run the installer. The default options are fine.
2. Open a **new** PowerShell window (Start menu > type `PowerShell`) and run:

   ```powershell
   git --version
   ```

   You should see something like `git version 2.4x.x.windows.x`.

## Step 2: Install VS Code and its extensions

1. Download VS Code from https://code.visualstudio.com/ and install it. On the "Select Additional Tasks" page, tick
   **Add to PATH** (it is ticked by default).
2. Open VS Code. Press `Ctrl+Shift+X` to open the Extensions panel and install these two (search for each name; publisher is **Microsoft**):
   - **C/C++** (`ms-vscode.cpptools`)
   - **CMake Tools** (`ms-vscode.cmake-tools`)

   (When you open the Devi folder later, VS Code will also offer these as "recommended extensions"; you can accept it there.)

## Step 3: Install the compiler toolchain

### Path A: MSYS2 / MinGW-w64 (GCC)

1. Download the installer from https://www.msys2.org/ and run it. Keep the default install folder `C:\msys64`.
   At the end, untick "Run MSYS2 now" and click Finish.
2. From the Start menu open **MSYS2 UCRT64**.
   It must be the **UCRT64** entry. Not "MSYS2 MSYS", "MINGW64" or "CLANG64". A black terminal window opens.
3. Update the package database. Run:

   ```bash
   pacman -Syu
   ```

   Press `Y` when asked. If the terminal says it must close, close it, open **MSYS2 UCRT64** again and run `pacman -Syu` once more
   until it says there is nothing to do.
4. Install the compiler, debugger, CMake and Ninja:

   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
   ```

   When it asks `Enter a selection (default=all):` just press `Enter`, then `Y` to proceed.
5. Add the tools to your Windows `PATH` so VS Code and PowerShell can find them:
   1. Press the Windows key, type **environment variables**, and open **Edit environment variables for your account**.
   2. Under *User variables*, select **Path** and click **Edit**.
   3. Click **New** and paste `C:\msys64\ucrt64\bin`, then **OK** on every dialog.
6. **Close all PowerShell and VS Code windows**, then open a fresh PowerShell and check:

   ```powershell
   g++ --version
   cmake --version
   ninja --version
   gdb --version
   ```

   Each prints a version. You need g++ 10 or newer and CMake 3.16 or newer (MSYS2 installs much newer ones).
   If any says *"is not recognized"*, see [Troubleshooting](#troubleshooting).

### Path B: Visual Studio Build Tools (MSVC)

1. Open https://visualstudio.microsoft.com/downloads/, scroll to **All Downloads > Tools for Visual Studio**, and download
   **Build Tools for Visual Studio 2022** (or install the free *Visual Studio 2022 Community*; the steps are the same).
2. Run the installer. On the **Workloads** tab tick **Desktop development with C++** (it includes the MSVC compiler,
   the Windows SDK and CMake). Click **Install** and wait.
3. No PATH changes are needed. VS Code finds this toolchain by itself. For command-line builds use the Start-menu entry
   **Developer PowerShell for VS 2022**, which has the compiler on its PATH.

---

## Step 4: Get the code and open it in VS Code

1. Pick a folder **without** spaces that is **not** inside OneDrive (OneDrive sync slows builds). For example `C:\dev`:

   ```powershell
   mkdir C:\dev
   cd C:\dev
   git clone https://github.com/thetubelight/devi.git
   cd devi
   code .
   ```

   (Replace the URL with this repository's address. `code .` opens the folder in VS Code; or use *File > Open Folder...*.)
2. If VS Code asks **"Do you trust the authors of the files in this folder?"** click **Yes, I trust the authors**.
3. If it offers **"Install recommended extensions"** click **Install** (they're the two from Step 2).

## Step 5: Configure and build

### Option 1: with the CMake Tools buttons (works for Path A and Path B)

1. CMake Tools asks **"Would you like to configure project 'devi'?"**. Click **Yes**.
   (If you missed it: press `Ctrl+Shift+P`, run **CMake: Configure**.)
2. A list of **kits** (compilers) appears. Choose:
   - Path A: the entry named like **`GCC 1x.x.x x86_64-w64-mingw32 (ucrt64)`**
   - Path B: **`Visual Studio Build Tools 2022 Release - amd64`** (or *Visual Studio Community 2022 Release - amd64*)

   If the list is empty or missing yours: `Ctrl+Shift+P` > **CMake: Scan for Kits**, then **CMake: Select a Kit**.
3. Look at the blue status bar at the bottom of VS Code. Click the build-type label (it may say `[Debug]`)
   and choose **Release**. This matters: Debug builds are many times slower and give meaningless speed/strength results.
4. Click **Build** in the status bar (or press `F7`). Wait until the Output panel ends with
   `Build finished with exit code 0`.

Where the program is:

| Toolchain | Engine | Tests |
|---|---|---|
| Path A (MinGW) | `build\devi.exe` | `build\unit_tests.exe`, `build\perft_tests.exe` |
| Path B (MSVC) | `build\Release\devi.exe` | `build\Release\unit_tests.exe`, `build\Release\perft_tests.exe` |

### Option 2: from the terminal (Path A)

In VS Code open a terminal (`` Ctrl+` ``), which opens in the repo folder, and run:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

(Or press `Ctrl+Shift+B` and pick **Devi: build (Release)**; the repo ships VS Code tasks that do exactly this.)

### Option 2: from the terminal (Path B)

Open **Developer PowerShell for VS 2022**, `cd` to the repo folder, and run:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

### Optional: a faster, machine-specific build
Add `-DDEVI_NATIVE=ON` to the configure command (e.g. `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DDEVI_NATIVE=ON`).
It uses your CPU's newest instructions, so the binary may crash on other computers. Use it only for local testing, never for a build you share.

## Step 6: Run the tests

This checks that move generation is correct (perft), the board logic works, and the engine runs. It takes about 10-60 seconds.

- **VS Code:** click **Run CTest** in the status bar, or `Ctrl+Shift+P` > **CMake: Run Tests**.
- **Terminal, Path A:** `ctest --test-dir build --output-on-failure`
- **Terminal, Path B:** `ctest --test-dir build -C Release --output-on-failure`

You should see:

```
100% tests passed, 0 tests failed out of 3
```

(If `--test-dir` isn't recognised, your CMake is older than 3.20: run `cd build` then `ctest` (add `-C Release` for MSVC).)

## Step 7: Run the engine

Devi talks the UCI protocol in a terminal. From the repo folder (use the path from Step 5's table):

```powershell
.\build\devi.exe
```

Now type these lines, pressing Enter after each:

```
uci
isready
position startpos moves e2e4 e7e5
go movetime 1000
```

You should see `id name Devi 1.0 ... uciok`, then `readyok`, then a few `info depth ...` lines and finally `bestmove ...`.
Type `quit` to exit.

Quick non-interactive checks:

```powershell
.\build\devi.exe bench 6                                 # prints "505819 nodes ..." (the node count is a fingerprint of the search)
"position startpos`nperft 5`n" | .\build\devi.exe        # prints "Nodes: 4865609" (move generation check)
```

## Step 8: Play against it in a chess GUI

Devi has no board of its own; use any UCI GUI.

- **Arena** (https://www.playwitharena.de/): *Engines > Install New Engine...*, select `devi.exe`, choose type **UCI**.
  Then *File > New* to start a game, and *Engines > Manage* to choose it as opponent.
- **Cute Chess** (https://cutechess.com/): *Tools > Settings > Engines > +*, set **Command** to the full path of `devi.exe`,
  Protocol **UCI**, then *Game > New*.

Suggested engine options: `Hash` = 64 to 256 (MB). `Move Overhead` = 100 (ms) if you play over a network/online.

## Step 9: Debug in VS Code (Path A)

The repo includes ready-made debug configurations.

1. Open the **Run and Debug** panel (`Ctrl+Shift+D`).
2. Choose **Devi: debug engine (type UCI commands in the terminal)** and press `F5`. This builds a Debug copy into `build-debug\`
   and launches it under gdb.
3. Set breakpoints by clicking left of a line number (for example in `src/search.cpp`). Type UCI commands such as
   `position startpos` and `go depth 5` in the terminal panel to trigger them.
4. **Devi: debug unit tests** does the same for the test program.

Path B (MSVC): use the **Debug** button in the CMake Tools status bar (it debugs the selected target with the Visual Studio debugger).
Select the `devi` target with **CMake: Set Launch/Debug Target** first.

## Step 10: After you change or update the code

- Edit files, then **Build** again (`F7`). Only changed files are recompiled.
- After `git pull`, run **Build** and then the tests again.
- If CMake gets confused after switching compilers: `Ctrl+Shift+P` > **CMake: Delete Cache and Reconfigure**.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `g++`, `cmake` or `ninja` "is not recognized" | The PATH entry from Step 3A.5 is missing, or you didn't restart the terminal/VS Code. Re-check the entry is exactly `C:\msys64\ucrt64\bin`, then close **all** PowerShell/VS Code windows and reopen. |
| `pacman` installed nothing useful / tools are under `C:\msys64\mingw64\bin` | You used the wrong MSYS2 shell. Use **MSYS2 UCRT64** and rerun the `pacman -S` command from Step 3A.4 (or put `C:\msys64\mingw64\bin` on PATH instead if you deliberately used MINGW64). |
| CMake Tools shows no kits | Run **CMake: Scan for Kits**. Path A: confirm `g++ --version` works in a fresh PowerShell. Path B: reopen the Visual Studio Installer and check **Desktop development with C++** is installed. |
| Compile error about `std::popcount`, `<bit>` or `-std=c++20` | Your compiler is too old. Path A: rerun `pacman -Syu`. Path B: update Visual Studio Build Tools (need VS 2019 16.8 or newer; VS 2022 recommended). |
| Engine is very slow / benchmark far slower than expected | You built **Debug**. Switch the status-bar variant to **Release** and rebuild (or delete `build\` and configure again with `-DCMAKE_BUILD_TYPE=Release`). |
| `devi.exe` complains about a missing `libstdc++-6.dll` or `libwinpthread-1.dll` | It was built by hand without `-static`. Use the CMake build (it links statically), or add `-static` to your `g++` command. |
| Windows SmartScreen / Defender blocks or delays `devi.exe` | Newly built unsigned programs can trigger this. Choose *More info > Run anyway*, or add the repo folder to Defender exclusions. |
| Script cannot run in PowerShell ("running scripts is disabled") | Run it as `powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1`. |
| Weird build errors in a folder inside OneDrive / path with spaces | Move or re-clone the repo to something like `C:\dev\devi`. |
| MSVC: `ctest` says "No tests were found" | Add the configuration: `ctest --test-dir build -C Release`. |

If you're still stuck, open a GitHub issue and paste the full output of the failing command plus your Windows version and the output of `g++ --version` (or the Visual Studio version).

---

## Appendix: build without CMake

Path A, single command in PowerShell from the repo folder (PowerShell doesn't expand `*.cpp` for native programs, hence `Get-ChildItem`):

```powershell
g++ -std=c++20 -O3 -DNDEBUG -static -o devi.exe (Get-ChildItem src\*.cpp).FullName
```

In the MSYS2 UCRT64 shell the same thing is `g++ -std=c++20 -O3 -DNDEBUG -static -o devi.exe src/*.cpp`.