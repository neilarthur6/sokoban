#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <windows.h>
#include "TUI.h"

#define SIZE 12  // Taille du cadre (doit être pair pour le centrage)
#define WIDTH SIZE
#define HEIGHT SIZE
#define MAX_TILES 50

// Niveaux de difficulté
typedef enum {
    DIFFICULTY_NORMAL,   // Une tuile tous les 3 mouvements, valeurs 2-4
    DIFFICULTY_HARD,     // Une tuile tous les 2 mouvements, valeurs 2-8
    DIFFICULTY_EXPERT    // Une tuile à chaque mouvement, valeurs 2-16
} Difficulty;

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position pos;
    int value;
} Tile;

typedef struct {
    Position player;
    Tile tiles[MAX_TILES];
    int numTiles;
    int score;
    int moveCount;
    Difficulty difficulty;
} GameState;

GameState gameState;

// Trouve une position vide aléatoire
Position getRandomEmptyPosition() {
    Position pos;
    int found = 0;
    
    while(!found) {
        pos.x = 1 + rand() % (WIDTH - 2);
        pos.y = 1 + rand() % (HEIGHT - 2);
        
        // Vérifier si la position est vide
        found = 1;
        if(pos.x == gameState.player.x && pos.y == gameState.player.y) {
            found = 0;
            continue;
        }
        
        for(int i = 0; i < gameState.numTiles; i++) {
            if(gameState.tiles[i].pos.x == pos.x && gameState.tiles[i].pos.y == pos.y) {
                found = 0;
                break;
            }
        }
    }
    
    return pos;
}

// Génère une valeur aléatoire selon le niveau de difficulté
int getRandomValue() {
    int maxPower;
    switch(gameState.difficulty) {
        case DIFFICULTY_NORMAL:
            maxPower = 2;  // Jusqu'à 4
            break;
        case DIFFICULTY_HARD:
            maxPower = 3;  // Jusqu'à 8
            break;
        case DIFFICULTY_EXPERT:
            maxPower = 4;  // Jusqu'à 16
            break;
    }
    
    int power = 1 + rand() % maxPower;
    return 1 << power;  // 2^power
}

// Ajoute une nouvelle tuile selon le niveau de difficulté
void addNewTile() {
    if(gameState.numTiles >= MAX_TILES) return;
    
    // Fréquence d'apparition selon la difficulté
    int frequency;
    switch(gameState.difficulty) {
        case DIFFICULTY_NORMAL:
            frequency = 3;
            break;
        case DIFFICULTY_HARD:
            frequency = 2;
            break;
        case DIFFICULTY_EXPERT:
            frequency = 1;
            break;
    }
    
    if(gameState.moveCount % frequency != 0) return;
    
    Position pos = getRandomEmptyPosition();
    gameState.tiles[gameState.numTiles].pos = pos;
    gameState.tiles[gameState.numTiles].value = getRandomValue();
    gameState.numTiles++;
}

// Initialise un nouveau niveau avec la difficulté spécifiée
void initLevel(Difficulty difficulty) {
    srand(time(NULL));
    
    gameState.player.x = WIDTH/2;
    gameState.player.y = HEIGHT/2;
    
    gameState.numTiles = 0;
    gameState.score = 0;
    gameState.moveCount = 0;
    gameState.difficulty = difficulty;
    
    // Ajouter deux tuiles initiales
    addNewTile();
    addNewTile();
}

// Vérifie si une position contient une tuile
int getTileAt(int x, int y) {
    for(int i = 0; i < gameState.numTiles; i++) {
        if(gameState.tiles[i].pos.x == x && gameState.tiles[i].pos.y == y) {
            return i;
        }
    }
    return -1;
}

// Fusionne deux tuiles si elles ont la même valeur
void tryMergeTiles(int tileIndex, int x, int y) {
    int targetTileIndex = getTileAt(x, y);
    if(targetTileIndex != -1 && 
       gameState.tiles[tileIndex].value == gameState.tiles[targetTileIndex].value) {
        // Fusion : doubler la valeur et supprimer l'autre tuile
        gameState.tiles[tileIndex].value *= 2;
        gameState.score += gameState.tiles[tileIndex].value;
        
        // Supprimer la tuile fusionnée en la remplaçant par la dernière
        gameState.tiles[targetTileIndex] = gameState.tiles[gameState.numTiles - 1];
        gameState.numTiles--;
    }
}

// Déplace une tuile si possible
int moveTile(int tileIndex, int dx, int dy) {
    int newX = gameState.tiles[tileIndex].pos.x + dx;
    int newY = gameState.tiles[tileIndex].pos.y + dy;
    
    // Vérifier les limites
    if(newX <= 0 || newX >= WIDTH-1 || newY <= 0 || newY >= HEIGHT-1) {
        return 0;
    }
    
    // Vérifier s'il y a une autre tuile
    int targetTileIndex = getTileAt(newX, newY);
    if(targetTileIndex != -1) {
        tryMergeTiles(tileIndex, newX, newY);
        return 1;
    }
    
    // Déplacer la tuile
    gameState.tiles[tileIndex].pos.x = newX;
    gameState.tiles[tileIndex].pos.y = newY;
    return 1;
}

