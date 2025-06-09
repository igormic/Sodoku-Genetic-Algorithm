#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define MAX_N 16

// Parametry algorytmu genetycznego
#define POPULATION_SIZE 1000
#define NUM_GENERATIONS 10000
#define MUTATION_RATE 0.15
#define CROSSOVER_RATE 0.8
#define ELITISM_RATE 0.2

int N;
int SRN;
int **board;
int **fixed;
int **solution;

// Struktura osobnika (rozwiązania Sudoku)
typedef struct {
    int **grid;
    int fitness;
} Individual;

// Alokacja pamięci
void allocBoard() {
    board = malloc(N * sizeof(int *));
    fixed = malloc(N * sizeof(int *));
    solution = malloc(N * sizeof(int *));
    for (int i = 0; i < N; i++) {
        board[i] = calloc(N, sizeof(int));
        fixed[i] = calloc(N, sizeof(int));
        solution[i] = calloc(N, sizeof(int));
    }
}

// Zwalnianie pamięci
void freeBoard() {
    for (int i = 0; i < N; i++) {
        free(board[i]);
        free(fixed[i]);
        free(solution[i]);
    }
    free(board);
    free(fixed);
    free(solution);
}

// Alokacja pamięci dla osobnika
Individual* createIndividual() {
    Individual *ind = malloc(sizeof(Individual));
    ind->grid = malloc(N * sizeof(int *));
    for (int i = 0; i < N; i++) {
        ind->grid[i] = malloc(N * sizeof(int));
    }
    return ind;
}

// Zwolnienie pamięci osobnika
void freeIndividual(Individual *ind) {
    for (int i = 0; i < N; i++) {
        free(ind->grid[i]);
    }
    free(ind->grid);
    free(ind);
}

// Kopiowanie planszy
void copyGrid(int **source, int **dest) {
    for (int i = 0; i < N; i++) {
        memcpy(dest[i], source[i], N * sizeof(int));
    }
}

// Sprawdza czy dane są poprawne według zasad sudoku
int isSafe(int row, int col, int num, int **grid) {
    for (int x = 0; x < N; x++)
        if (grid[row][x] == num || grid[x][col] == num)
            return 0;

    int startRow = row - row % SRN, startCol = col - col % SRN;
    for (int i = 0; i < SRN; i++)
        for (int j = 0; j < SRN; j++)
            if (grid[i + startRow][j + startCol] == num)
                return 0;

    return 1;
}

// Wypełnia planszę sudoku
int fillBoard(int row, int col, int **grid) {
    if (row == N - 1 && col == N)
        return 1;
    if (col == N) {
        row++;
        col = 0;
    }

    if (grid[row][col] != 0)
        return fillBoard(row, col + 1, grid);

    int nums[N];
    for (int i = 0; i < N; i++)
        nums[i] = i + 1;

    for (int i = N - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = nums[i];
        nums[i] = nums[j];
        nums[j] = temp;
    }

    for (int i = 0; i < N; i++) {
        int num = nums[i];
        if (isSafe(row, col, num, grid)) {
            grid[row][col] = num;
            if (fillBoard(row, col + 1, grid))
                return 1;
            grid[row][col] = 0;
        }
    }

    return 0;
}

// Liczy konflikty (duplikaty) w wierszach i kolumnach
int calculate_conflicts(int **grid) {
    int conflicts = 0;
    
    // Konflikty w wierszach
for (int i = 0; i < N; i++) {
    int count[N+1];
    for (int k = 0; k <= N; k++) count[k] = 0;
        for (int j = 0; j < N; j++) {
            count[grid[i][j]]++;
        }
        for (int k = 1; k <= N; k++) {
            if (count[k] > 1) conflicts += count[k] - 1;
        }
    }
    
    // Konflikty w kolumnach
for (int j = 0; j < N; j++) {
    int count[N+1];
    for (int k = 0; k <= N; k++) count[k] = 0;
        for (int i = 0; i < N; i++) {
            count[grid[i][j]]++;
        }
        for (int k = 1; k <= N; k++) {
            if (count[k] > 1) conflicts += count[k] - 1;
        }
    }
    
    // Konflikty w blokach 3x3
for (int block_row = 0; block_row < SRN; block_row++) {
    for (int block_col = 0; block_col < SRN; block_col++) {
        int count[N+1];
        for (int k = 0; k <= N; k++) count[k] = 0;
            for (int i = 0; i < SRN; i++) {
                for (int j = 0; j < SRN; j++) {
                    int val = grid[block_row*SRN + i][block_col*SRN + j];
                    count[val]++;
                }
            }
            for (int k = 1; k <= N; k++) {
                if (count[k] > 1) conflicts += count[k] - 1;
            }
        }
    }
    
    return conflicts;
}

