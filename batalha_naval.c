/* batalha_naval.c
 * Desafio Jogo de Batalha Naval (C)
 * - Tabuleiro NxN (default 8x8).
 * - Frota: 1x4, 2x3, 2x2 (clássico simplificado).
 * - Colocação aleatória para jogador e CPU (sem sobreposição).
 * - Jogador atira primeiro; CPU atira aleatoriamente sem repetir.
 * - Entrada de tiro: "A5" (linha A..H, coluna 1..N) ou "5 1" (linha, coluna).
 * - Impressão: tabuleiro do jogador revela navios; do CPU mostra só acertos/erros.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#define N 8                /* tamanho do tabuleiro */
#define AGUA '~'           /* célula desconhecida (vista do alvo) */
#define VAZIO '.'          /* célula vazia no tabuleiro real */
#define NAVIO '#'
#define ERRO 'o'           /* tiro na água */
#define ACERTO 'X'

typedef struct {
    int len;
} Navio;

/* frota: 1 de 4, 2 de 3, 2 de 2 */
static const Navio FROTA[] = { {4}, {3}, {3}, {2}, {2} };
static const int FROTA_TAM = (int)(sizeof(FROTA) / sizeof(FROTA[0]));

/* ------------------ util ------------------ */

static void init_tab(char t[N][N], char fill) {
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            t[r][c] = fill;
}

static int dentro(int r, int c) {
    return (r >= 0 && r < N && c >= 0 && c < N);
}

static int cabe_livre(const char t[N][N], int r, int c, int len, int horizontal) {
    if (horizontal) {
        if (c + len > N) return 0;
        for (int j = 0; j < len; j++)
            if (t[r][c + j] != VAZIO) return 0;
    } else {
        if (r + len > N) return 0;
        for (int i = 0; i < len; i++)
            if (t[r + i][c] != VAZIO) return 0;
    }
    return 1;
}

static void coloca_navio(char t[N][N], int r, int c, int len, int horizontal) {
    if (horizontal) {
        for (int j = 0; j < len; j++) t[r][c + j] = NAVIO;
    } else {
        for (int i = 0; i < len; i++) t[r + i][c] = NAVIO;
    }
}

static void coloca_frota_aleatoria(char t[N][N]) {
    for (int k = 0; k < FROTA_TAM; k++) {
        int len = FROTA[k].len;
        int tentativas = 0;
        while (1) {
            int horizontal = rand() % 2;
            int r = rand() % N;
            int c = rand() % N;
            if (cabe_livre(t, r, c, len, horizontal)) {
                coloca_navio(t, r, c, len, horizontal);
                break;
            }
            if (++tentativas > 5000) {
                /* em casos raros de saturação, recomeça o tabuleiro */
                init_tab(t, VAZIO);
                k = -1; /* recomeça frota */
                break;
            }
        }
    }
}

/* impressão: jogador vê navios; alvo (CPU) esconde navios ainda não atingidos */
static void imprime_tabuleiro(const char t[N][N], int revelar_navios) {
    printf("   ");
    for (int c = 0; c < N; c++) printf("%2d ", c + 1);
    puts("");
    for (int r = 0; r < N; r++) {
        printf(" %c ", 'A' + r);
        for (int c = 0; c < N; c++) {
            char ch = t[r][c];
            if (!revelar_navios && ch == NAVIO) ch = AGUA;
            printf(" %c ", ch);
        }
        puts("");
    }
}

static int parse_coord(const char* s, int* out_r, int* out_c) {
    /* aceita "A5" ou "a5", e "5 1" (linha col) */
    int r = -1, c = -1;

    /* ignora espaços no início */
    while (*s && isspace((unsigned char)*s)) s++;

    if (isalpha((unsigned char)*s)) {
        r = toupper((unsigned char)*s) - 'A';
        s++;
        /* salta espaços */
        while (*s && isspace((unsigned char)*s)) s++;
        if (!isdigit((unsigned char)*s)) return 0;
        long col = strtol(s, NULL, 10);
        c = (int)col - 1;
    } else {
        /* tenta "linha coluna" numérica */
        int rr = 0, cc = 0;
        if (sscanf(s, "%d %d", &rr, &cc) != 2) return 0;
        r = rr - 1;
        c = cc - 1;
    }

    if (!dentro(r, c)) return 0;
    *out_r = r; *out_c = c;
    return 1;
}

