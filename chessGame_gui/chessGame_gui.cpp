#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>

// Basic engine headers only
#include "src/core/Board.h"
#include "src/core/Game.h"
#include "src/core/GameRules.h"
#include "src/core/Piece.h"
#include "src/core/Move.h"
#include "src/core/MoveHistory.h"
#include "src/engine/AI.h"
#include "src/engine/Evaluator.h"
#include "src/engine/TranspositionTable.h"
#include "src/utils/Types.h"

using namespace std;
void debugGameState(Game& game);
void checkAndDisplayGameOver(Game& game);
void makeAIMove();

// Store all piece textures
map<string, sf::Texture> pieceTextures;

// Global game state
Game chessGame;
AI chessAI(MEDIUM);
bool playerVsAI = false;
Color aiColor = BLACK;

// Game states
enum class GameMode {
    MAIN_MENU,
    TWO_PLAYERS,
    VS_AI_WHITE,
    VS_AI_BLACK
};

GameMode currentGameMode = GameMode::MAIN_MENU;

// Convert Piece to filename
string pieceToFilename(const Piece& piece) {
    if (piece.isEmpty()) return "";

    char colorChar = (piece.getColor() == WHITE) ? 'w' : 'b';
    char typeChar;

    switch (piece.getType()) {
    case KING:   typeChar = 'k'; break;
    case QUEEN:  typeChar = 'q'; break;
    case ROOK:   typeChar = 'r'; break;
    case BISHOP: typeChar = 'b'; break;
    case KNIGHT: typeChar = 'n'; break;
    case PAWN:   typeChar = 'p'; break;
    default:     return "";
    }

    return string(1, colorChar) + typeChar + ".png";
}

bool loadChessPieces() {
    cout << "=== LOADING CHESS PIECES ===\n";

    string pieces[] = {
        "wk.png", "wq.png", "wr.png", "wb.png", "wn.png", "wp.png",
        "bk.png", "bq.png", "br.png", "bb.png", "bn.png", "bp.png"
    };

    int loaded = 0;

    for (const auto& filename : pieces) {
        sf::Texture texture;
        string path = "assets/" + filename;  // Relative to executable

        if (texture.loadFromFile(path)) {
            pieceTextures[filename] = texture;
            loaded++;
            cout << "[OK] " << filename << "\n";
        }
        else {
            // Try alternative path
            path = "../assets/" + filename;
            if (texture.loadFromFile(path)) {
                pieceTextures[filename] = texture;
                loaded++;
                cout << "[OK] " << filename << " (from ../assets/)\n";
            }
            else {
                cout << "[FAIL] " << filename << " (NOT FOUND)\n";
            }
        }
    }

    cout << "\nLoaded " << loaded << "/12 pieces.\n";
    return loaded > 0;
}

// Convert SFML coordinates to chess Position - NO FLIP NEEDED
Position screenToPosition(float x, float y, float cellSize) {
    int col = static_cast<int>(x / cellSize);
    int row = static_cast<int>(y / cellSize);
    // NO FLIP - Engine is now correct (0=top=Black, 7=bottom=White)
    return Position(row, col);
}

// Draw piece on board - NO FLIP NEEDED
void drawPiece(sf::RenderWindow& window, const Piece& piece, int row, int col, float cellSize) {
    if (piece.isEmpty()) return;

    string filename = pieceToFilename(piece);

    if (pieceTextures.count(filename) > 0) {
        sf::Sprite pieceSprite(pieceTextures[filename]);

        sf::Vector2u texSize = pieceSprite.getTexture()->getSize();
        float scale = (cellSize * 0.8f) / static_cast<float>(max(texSize.x, texSize.y));
        pieceSprite.setScale(scale, scale);

        // NO FLIP - Draw at actual row position
        float x = static_cast<float>(col) * cellSize + (cellSize - texSize.x * scale) / 2.0f;
        float y = static_cast<float>(row) * cellSize + (cellSize - texSize.y * scale) / 2.0f;
        pieceSprite.setPosition(x, y);

        window.draw(pieceSprite);
    }
}