// Inicjalizuje populację
void initializePopulation(Individual **population) {
    for (int i = 0; i < POPULATION_SIZE; i++) {
        population[i] = createIndividual();
        copyGrid(board, population[i]->grid);
        
        // Wypełnij puste komórki losowymi wartościami, ale zgodnymi z blokami 3x3
        for (int block_row = 0; block_row < SRN; block_row++) {
            for (int block_col = 0; block_col < SRN; block_col++) {
                int used[N+1];
                for (int u = 0; u <= N; u++) used[u] = 0;
                
                // Zaznacz już używane liczby w bloku
                for (int r = 0; r < SRN; r++) {
                    for (int c = 0; c < SRN; c++) {
                        int val = population[i]->grid[block_row*SRN + r][block_col*SRN + c];
                        if (val != 0) used[val] = 1;
                    }
                }
                
                // Wypełnij puste komórki unikalnymi wartościami w bloku
                for (int r = 0; r < SRN; r++) {
                    for (int c = 0; c < SRN; c++) {
                        if (population[i]->grid[block_row*SRN + r][block_col*SRN + c] == 0 && 
                            !fixed[block_row*SRN + r][block_col*SRN + c]) {
                            
                            int val;
                            do {
                                val = (rand() % N) + 1;
                            } while (used[val]);
                            
                            population[i]->grid[block_row*SRN + r][block_col*SRN + c] = val;
                            used[val] = 1;
                        }
                    }
                }
            }
        }
        
        population[i]->fitness = calculate_conflicts(population[i]->grid);
    }
}

// Selekcja turniejowa
Individual* tournamentSelection(Individual **population, int tournamentSize) {
    Individual *best = population[rand() % POPULATION_SIZE];
    for (int i = 1; i < tournamentSize; i++) {
        Individual *contender = population[rand() % POPULATION_SIZE];
        if (contender->fitness < best->fitness) {
            best = contender;
        }
    }
    return best;
}

// Selekcja ruletkowa (proporcjonalna do fitness)
Individual* rouletteWheelSelection(Individual **population) {
    // Oblicz całkowity fitness (odwrotność konfliktów)
    double totalFitness = 0;
    for (int i = 0; i < POPULATION_SIZE; i++) {
        totalFitness += 1.0 / (population[i]->fitness + 1);
    }
    
    // Losuj wartość z zakresu 0-totalFitness
    double slice = ((double)rand() / RAND_MAX) * totalFitness;
    
    // Znajdź osobnika odpowiadającego wylosowanej wartości
    double currentSum = 0;
    for (int i = 0; i < POPULATION_SIZE; i++) {
        currentSum += 1.0 / (population[i]->fitness + 1);
        if (currentSum >= slice) {
            return population[i];
        }
    }
    
    return population[POPULATION_SIZE - 1];
}

// Krzyżowanie jednopunktowe (dla wierszy)
void singlePointCrossover(Individual *parent1, Individual *parent2, Individual *child1, Individual *child2) {
    int crossoverPoint = rand() % N;
    
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i < crossoverPoint) {
                child1->grid[i][j] = parent1->grid[i][j];
                child2->grid[i][j] = parent2->grid[i][j];
            } else {
                child1->grid[i][j] = parent2->grid[i][j];
                child2->grid[i][j] = parent1->grid[i][j];
            }
        }
    }
    
    // Upewnij się, że stałe wartości pozostają niezmienione
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (fixed[i][j]) {
                child1->grid[i][j] = board[i][j];
                child2->grid[i][j] = board[i][j];
            }
        }
    }
}

// Krzyżowanie blokowe (dla bloków Sudoku)
void blockCrossover(Individual *parent1, Individual *parent2, Individual *child1, Individual *child2) {
    int blockRow = rand() % SRN;
    int blockCol = rand() % SRN;
    
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int currentBlockRow = i / SRN;
            int currentBlockCol = j / SRN;
            
            if (currentBlockRow == blockRow && currentBlockCol == blockCol) {
                child1->grid[i][j] = parent2->grid[i][j];
                child2->grid[i][j] = parent1->grid[i][j];
            } else {
                child1->grid[i][j] = parent1->grid[i][j];
                child2->grid[i][j] = parent2->grid[i][j];
            }
        }
    }
    
    // Upewnij się, że stałe wartości pozostają niezmienione
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (fixed[i][j]) {
                child1->grid[i][j] = board[i][j];
                child2->grid[i][j] = board[i][j];
            }
        }
    }
}