/* devolve: -1 já atirado; 0 água; 1 acerto */
static int disparar(char t[N][N], int r, int c) {
    if (t[r][c] == ACERTO || t[r][c] == ERRO) return -1;
    if (t[r][c] == NAVIO) { t[r][c] = ACERTO; return 1; }
    if (t[r][c] == VAZIO) { t[r][c] = ERRO; return 0; }
    /* caso esteja AGUA (em tab “alvo”), nunca guardamos AGUA no real; mas tratamos: */
    if (t[r][c] == AGUA)  { t[r][c] = ERRO; return 0; }
    return 0;
}

static int navios_restantes(const char t[N][N]) {
    int rest = 0;
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            if (t[r][c] == NAVIO) rest++;
    return rest;
}

/* CPU escolhe célula aleatória ainda não tentada */
static void cpu_joga(char t_jogador[N][N]) {
    while (1) {
        int r = rand() % N;
        int c = rand() % N;
        if (t_jogador[r][c] == ACERTO || t_jogador[r][c] == ERRO) continue;
        int res = disparar(t_jogador, r, c);
        printf("CPU disparou em %c%d: %s\n",
               'A' + r, c + 1, (res == 1 ? "ACERTOU!" : "água."));
        break;
    }
}

/* ------------------ jogo ------------------ */

int main(void) {
    srand((unsigned)time(NULL));

    char tab_jogador[N][N];
    char tab_cpu[N][N];

    init_tab(tab_jogador, VAZIO);
    init_tab(tab_cpu, VAZIO);

    coloca_frota_aleatoria(tab_jogador);
    coloca_frota_aleatoria(tab_cpu);

    puts("\n=== BATALHA NAVAL ===");
    puts("Coordenadas: \"A5\" ou \"5 1\". Ex.: A1, C8, 3 2");
    puts("Objetivo: afundar todos os navios do adversário.\n");

    while (1) {
        /* mostra visão atual */
        puts("Seu tabuleiro:");
        imprime_tabuleiro(tab_jogador, /*revelar_navios=*/1);
        puts("\nAlvo (CPU) - acertos/erros (navios escondidos):");
        imprime_tabuleiro(tab_cpu, /*revelar_navios=*/0);
        puts("");

        /* jogada do jogador */
        char linha[64];
        int r, c;
        while (1) {
            printf("Seu tiro (ex.: A5 ou \"5 1\"): ");
            if (!fgets(linha, sizeof(linha), stdin)) return 0;
            if (!parse_coord(linha, &r, &c)) {
                puts("Coordenada inválida. Tente novamente.");
                continue;
            }
            int res = disparar(tab_cpu, r, c);
            if (res == -1) {
                puts("Já disparou aí. Escolha outra célula.");
                continue;
            }
            printf("Disparo em %c%d: %s\n",
                   'A' + r, c + 1, (res == 1 ? "ACERTO!" : "água."));
            break;
        }

        if (navios_restantes(tab_cpu) == 0) {
            puts("\nParabéns! Venceu: todos os navios inimigos foram afundados.");
            break;
        }

        /* jogada da CPU */
        cpu_joga(tab_jogador);
        if (navios_restantes(tab_jogador) == 0) {
            puts("\nDerrota! A CPU afundou toda a sua frota.");
            break;
        }

        puts("\n----------------------------------------------\n");
    }

    puts("\nEstado final:");
    puts("Seu tabuleiro:");
    imprime_tabuleiro(tab_jogador, 1);
    puts("\nTabuleiro da CPU (revelado):");
    imprime_tabuleiro(tab_cpu, 1);

    return 0;
}