// Draw board squares
void drawBoard(sf::RenderWindow& window, float cellSize,
    const Position& selectedPos,
    const vector<Position>& possibleMoves) {

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            sf::RectangleShape square(sf::Vector2f(cellSize, cellSize));
            square.setPosition(static_cast<float>(col) * cellSize, static_cast<float>(row) * cellSize);

            bool isSelected = (row == selectedPos.row && col == selectedPos.col);

            bool isPossibleMove = false;
            for (const auto& movePos : possibleMoves) {
                if (movePos.row == row && movePos.col == col) {
                    isPossibleMove = true;
                    break;
                }
            }

            if (isSelected) {
                square.setFillColor(sf::Color(255, 255, 150)); // Yellow
            }
            else if (isPossibleMove) {
                square.setFillColor(sf::Color(150, 255, 150)); // Green
            }
            else if ((row + col) % 2 == 0) {
                square.setFillColor(sf::Color(240, 217, 181)); // Light
            }
            else {
                square.setFillColor(sf::Color(181, 136, 99)); // Dark
            }

            square.setOutlineThickness(1.0f);
            square.setOutlineColor(sf::Color(100, 100, 100));
            window.draw(square);
        }
    }
}

// Draw coordinates (a-h, 1-8)
void drawCoordinates(sf::RenderWindow& window, float cellSize) {
    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
        // Try alternative font path
        if (!font.loadFromFile("arial.ttf")) {
            return;
        }
    }

    // Ranks (1-8 on left) - Show 1 at top (White side), 8 at bottom (Black side)
    for (int i = 0; i < 8; i++) {
        sf::Text rank;
        rank.setFont(font);
        rank.setString(to_string(i + 1));  // ONLY CHANGE NEEDED: was (8 - i)
        rank.setCharacterSize(16);
        rank.setFillColor(sf::Color(100, 100, 100));
        rank.setPosition(5.0f, static_cast<float>(i) * cellSize + 5.0f);
        window.draw(rank);
    }

    // Files (a-h on bottom) - This stays the same
    for (int i = 0; i < 8; i++) {
        sf::Text file;
        file.setFont(font);
        file.setString(string(1, 'a' + i));
        file.setCharacterSize(16);
        file.setFillColor(sf::Color(100, 100, 100));
        file.setPosition(static_cast<float>(i) * cellSize + cellSize - 15.0f, 8.0f * cellSize - 20.0f);
        window.draw(file);
    }
}

// Draw status text
void drawStatus(sf::RenderWindow& window, float cellSize,
    const Game& game, bool pieceSelected,
    bool playerVsAI, Color aiColor) {

    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
        if (!font.loadFromFile("arial.ttf")) {
            return;
        }
    }

    sf::Text status;
    status.setFont(font);

    stringstream statusText;

    // Current player
    string playerName = (game.getCurrentPlayer() == WHITE) ? "White" : "Black";
    statusText << playerName << "'s turn";

    // Game status
    if (game.isGameOver()) {
        if (game.getWinner() != NONE) {
            string winnerName = (game.getWinner() == WHITE) ? "White" : "Black";
            statusText << " | CHECKMATE! " << winnerName << " WINS!";
        }
        else {
            statusText << " | STALEMATE! DRAW!";
        }
    }
    else if (game.isInCheck()) {
        statusText << " | " << playerName << " is in CHECK!";
    }

    // Mode
    if (playerVsAI) {
        statusText << " | Mode: vs AI";
        statusText << " (AI plays " << (aiColor == WHITE ? "White" : "Black") << ")";

        if (game.getCurrentPlayer() == aiColor) {
            statusText << " | AI thinking...";
        }
    }
    else {
        statusText << " | Mode: Two Players";
    }

    // Controls hint
    statusText << " | ESC: Menu | R: Reset | U: Undo | A: Toggle AI";

    status.setString(statusText.str());
    status.setCharacterSize(18);
    status.setFillColor(sf::Color::Black);
    status.setPosition(20.0f, 8.0f * cellSize + 15.0f);
    window.draw(status);
}