// Mutacja - zamiana dwóch komórek w bloku
void swapMutation(Individual *ind) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (!fixed[i][j] && ((double)rand() / RAND_MAX) < MUTATION_RATE) {
                // Znajdź inny losowy niezamrożony element w tym samym bloku
                int blockRow = i / SRN;
                int blockCol = j / SRN;
                int attempts = 0;
                
                do {
                    int r = blockRow * SRN + (rand() % SRN);
                    int c = blockCol * SRN + (rand() % SRN);
                    attempts++;
                    
                    if (!fixed[r][c] && attempts < 10) {
                        // Zamień wartości
                        int temp = ind->grid[i][j];
                        ind->grid[i][j] = ind->grid[r][c];
                        ind->grid[r][c] = temp;
                        break;
                    }
                } while (attempts < 10);
            }
        }
    }
}

// Mutacja - losowa zmiana wartości komórki
void randomResetMutation(Individual *ind) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (!fixed[i][j] && ((double)rand() / RAND_MAX) < MUTATION_RATE) {
                ind->grid[i][j] = (rand() % N) + 1;
            }
        }
    }
}

// Znajdź najlepszego osobnika w populacji
Individual* findBestIndividual(Individual **population) {
    Individual *best = population[0];
    for (int i = 1; i < POPULATION_SIZE; i++) {
        if (population[i]->fitness < best->fitness) {
            best = population[i];
        }
    }
    return best;
}

// Algorytm genetyczny do rozwiązania Sudoku
void solveSudokuGA() {
    srand(time(NULL));
    
    // Inicjalizacja populacji
    Individual *population[POPULATION_SIZE];
    Individual *newPopulation[POPULATION_SIZE];
    initializePopulation(population);
    
    Individual *bestIndividual = findBestIndividual(population);
    printf("Początkowa liczba konfliktów: %d\n", bestIndividual->fitness);
    
    for (int generation = 0; generation < NUM_GENERATIONS; generation++) {
        // Elitaryzm - zachowaj najlepsze osobniki
        int eliteCount = POPULATION_SIZE * ELITISM_RATE;
        
        // Posortuj populację według fitness
        for (int i = 0; i < POPULATION_SIZE - 1; i++) {
            for (int j = i + 1; j < POPULATION_SIZE; j++) {
                if (population[j]->fitness < population[i]->fitness) {
                    Individual *temp = population[i];
                    population[i] = population[j];
                    population[j] = temp;
                }
            }
        }
        
        // Przenieś najlepsze osobniki do nowej populacji
        for (int i = 0; i < eliteCount; i++) {
            newPopulation[i] = createIndividual();
            copyGrid(population[i]->grid, newPopulation[i]->grid);
            newPopulation[i]->fitness = population[i]->fitness;
        }
        
        // Wypełnij resztę nowej populacji poprzez selekcję, krzyżowanie i mutację
        for (int i = eliteCount; i < POPULATION_SIZE; i += 2) {
            // Selekcja rodziców (można wybrać różne metody)
            Individual *parent1 = tournamentSelection(population, 3);
            Individual *parent2 = rouletteWheelSelection(population);
            
            // Stwórz dzieci
            Individual *child1 = createIndividual();
            Individual *child2 = createIndividual();
            
            // Krzyżowanie (można wybrać różne metody)
            if ((double)rand() / RAND_MAX < CROSSOVER_RATE) {
                if (rand() % 2 == 0) {
                    singlePointCrossover(parent1, parent2, child1, child2);
                } else {
                    blockCrossover(parent1, parent2, child1, child2);
                }
            } else {
                copyGrid(parent1->grid, child1->grid);
                copyGrid(parent2->grid, child2->grid);
            }
            
            // Mutacja (można wybrać różne metody)
            if (rand() % 2 == 0) {
                swapMutation(child1);
                swapMutation(child2);
            } else {
                randomResetMutation(child1);
                randomResetMutation(child2);
            }
            
            // Oblicz fitness dzieci
            child1->fitness = calculate_conflicts(child1->grid);
            child2->fitness = calculate_conflicts(child2->grid);
            
            // Dodaj dzieci do nowej populacji
            newPopulation[i] = child1;
            if (i + 1 < POPULATION_SIZE) {
                newPopulation[i + 1] = child2;
            }
        }
        
        // Zastąp starą populację nową
        for (int i = 0; i < POPULATION_SIZE; i++) {
            freeIndividual(population[i]);
            population[i] = newPopulation[i];
        }
        
        // Znajdź najlepszego osobnika w nowej populacji
        bestIndividual = findBestIndividual(population);
        
        // Wyświetl postęp
        if (generation % 100 == 0) {
            printf("Pokolenie %d: Najlepszy fitness = %d\n", generation, bestIndividual->fitness);
        }
        
        // Zakończ jeśli znaleziono rozwiązanie
        if (bestIndividual->fitness == 0) {
            printf("Znaleziono rozwiązanie w pokoleniu %d\n", generation);
            break;
        }
    }
    
    // Skopiuj najlepsze rozwiązanie do planszy
    copyGrid(bestIndividual->grid, board);
    
    // Wyświetl wynik
    printf("\nSudoku rozwiązane przez GA (konflikty: %d)\n", bestIndividual->fitness);
    
    // Zwolnij pamięć
    for (int i = 0; i < POPULATION_SIZE; i++) {
        freeIndividual(population[i]);
    }
}

