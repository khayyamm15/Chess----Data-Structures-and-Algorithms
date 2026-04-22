# ♟️ Chess Game GUI

A fully-featured chess game built in **C++** with **SFML** graphics, featuring a custom chess engine with AI opponent powered by minimax search with alpha-beta pruning.

![C++](https://img.shields.io/badge/C%2B%2B-17-blue?logo=cplusplus)
![SFML](https://img.shields.io/badge/SFML-2.5+-green?logo=sfml)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey?logo=windows)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## ✨ Features

### 🎮 Game Modes
- **Player vs Player** — local two-player mode on the same machine
- **Player vs AI (as White)** — play White against the computer
- **Player vs AI (as Black)** — play Black against the computer

### 🧠 Chess Engine
- **Minimax algorithm** with configurable search depth (2–6 plies)
- **Alpha-beta pruning** for efficient move tree exploration
- **Transposition table** to cache and reuse evaluated board positions
- **Move ordering** — captures, promotions, castling, and central moves evaluated first for better pruning
- **Three AI difficulty levels:**

  | Difficulty | Search Depth | Transposition Table | Advanced Eval | Randomness |
  |------------|:------------:|:-------------------:|:-------------:|:----------:|
  | Easy       | 2            | ❌                  | ❌            | 30%        |
  | Medium     | 4            | ✅                  | ✅            | 10%        |
  | Hard       | 6            | ✅                  | ✅            | 0%         |

### ♔ Complete Chess Rules
- All standard piece movement (King, Queen, Rook, Bishop, Knight, Pawn)
- **Castling** (kingside & queenside)
- **En passant** captures
- **Pawn promotion**
- **Check & checkmate** detection
- **Stalemate** detection
- **Draw by insufficient material** detection
- Full **move validation** preventing illegal moves (including self-check)

### 🎨 GUI Features
- Rendered 8×8 chessboard with classic light/dark square colors
- PNG piece sprites for all 12 pieces
- **Visual move highlighting** — selected piece shown in yellow, valid moves in green
- Board coordinates (files a–h, ranks 1–8)
- Real-time status bar showing current turn, game state, check alerts, and controls
- Styled main menu with bordered buttons and keyboard/mouse navigation

### 🔧 Quality-of-Life
- **Undo moves** (`U` key)
- **Reset game** (`R` key)
- **Toggle AI** mid-game (`A` key in two-player mode)
- **Debug output** in console — board state, AI thinking statistics, move validation
- Return to main menu anytime (`ESC`)

---

## 📁 Project Structure

```
chessGame_gui/
├── chessGame_gui.sln              # Visual Studio solution file
├── chessGame_gui/
│   ├── chessGame_gui.cpp          # Main application (GUI, game loop, rendering)
│   ├── chessGame_gui.vcxproj      # Visual Studio project file
│   ├── assets/                    # Piece sprite PNGs
│   │   ├── wk.png, wq.png, wr.png, wb.png, wn.png, wp.png  # White pieces
│   │   └── bk.png, bq.png, br.png, bb.png, bn.png, bp.png  # Black pieces
│   └── src/
│       ├── core/                  # Core chess logic
│       │   ├── Board.h / Board.cpp         # Board representation & piece management
│       │   ├── Game.h / Game.cpp           # Game state, turn management, move execution
│       │   ├── GameRules.h / GameRules.cpp # Move validation, check/mate/stalemate logic
│       │   ├── Move.h / Move.cpp           # Move representation (incl. special moves)
│       │   ├── MoveHistory.h / MoveHistory.cpp # Doubly-linked list move history
│       │   └── Piece.h / Piece.cpp         # Piece type, color, and state
│       ├── engine/                # AI engine
│       │   ├── AI.h / AI.cpp               # Minimax search with alpha-beta pruning
│       │   ├── Evaluator.h / Evaluator.cpp # Board evaluation (material, position, mobility, king safety, pawn structure)
│       │   └── TranspositionTable.h / TranspositionTable.cpp  # Position caching
│       └── utils/
│           └── Types.h            # Enums (PieceType, Color) and Position struct
```

---

## 🏗️ Architecture

```
┌──────────────────────────────────────────────────┐
│                  chessGame_gui.cpp                │
│         (SFML Window, Rendering, Input)           │
├──────────────────────────────────────────────────┤
│                                                    │
│   ┌──────────┐    ┌───────────┐    ┌───────────┐  │
│   │   Game    │◄──►│ GameRules │    │    AI      │  │
│   └────┬─────┘    └───────────┘    └─────┬──────┘  │
│        │                                  │         │
│   ┌────▼─────┐                    ┌───────▼──────┐ │
│   │  Board   │                    │  Evaluator   │ │
│   └────┬─────┘                    └──────────────┘ │
│        │                                            │
│   ┌────▼─────┐    ┌───────────┐                    │
│   │  Piece   │    │MoveHistory│ (Doubly-Linked)    │
│   └──────────┘    └───────────┘                    │
│                                                    │
│   ┌──────────────────────────────────────────┐     │
│   │  Types.h  (Position, PieceType, Color)   │     │
│   └──────────────────────────────────────────┘     │
└──────────────────────────────────────────────────┘
```

- **Game** orchestrates the overall game state, delegates rule checking to **GameRules**, and manages board history for undo.
- **GameRules** is a static utility class handling all move validation, move generation, and game-end detection.
- **AI** uses iterative minimax with alpha-beta pruning, consulting **Evaluator** for leaf-node scoring and **TranspositionTable** for caching.
- **Evaluator** combines five heuristics: material balance, positional bonuses, piece mobility, king safety, and pawn structure.
- **MoveHistory** uses a custom doubly-linked list for move traversal and undo support.

---

## 🔨 Prerequisites

- **Visual Studio 2022** (or later) with the C++ Desktop Development workload
- **SFML 2.5+** — [Download here](https://www.sfml-dev.org/download.php)
- Windows OS (uses `C:/Windows/Fonts/arial.ttf` for in-game text)

---

## 🚀 Build & Run

### Using Visual Studio

1. **Clone the repository:**
   ```bash
   git clone https://github.com/<your-username>/chessGame_gui.git
   cd chessGame_gui
   ```

2. **Set up SFML:**
   - Download SFML for Visual C++ from the [official site](https://www.sfml-dev.org/download.php).
   - In the project properties:
     - **C/C++ → Additional Include Directories** → add path to SFML's `include/` folder
     - **Linker → Additional Library Directories** → add path to SFML's `lib/` folder
     - **Linker → Input → Additional Dependencies** → add:
       ```
       sfml-graphics.lib
       sfml-window.lib
       sfml-system.lib
       ```
   - Copy the SFML `.dll` files to the output directory (or add them to your system PATH).

3. **Open `chessGame_gui.sln`** in Visual Studio.

4. **Build & Run** (`F5` or `Ctrl+F5`).

> **Note:** Ensure the `assets/` folder containing the piece PNGs is in the working directory of the executable.

---

## 🎮 Controls

| Key / Action     | Description                                     |
|------------------|-------------------------------------------------|
| **Mouse Click**  | Select a piece, then click a highlighted square to move |
| **R**            | Reset the game                                  |
| **U**            | Undo the last move                              |
| **A**            | Toggle AI on/off (in two-player mode)           |
| **B**            | Print board state to console                    |
| **Space**        | Print game status to console                    |
| **ESC**          | Return to main menu (or exit from menu)         |
| **1–4**          | Select menu options from the main menu          |

---

## 🧪 AI Evaluation Heuristics

The board evaluator scores positions using a weighted combination of:

1. **Material** — standard piece values (♙100, ♞320, ♝330, ♜500, ♛900, ♚20000)
2. **Positional Bonuses** — rewards pieces on central/advanced squares
3. **Mobility** — number of reachable squares per piece
4. **King Safety** — penalizes exposed kings, rewards castled positions
5. **Pawn Structure** — penalizes doubled pawns, rewards advanced pawns

---

## 📝 License

This project is open source. Feel free to use, modify, and distribute.

---

## 🤝 Contributing

Contributions are welcome! Feel free to open issues or submit pull requests for:
- New features (e.g., clock/timer, move notation panel, online multiplayer)
- Bug fixes
- AI improvements (deeper search, opening book, endgame tables)
- UI/UX enhancements