// Draw main menu WITH BORDERED BUTTONS
void drawMainMenu(sf::RenderWindow& window) {
   
    window.clear(sf::Color(30, 50, 90));
    
    // Load font
    sf::Font font;
    if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf")) {
        if (!font.loadFromFile("arial.ttf")) {
            return;
        }
    }

    // Title
    sf::Text title;
    title.setFont(font);
    title.setString("CHESS GAME");
    title.setCharacterSize(60);
    title.setFillColor(sf::Color(230, 240, 250));
    title.setStyle(sf::Text::Bold);

    sf::FloatRect titleBounds = title.getLocalBounds();
    title.setPosition((900 - titleBounds.width) / 2.0f, 100.0f);
    window.draw(title);

    // Menu buttons with borders
    vector<string> menuOptions = {
        "PLAYER VS PLAYER",
        "PLAYER VS AI (AS WHITE)",
        "PLAYER VS AI (AS BLACK)",
        "EXIT"
    };

    vector<string> menuNumbers = {
        "1.",
        "2.",
        "3.",
        "4."
    };

    float buttonWidth = 500.0f;
    float buttonHeight = 70.0f;
    float startY = 300.0f;
    float spacing = 90.0f;

    for (int i = 0; i < menuOptions.size(); i++) {
        float y = startY + i * spacing;
        float x = (900 - buttonWidth) / 2.0f;

        // Draw button background (main rectangle)
        sf::RectangleShape button(sf::Vector2f(buttonWidth, buttonHeight));
        button.setPosition(x, y);
        button.setFillColor(sf::Color(70, 70, 70, 200));  // Semi-transparent dark gray
        button.setOutlineThickness(3.0f);
        button.setOutlineColor(sf::Color(240, 217, 181));  // Chess beige border
        window.draw(button);

        // Draw inner highlight (thin border inside)
        sf::RectangleShape innerHighlight(sf::Vector2f(buttonWidth - 6, buttonHeight - 6));
        innerHighlight.setPosition(x + 3, y + 3);
        innerHighlight.setFillColor(sf::Color::Transparent);
        innerHighlight.setOutlineThickness(1.0f);
        innerHighlight.setOutlineColor(sf::Color(255, 255, 255, 100));  // Semi-transparent white
        window.draw(innerHighlight);

        // Draw number badge on left
        sf::RectangleShape numberBadge(sf::Vector2f(50.0f, buttonHeight));
        numberBadge.setPosition(x, y);
        numberBadge.setFillColor(sf::Color(181, 136, 99));  // Chess dark square color
        numberBadge.setOutlineThickness(3.0f);
        numberBadge.setOutlineColor(sf::Color(240, 217, 181));
        window.draw(numberBadge);

        // Draw number text
        sf::Text numberText;
        numberText.setFont(font);
        numberText.setString(menuNumbers[i]);
        numberText.setCharacterSize(24);
        numberText.setFillColor(sf::Color::White);
        numberText.setStyle(sf::Text::Bold);

        sf::FloatRect numberBounds = numberText.getLocalBounds();
        numberText.setPosition(
            x + (50.0f - numberBounds.width) / 2.0f,
            y + (buttonHeight - numberBounds.height) / 2.0f - 5.0f
        );
        window.draw(numberText);

        // Draw option text
        sf::Text option;
        option.setFont(font);
        option.setString(menuOptions[i]);
        option.setCharacterSize(28);
        option.setFillColor(sf::Color(230, 240, 250));
        option.setStyle(sf::Text::Bold);

        sf::FloatRect optionBounds = option.getLocalBounds();
        option.setPosition(
            x + 70.0f + (buttonWidth - 70.0f - optionBounds.width) / 2.0f,
            y + (buttonHeight - optionBounds.height) / 2.0f - 5.0f
        );
        window.draw(option);

        // Draw subtle shadow effect
        sf::RectangleShape shadow(sf::Vector2f(buttonWidth + 10, buttonHeight + 10));
        shadow.setPosition(x - 5, y - 5);
        shadow.setFillColor(sf::Color(0, 0, 0, 50));  // Very transparent black
        shadow.setOutlineThickness(0);
        window.draw(shadow);
    }

    // Instructions with better styling
    sf::Text instructions;
    instructions.setFont(font);
    instructions.setString("Click an option or press 1-4 to select");
    instructions.setCharacterSize(22);
    instructions.setFillColor(sf::Color(230, 240, 250));
    instructions.setStyle(sf::Text::Italic);

    sf::FloatRect instrBounds = instructions.getLocalBounds();
    instructions.setPosition((900 - instrBounds.width) / 2.0f, 750.0f);
    window.draw(instructions);
}