// Usuwa komórki z planszy, aby stworzyć zagadkę sudoku z określoną liczbą wskazówek
void removeCells(int clues) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            fixed[i][j] = 1;

    int cellsToRemove = N * N - clues;
    while (cellsToRemove > 0) {
        int i = rand() % N;
        int j = rand() % N;
        if (board[i][j] != 0) {
            board[i][j] = 0;
            fixed[i][j] = 0;
            cellsToRemove--;
        }
    }
}

// Wyświetla aktualny stan planszy

void printBoard() {
    printf("\n");
    for (int i = 0; i < N; i++) {
        if (i % SRN == 0 && i != 0)
            for (int k = 0; k < N * 3; k++) printf("-");
        printf("\n");
        for (int j = 0; j < N; j++) {
            if (j % SRN == 0 && j != 0)
                printf(" | ");
            if (board[i][j] == 0)
                printf(" . ");
            else
                printf("%2d ", board[i][j]);
        }
        printf("\n");
    }
}

// Zapisuje stan gry do pliku

void saveGame() {
    FILE *f = fopen("save.txt", "w");
    if (!f) {
        printf("Błąd zapisu\n");
        return;
    }
    fprintf(f, "%d\n", N);
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            fprintf(f, "%d %d %d\n", board[i][j], fixed[i][j], solution[i][j]);
    fclose(f);
    printf("Gra zapisana!\n");
}

// Wczytuje grę z pliku

int loadGame() {
    FILE *f = fopen("save.txt", "r");
    if (!f) {
        printf("Brak zapisu gry.\n");
        return 0;
    }

    fscanf(f, "%d", &N);
    SRN = sqrt(N);
    allocBoard();

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            fscanf(f, "%d %d %d", &board[i][j], &fixed[i][j], &solution[i][j]);

    fclose(f);
    printf("Gra wczytana!\n");
    return 1;
}

// Główna pętla gry, obsługuje ruchy gracza i interakcję z planszą
void playGame() {
    int x, y, val;
    while (1) {
        printBoard();
        printf("Wprowadź ruch: wiersz kolumna wartość (np. 1 2 5), -1 aby zakończyć, -2 aby zapisać, -3 aby usunąć wartość, -4 aby rozwiązać: ");
        scanf("%d", &x);
        if (x == -1) break;
        if (x == -2) {
            saveGame();
            continue;
        }
        if (x == -3) {
            printf("Podaj wiersz i kolumnę do wyczyszczenia: ");
            scanf("%d %d", &x, &y);
            if (x >= 0 && x < N && y >= 0 && y < N) {
                if (fixed[x][y]) {
                    printf("Nie można usunąć stałej wartości.\n");
                } else {
                    board[x][y] = 0;
                }
            } else {
                printf("Błędne współrzędne.\n");
            }
            continue;
        }

        if (x == -4) {
            if (N == 0 || SRN == 0 || board == NULL) {
                printf("Plansza nie została zainicjalizowana.\n");
                continue;
            }
            printf("Uruchamiam rozwiązywanie przez algorytm genetyczny...\n");
            solveSudokuGA();
            continue;
        }        

        scanf("%d %d", &y, &val);
        if (x >= 0 && x < N && y >= 0 && y < N && val >= 1 && val <= N) {
            if (fixed[x][y]) {
                printf("Nie można zmienić już wypełnionej komórki.\n");
            } else if (val == solution[x][y]) {
                board[x][y] = val;
            } else {
                printf("Niepoprawny ruch\n");
            }
        } else {
            printf("Błędne dane wejściowe\n");
        }
    }
}