// Déplace le joueur dans la direction spécifiée
int movePlayer(int dx, int dy) {
    int newX = gameState.player.x + dx;
    int newY = gameState.player.y + dy;
    
    // Vérifier les limites
    if(newX <= 0 || newX >= WIDTH-1 || newY <= 0 || newY >= HEIGHT-1) {
        return 0;
    }
    
    // Vérifier s'il y a une tuile
    int tileIndex = getTileAt(newX, newY);
    if(tileIndex != -1) {
        // Essayer de pousser la tuile
        if(moveTile(tileIndex, dx, dy)) {
            gameState.player.x = newX;
            gameState.player.y = newY;
            return 1;
        }
        return 0;  // Si on ne peut pas pousser la tuile, le mouvement est impossible
    } else {
        gameState.player.x = newX;
        gameState.player.y = newY;
        return 1;
    }
}

// Téléporte le joueur à une position aléatoire vide
int teleportPlayer() {
    Position newPos = getRandomEmptyPosition();
    gameState.player = newPos;
    return 1;
}

// Vérifie si le jeu est gagné
int isGameWon() {
    return gameState.numTiles == 1;
}

// Vérifie si un mouvement est possible pour une tuile
int canMoveTile(int tileIndex, int dx, int dy) {
    int newX = gameState.tiles[tileIndex].pos.x + dx;
    int newY = gameState.tiles[tileIndex].pos.y + dy;
    
    if(newX < 0 || newX >= WIDTH || newY < 0 || newY >= HEIGHT) {
        return 0;
    }
    
    int targetTileIndex = getTileAt(newX, newY);
    if(targetTileIndex != -1) {
        return gameState.tiles[tileIndex].value == gameState.tiles[targetTileIndex].value;
    }
    
    return 1;
}

// Vérifie si des mouvements sont encore possibles
int hasMovesLeft() {
    // Vérifier pour chaque tuile
    for(int i = 0; i < gameState.numTiles; i++) {
        // Vérifier les 4 directions
        if(canMoveTile(i, 0, -1) || // haut
           canMoveTile(i, 0, 1) ||  // bas
           canMoveTile(i, -1, 0) || // gauche
           canMoveTile(i, 1, 0)) {  // droite
            return 1;
        }
    }
    return 0;
}

void drawBoard() {
    clearWindow();
    
    // Remplir l'espace de jeu avec des points
    for(int i = 0; i < HEIGHT; i++) {
        for(int j = 0; j < WIDTH; j++) {
            printAt(i, j, ".");
        }
    }
    
    // Dessiner les tuiles
    for(int i = 0; i < gameState.numTiles; i++) {
        printAt(gameState.tiles[i].pos.y, gameState.tiles[i].pos.x, "%d", 
               gameState.tiles[i].value);
    }
    
    // Dessiner le joueur
    printAt(gameState.player.y, gameState.player.x, "P");
    
    // Afficher le score, le nombre de tuiles et la difficulté
    const char* difficultyStr;
    switch(gameState.difficulty) {
        case DIFFICULTY_NORMAL:
            difficultyStr = "Normal";
            break;
        case DIFFICULTY_HARD:
            difficultyStr = "Difficile";
            break;
        case DIFFICULTY_EXPERT:
            difficultyStr = "Expert";
            break;
    }
    
    printAt(HEIGHT, 0, "Score: %d  Tuiles: %d  Niveau: %s", 
            gameState.score, gameState.numTiles, difficultyStr);
    
    refreshWindow();
}

void displayMenu() {
    printf("\nChoisissez le niveau de difficulte:\n");
    printf("1. Normal   (tuiles 2-4, frequence: 3 mouvements)\n");
    printf("2. Difficile (tuiles 2-8, frequence: 2 mouvements)\n");
    printf("3. Expert    (tuiles 2-16, frequence: 1 mouvement)\n");
    printf("Votre choix (1-3): ");
}

int main() {
    // Afficher le menu et obtenir le choix du joueur
    displayMenu();
    
    int choice;
    scanf("%d", &choice);
    
    Difficulty difficulty;
    switch(choice) {
        case 1:
            difficulty = DIFFICULTY_NORMAL;
            break;
        case 2:
            difficulty = DIFFICULTY_HARD;
            break;
        case 3:
            difficulty = DIFFICULTY_EXPERT;
            break;
        default:
            printf("Choix invalide. Utilisation du niveau normal.\n");
            difficulty = DIFFICULTY_NORMAL;
    }
    
    initTUI();
    initLevel(difficulty);
    
    key input;
    int running = 1;
    int moved;
    
    while(running) {
        drawBoard();
        
        // Vérifier s'il reste des mouvements possibles
        if(!hasMovesLeft()) {
            printAt(HEIGHT/2, WIDTH/2 - 15, "Game Over! Plus de mouvements possibles. Score: %d", gameState.score);
            refreshWindow();
            Sleep(2000);
            running = 0;
            continue;
        }
        
        input = getInput();
        moved = 0;
        
        switch(input) {
            case UP:
                moved = movePlayer(0, -1);
                break;
            case DOWN:
                moved = movePlayer(0, 1);
                break;
            case LEFT:
                moved = movePlayer(-1, 0);
                break;
            case RIGHT:
                moved = movePlayer(1, 0);
                break;
            case T:
                moved = teleportPlayer();
                break;
            case Q:
                running = 0;
                break;
            default:
                break;
        }
        
        if(moved) {
            gameState.moveCount++;
            addNewTile();
        }
        
        if(isGameWon()) {
            printAt(HEIGHT/2, WIDTH/2 - 10, "Victoire! Score: %d", gameState.score);
            refreshWindow();
            Sleep(2000);
            running = 0;
        }
    }
    
    endTUI();
    printf("\nMerci d'avoir joue! Score final: %d\n", gameState.score);
    return 0;
}