// Start game with selected mode
void startGameWithMode(GameMode mode) {
    currentGameMode = mode;

    // Reset game state
    chessGame.reset();
    chessGame.startNewGame();

    // Set AI mode based on selection
    switch (mode) {
    case GameMode::TWO_PLAYERS:
        playerVsAI = false;
        cout << "Starting: Two Players Mode\n";
        break;
    case GameMode::VS_AI_WHITE:
        playerVsAI = true;
        aiColor = BLACK;  // AI plays Black
        cout << "Starting: Player (White) vs AI (Black)\n";
        break;
    case GameMode::VS_AI_BLACK:
        playerVsAI = true;
        aiColor = WHITE;  // AI plays White
        cout << "Starting: Player (Black) vs AI (White)\n";

        // If AI goes first, make its move
        if (chessGame.getCurrentPlayer() == aiColor) {
            cout << "AI (White) makes first move...\n";
            // AI move will be handled in game loop
        }
        break;
    default:
        break;
    }

    debugGameState(chessGame);
}

// Debug function to show current game state
void debugGameState(Game& game) {
    cout << "\n=== DEBUG GAME STATE ===\n";
    cout << "Current player: " << (game.getCurrentPlayer() == WHITE ? "White" : "Black") << "\n";
    cout << "Game over: " << (game.isGameOver() ? "YES" : "NO") << "\n";
    cout << "In check: " << (game.isInCheck() ? "YES" : "NO") << "\n";

    if (game.isGameOver()) {
        cout << "Winner: ";
        if (game.getWinner() == WHITE) cout << "White";
        else if (game.getWinner() == BLACK) cout << "Black";
        else cout << "None (stalemate)";
        cout << "\n";
    }

    // Debug board setup
    cout << "Board setup (engine coordinates - 0=top):\n";
    for (int row = 0; row < 8; row++) {
        cout << "Row " << row << ": ";
        for (int col = 0; col < 8; col++) {
            Piece piece = game.getPieceAt(Position(row, col));
            if (piece.isEmpty()) {
                cout << ". ";
            }
            else {
                cout << (piece.getColor() == WHITE ? "W" : "B");
                switch (piece.getType()) {
                case KING: cout << "K"; break;
                case QUEEN: cout << "Q"; break;
                case ROOK: cout << "R"; break;
                case BISHOP: cout << "B"; break;
                case KNIGHT: cout << "N"; break;
                case PAWN: cout << "P"; break;
                default: cout << "?";
                }
                cout << " ";
            }
        }
        cout << "\n";
    }

    cout << "=== END DEBUG ===\n\n";
}

// Helper function to check and display game over status
void checkAndDisplayGameOver(Game& game) {
    if (game.isGameOver()) {
        if (game.getWinner() != NONE) {
            string winnerName = (game.getWinner() == WHITE) ? "White" : "Black";
            cout << "\n";
            cout << "================================================\n";
            cout << "               *** GAME OVER ***               \n";
            cout << "================================================\n";
            cout << "                                                \n";
            cout << "               *** CHECKMATE! ***              \n";
            cout << "                                                \n";
            cout << "               " << winnerName << " WINS!               \n";
            cout << "                                                \n";
            cout << "       Press R to reset the game               \n";
            cout << "================================================\n";
            cout << "\n";
        }
        else {
            cout << "\n";
            cout << "================================================\n";
            cout << "               *** GAME OVER ***               \n";
            cout << "================================================\n";
            cout << "                                                \n";
            cout << "             *** STALEMATE! ***                \n";
            cout << "                                                \n";
            cout << "                  DRAW!                        \n";
            cout << "                                                \n";
            cout << "       Press R to reset the game               \n";
            cout << "================================================\n";
            cout << "\n";
        }
    }
    // Add immediate check feedback
    else if (game.isInCheck()) {
        string playerName = (game.getCurrentPlayer() == WHITE) ? "White" : "Black";
        cout << "\n[ALERT] " << playerName << " is in CHECK!\n";
    }
}

// AI makes a move
void makeAIMove() {
    if (!playerVsAI || chessGame.isGameOver()) return;

    if (chessGame.getCurrentPlayer() == aiColor) {
        cout << "\n[AI] is thinking...\n";
        Move aiMove = chessAI.getBestMove(chessGame, aiColor);

        if (aiMove.isValid()) {
            chessGame.makeMove(aiMove);
            cout << "AI moved: " << aiMove.toAlgebraicNotation() << "\n";

            // Debug output
            debugGameState(chessGame);

            // Check for game over after AI move
            checkAndDisplayGameOver(chessGame);
        }
    }
}