// Na podstawie poziomu trudności generuje planszę sudoku z danym procentem komórek do wypełnienia

void generateSudoku(int difficulty) {
    allocBoard();
    fillBoard(0, 0, board);

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            solution[i][j] = board[i][j];

    int clues;
    switch (difficulty) {
        case 1: clues = N*N * 0.6; break;
        case 2: clues = N*N * 0.45; break;
        case 3: clues = N*N * 0.3; break;
        default: clues = N*N * 0.5; break;
    }
    removeCells(clues);
}

// Inicjalizuje puste komórki w blokach 3x3 tak, aby były unikalne
void initialize_solution_randomly() {
    for (int block_row = 0; block_row < SRN; block_row++) {
        for (int block_col = 0; block_col < SRN; block_col++) {
            int present[N + 1];
            for (int i = 0; i <= N; i++) present[i] = 0;
            int positions[N];
            int idx = 0;

            for (int i = 0; i < SRN; i++) {
                for (int j = 0; j < SRN; j++) {
                    int r = block_row * SRN + i;
                    int c = block_col * SRN + j;
                    if (board[r][c] != 0)
                        present[board[r][c]] = 1;
                    else
                        positions[idx++] = r * N + c;
                }
            }

            int values[N];
            int val_idx = 0;
            for (int v = 1; v <= N; v++)
                if (!present[v])
                    values[val_idx++] = v;

            for (int i = val_idx - 1; i > 0; i--) {
                int j = rand() % (i + 1);
                int tmp = values[i];
                values[i] = values[j];
                values[j] = tmp;
            }

            for (int i = 0; i < val_idx; i++) {
                int r = positions[i] / N;
                int c = positions[i] % N;
                board[r][c] = values[i];
            }
        }
    }
}

// Losowa zamiana dwóch niezamrożonych komórek w tym samym bloku
void generate_neighbor() {
    int block_row = rand() % SRN;
    int block_col = rand() % SRN;
    int r1, c1, r2, c2;

    int attempts = 0;
    do {
        r1 = block_row * SRN + rand() % SRN;
        c1 = block_col * SRN + rand() % SRN;
        r2 = block_row * SRN + rand() % SRN;
        c2 = block_col * SRN + rand() % SRN;
        attempts++;
    } while ((fixed[r1][c1] || fixed[r2][c2] || (r1 == r2 && c1 == c2)) && attempts < 100);

    if (!fixed[r1][c1] && !fixed[r2][c2]) {
        int tmp = board[r1][c1];
        board[r1][c1] = board[r2][c2];
        board[r2][c2] = tmp;
    }
}

// Wyświetla menu i obsługuje wybór użytkownika.

void menu() {
    int choice;
    do {
        printf("\n==== SUDOKU ====\n");
        printf("1. Nowa gra\n2. Wczytaj grę\n3. Koniec programu\nWybór: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: {
                printf("Rozmiar planszy (1 - 4x4, 2 - 9x9, 3 - 16x16): ");
                int sizeOpt; scanf("%d", &sizeOpt);
                if (sizeOpt == 1) N = 4;
                else if (sizeOpt == 2) N = 9;
                else N = 16;
                SRN = sqrt(N);

                printf("Poziom trudności (1 - łatwy, 2 - średni, 3 - trudny): ");
                int diff; scanf("%d", &diff);
                generateSudoku(diff);
                playGame();
                freeBoard();
                break;
            }
            case 2: {
                if (loadGame()) {
                    playGame();
                    freeBoard();
                }
                break;
            }
            case 3: {
                printf("Koniec\n");
                break;
            }
            default:
                printf("Nieprawidłowy wybór.\n");
        }
    } while (choice != 3);
}

// Funkcja główna. obsługuje menu i tworzy losową planszę sudoku na podstawie aktualnego czasu systemowego.

int main() {
    srand(time(NULL));
    menu();
    return 0;
}