int main() {
    cout << "=== CHESS GAME WITH COMPLETE ENGINE ===\n\n";

    if (!loadChessPieces()) {
        cout << "WARNING: Some pieces missing. Game will continue.\n";
    }

    // Create window
    sf::RenderWindow window(sf::VideoMode(900, 900), "Chess Game - Complete Engine");

    // Cell size
    float cellSize = 100.0f;

    // Game state variables
    Position selectedPos(-1, -1);
    vector<Position> possibleMoves;
    bool pieceSelected = false;

    // Game loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Handle events based on current game mode
            if (currentGameMode == GameMode::MAIN_MENU) {
                // Main menu events
                if (event.type == sf::Event::KeyPressed) {
                    // Number keys to select menu option
                    if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num4) {
                        int selection = event.key.code - sf::Keyboard::Num1;
                        switch (selection) {
                        case 0: startGameWithMode(GameMode::TWO_PLAYERS); break;
                        case 1: startGameWithMode(GameMode::VS_AI_WHITE); break;
                        case 2: startGameWithMode(GameMode::VS_AI_BLACK); break;
                        case 3: window.close(); break;
                        }
                    }
                    // ESC also exits from menu
                    if (event.key.code == sf::Keyboard::Escape) {
                        window.close();
                    }
                }

                // Mouse click on menu options
                if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        float mouseX = static_cast<float>(event.mouseButton.x);
                        float mouseY = static_cast<float>(event.mouseButton.y);

                        // Button dimensions
                        float buttonWidth = 500.0f;
                        float buttonHeight = 70.0f;
                        float startY = 300.0f;
                        float spacing = 90.0f;
                        float x = (900 - buttonWidth) / 2.0f;

                        // Check which button was clicked
                        for (int i = 0; i < 4; i++) {
                            float buttonTop = startY + i * spacing;
                            float buttonBottom = buttonTop + buttonHeight;

                            if (mouseY >= buttonTop && mouseY <= buttonBottom &&
                                mouseX >= x && mouseX <= x + buttonWidth) {

                                switch (i) {
                                case 0: startGameWithMode(GameMode::TWO_PLAYERS); break;
                                case 1: startGameWithMode(GameMode::VS_AI_WHITE); break;
                                case 2: startGameWithMode(GameMode::VS_AI_BLACK); break;
                                case 3: window.close(); break;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            else {
                // In-game events (when playing)
                if (event.type == sf::Event::KeyPressed) {
                    // ESC returns to main menu
                    if (event.key.code == sf::Keyboard::Escape) {
                        currentGameMode = GameMode::MAIN_MENU;
                        selectedPos = Position(-1, -1);
                        possibleMoves.clear();
                        pieceSelected = false;
                        cout << "\nReturning to main menu\n";
                        continue;
                    }

                    // R to reset game
                    if (event.key.code == sf::Keyboard::R) {
                        chessGame.reset();
                        chessGame.startNewGame();
                        selectedPos = Position(-1, -1);
                        possibleMoves.clear();
                        pieceSelected = false;
                        cout << "\n==========================================\n";
                        cout << "           GAME RESET\n";
                        cout << "==========================================\n\n";
                        debugGameState(chessGame);
                    }

                    // U to undo move
                    if (event.key.code == sf::Keyboard::U) {
                        if (chessGame.canUndo()) {
                            chessGame.undoMove();
                            selectedPos = Position(-1, -1);
                            possibleMoves.clear();
                            pieceSelected = false;
                            cout << "Move undone\n";
                            debugGameState(chessGame);
                        }
                    }

                    // A to toggle AI mode (only in two players mode)
                    if (event.key.code == sf::Keyboard::A && currentGameMode == GameMode::TWO_PLAYERS) {
                        playerVsAI = !playerVsAI;
                        if (playerVsAI) {
                            aiColor = (chessGame.getCurrentPlayer() == WHITE) ? BLACK : WHITE;
                            cout << "AI mode enabled (AI plays " << (aiColor == WHITE ? "White" : "Black") << ")\n";
                            if (chessGame.getCurrentPlayer() == aiColor) {
                                makeAIMove();
                            }
                        }
                        else {
                            cout << "Two-player mode enabled\n";
                        }
                    }

                    // Space to show game status
                    if (event.key.code == sf::Keyboard::Space) {
                        cout << "Game status: " << chessGame.getGameStatus() << "\n";
                        debugGameState(chessGame);
                    }

                    // B to print current board
                    if (event.key.code == sf::Keyboard::B) {
                        debugGameState(chessGame);
                    }
                }

                // Mouse click handling for game
                if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        float mouseX = static_cast<float>(event.mouseButton.x);
                        float mouseY = static_cast<float>(event.mouseButton.y);

                        // Check if click is on board
                        if (mouseX < 8.0f * cellSize && mouseY < 8.0f * cellSize) {
                            Position clickedPos = screenToPosition(mouseX, mouseY, cellSize);

                            if (clickedPos.row >= 0 && clickedPos.row < 8 &&
                                clickedPos.col >= 0 && clickedPos.col < 8) {

                                cout << "Clicked screen: (" << mouseY / cellSize << "," << mouseX / cellSize << ")";
                                cout << " -> Board position: (" << clickedPos.row << "," << clickedPos.col << ")\n";

                                // If playing against AI and it's AI's turn, ignore clicks
                                if (playerVsAI && chessGame.getCurrentPlayer() == aiColor) {
                                    cout << "Wait for AI to move!\n";
                                    continue;
                                }

                                // If no piece is selected, try to select one
                                if (!pieceSelected) {
                                    Piece clickedPiece = chessGame.getPieceAt(clickedPos);

                                    cout << "Piece at position: ";
                                    if (clickedPiece.isEmpty()) {
                                        cout << "Empty\n";
                                    }
                                    else {
                                        cout << (clickedPiece.getColor() == WHITE ? "White " : "Black ");
                                        cout << "piece, Current turn: ";
                                        cout << (chessGame.getCurrentPlayer() == WHITE ? "White" : "Black") << "\n";
                                    }

                                    // Check if there's a piece and it's current player's turn
                                    if (!clickedPiece.isEmpty() &&
                                        clickedPiece.getColor() == chessGame.getCurrentPlayer()) {

                                        selectedPos = clickedPos;
                                        pieceSelected = true;

                                        // Get valid moves from engine
                                        vector<Move> validMoves = chessGame.getValidMoves(clickedPos);
                                        possibleMoves.clear();

                                        for (const auto& move : validMoves) {
                                            possibleMoves.push_back(move.getTo());
                                        }

                                        cout << "Selected piece at (" << selectedPos.row << ","
                                            << selectedPos.col << ")\n";
                                        cout << "Valid moves: " << validMoves.size() << "\n";
                                    }
                                    else if (!clickedPiece.isEmpty()) {
                                        cout << "Wrong player's piece! It's ";
                                        cout << (chessGame.getCurrentPlayer() == WHITE ? "White" : "Black");
                                        cout << "'s turn.\n";
                                    }
                                }
                                // If a piece is already selected
                                else {
                                    // Check if clicked on a valid move destination
                                    bool isValidDestination = false;
                                    for (const auto& movePos : possibleMoves) {
                                        if (movePos.row == clickedPos.row && movePos.col == clickedPos.col) {
                                            isValidDestination = true;
                                            break;
                                        }
                                    }

                                    if (isValidDestination) {
                                        // Make the move using engine
                                        if (chessGame.makeMove(selectedPos, clickedPos)) {
                                            cout << "Move successful!\n";

                                            // Debug output
                                            debugGameState(chessGame);

                                            // Check for game over after player move
                                            checkAndDisplayGameOver(chessGame);

                                            // If playing against AI, let AI move
                                            if (playerVsAI && !chessGame.isGameOver()) {
                                                makeAIMove();
                                            }
                                        }
                                        else {
                                            cout << "Move failed!\n";
                                        }
                                    }
                                    else {
                                        cout << "Not a valid move destination\n";
                                    }

                                    // Reset selection
                                    selectedPos = Position(-1, -1);
                                    possibleMoves.clear();
                                    pieceSelected = false;
                                }
                            }
                        }
                    }
                }
            }
        }

        // Clear window
        window.clear(sf::Color(220, 220, 220));

        // Draw based on current game mode
        if (currentGameMode == GameMode::MAIN_MENU) {
            drawMainMenu(window);
        }
        else {
            // Draw game board
            drawBoard(window, cellSize, selectedPos, possibleMoves);

            // Draw pieces from engine board
            const Board& board = chessGame.getBoard();
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    Piece piece = board.getPiece(row, col);
                    if (!piece.isEmpty()) {
                        drawPiece(window, piece, row, col, cellSize);
                    }
                }
            }

            // Draw coordinates and status
            drawCoordinates(window, cellSize);
            drawStatus(window, cellSize, chessGame, pieceSelected, playerVsAI, aiColor);
        }

        // Display everything
        window.display();
    }

    cout << "\n=== GAME ENDED ===\n";
    return 0;
}