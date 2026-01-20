#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*═══════════════════════════════════════════════════════════
  PHASE 1 : DÉFINITION DES TOKENS
═══════════════════════════════════════════════════════════*/
typedef enum {
    TOKEN_EOF = 0, TOKEN_IDENTIFIANT, TOKEN_NOMBRE, TOKEN_HEXADECIMAL,
    TOKEN_STRING, TOKEN_PROTOCOL, TOKEN_ENDPROTOCOL, TOKEN_KEYSPACE,
    TOKEN_MAIN, TOKEN_LOOP, TOKEN_FOR, TOKEN_BYTE, TOKEN_PLAIN,
    TOKEN_CIPHER, TOKEN_HASH, TOKEN_KEY256, TOKEN_PLUS, TOKEN_MOINS,
    TOKEN_MULT, TOKEN_DIV, TOKEN_EGAL, TOKEN_EGAL_EGAL, TOKEN_DIFFERENT,
    TOKEN_INFERIEUR, TOKEN_SUPERIEUR, TOKEN_INFERIEUR_EGAL, 
    TOKEN_SUPERIEUR_EGAL, TOKEN_ET_LOGIQUE, TOKEN_OU_LOGIQUE,
    TOKEN_CHIFFREMENT, TOKEN_DECHIFFREMENT, TOKEN_HASH_OP,
    TOKEN_DOUBLE_DEUX_POINTS, TOKEN_POINT_VIRGULE, TOKEN_ACCOLADE_OUV,
    TOKEN_ACCOLADE_FERM, TOKEN_PAREN_OUV, TOKEN_PAREN_FERM, 
    TOKEN_CROCHET_OUV, TOKEN_CROCHET_FERM, TOKEN_FLECHE_DROITE, 
    TOKEN_FLECHE_GAUCHE, TOKEN_COMMENTAIRE, TOKEN_ERREUR
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[256];
    int ligne;
    int colonne;
} Token;

/*═══════════════════════════════════════════════════════════
  PHASE 1 : ANALYSEUR LEXICAL - VARIABLES GLOBALES
═══════════════════════════════════════════════════════════*/
char *source_code;
int pos = 0;
int ligne_lex = 1;
int colonne_lex = 1;
int erreurs = 0;

Token tokens[1000];
int token_count = 0;

// Fonctions utilitaires lexer
char peek() { return source_code[pos]; }
char peek_next() { return (source_code[pos] == '\0') ? '\0' : source_code[pos + 1]; }
char advance() {
    char c = source_code[pos++];
    colonne_lex++;
    if (c == '\n') { ligne_lex++; colonne_lex = 1; }
    return c;
}
void skip_whitespace() { while (isspace(peek())) advance(); }
int is_letter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
int is_digit(char c) { return c >= '0' && c <= '9'; }
int is_hex_digit(char c) { return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

TokenType check_keyword(const char *lexeme) {
    if (strcmp(lexeme, "protocol") == 0) return TOKEN_PROTOCOL;
    if (strcmp(lexeme, "endprotocol") == 0) return TOKEN_ENDPROTOCOL;
    if (strcmp(lexeme, "keyspace") == 0) return TOKEN_KEYSPACE;
    if (strcmp(lexeme, "main") == 0) return TOKEN_MAIN;
    if (strcmp(lexeme, "loop") == 0) return TOKEN_LOOP;
    if (strcmp(lexeme, "for") == 0) return TOKEN_FOR;
    if (strcmp(lexeme, "byte") == 0) return TOKEN_BYTE;
    if (strcmp(lexeme, "plain") == 0) return TOKEN_PLAIN;
    if (strcmp(lexeme, "cipher") == 0) return TOKEN_CIPHER;
    if (strcmp(lexeme, "hash") == 0) return TOKEN_HASH;
    if (strcmp(lexeme, "key256") == 0) return TOKEN_KEY256;
    return TOKEN_IDENTIFIANT;
}

Token scan_hexadecimal() {
    Token token = {TOKEN_HEXADECIMAL, "", ligne_lex, colonne_lex};
    int i = 0;
    token.lexeme[i++] = advance(); // '0'
    token.lexeme[i++] = advance(); // 'x'
    while (is_hex_digit(peek())) token.lexeme[i++] = advance();
    token.lexeme[i] = '\0';
    return token;
}

Token scan_number() {
    Token token = {TOKEN_NOMBRE, "", ligne_lex, colonne_lex};
    int i = 0;
    while (is_digit(peek())) token.lexeme[i++] = advance();
    token.lexeme[i] = '\0';
    return token;
}

Token scan_identifiant() {
    Token token = {TOKEN_IDENTIFIANT, "", ligne_lex, colonne_lex};
    int i = 0;
    if (is_letter(peek()) || peek() == '_') token.lexeme[i++] = advance();
    while (is_letter(peek()) || is_digit(peek()) || peek() == '_') 
        token.lexeme[i++] = advance();
    token.lexeme[i] = '\0';
    token.type = check_keyword(token.lexeme);
    return token;
}

Token scan_string() {
    Token token = {TOKEN_ERREUR, "", ligne_lex, colonne_lex};
    int i = 0;
    token.lexeme[i++] = advance(); // '"'
    
    while (peek() != '"' && peek() != '\0' && peek() != '\n') {
        if (i < 255) token.lexeme[i++] = advance();
        else { advance(); } // éviter buffer overflow
    }
    
    if (peek() == '"') {
        token.lexeme[i++] = advance(); // fermer le guillemet
        token.lexeme[i] = '\0';
        token.type = TOKEN_STRING;
    } else {
        // Chaîne non fermée → erreur lexicale explicite
        strcpy(token.lexeme, "chaîne non fermée");
        token.type = TOKEN_ERREUR;
    }
    return token;
}

Token scan_commentaire() {
    Token token = {TOKEN_COMMENTAIRE, "//...", ligne_lex, colonne_lex};
    advance(); advance(); // "//"
    while (peek() != '\n' && peek() != '\0') advance();
    return token;
}

Token get_next_token() {
    skip_whitespace();
    Token token = {TOKEN_ERREUR, "", ligne_lex, colonne_lex};
    char c = peek();
    
    if (c == '\0') { token.type = TOKEN_EOF; strcpy(token.lexeme, "EOF"); return token; }
    if (c == '/' && peek_next() == '/') return scan_commentaire();
    if (c == '0' && (peek_next() == 'x' || peek_next() == 'X')) return scan_hexadecimal();
    if (is_digit(c)) return scan_number();
    if (c == '"') return scan_string();
    if (is_letter(c) || c == '_') return scan_identifiant();
    
    token.lexeme[0] = advance();
    token.lexeme[1] = '\0';
    
  switch (token.lexeme[0]) {
    case '@':
        if (peek() == '>') { 
            advance(); 
            token.type = TOKEN_CHIFFREMENT; 
            strcpy(token.lexeme, "@>"); 
        }
        else if (is_letter(peek()) || peek() == '_') {
            Token id = scan_identifiant();
            id.type = check_keyword(id.lexeme);
            
            // Construction sécurisée de "@mot_clé"
            token.lexeme[0] = '@';
            strncpy(token.lexeme + 1, id.lexeme, sizeof(token.lexeme) - 2);
            token.lexeme[sizeof(token.lexeme) - 1] = '\0';
            token.type = id.type;
            return token;
        }
        break;
        case '<':
            if (peek() == '@') { advance(); token.type = TOKEN_DECHIFFREMENT; strcpy(token.lexeme, "<@"); }
            else if (peek() == '=') { advance(); token.type = TOKEN_INFERIEUR_EGAL; strcpy(token.lexeme, "<="); }
            else if (peek() == '-') { advance(); token.type = TOKEN_FLECHE_GAUCHE; strcpy(token.lexeme, "<-"); }
            else token.type = TOKEN_INFERIEUR;
            break;
        case '>':
            if (peek() == '=') { advance(); token.type = TOKEN_SUPERIEUR_EGAL; strcpy(token.lexeme, ">="); }
            else token.type = TOKEN_SUPERIEUR;
            break;
        case '#':
            if (peek() == '>') { advance(); token.type = TOKEN_HASH_OP; strcpy(token.lexeme, "#>"); }
            break;
        case '&':
            if (peek() == '&') { advance(); token.type = TOKEN_ET_LOGIQUE; strcpy(token.lexeme, "&&"); }
            break;
        case '|':
            if (peek() == '|') { advance(); token.type = TOKEN_OU_LOGIQUE; strcpy(token.lexeme, "||"); }
            break;
        case '!':
            if (peek() == '=') { advance(); token.type = TOKEN_DIFFERENT; strcpy(token.lexeme, "!="); }
            break;
        case '=':
            if (peek() == '=') { advance(); token.type = TOKEN_EGAL_EGAL; strcpy(token.lexeme, "=="); }
            else token.type = TOKEN_EGAL;
            break;
        case '-':
            if (peek() == '>') { advance(); token.type = TOKEN_FLECHE_DROITE; strcpy(token.lexeme, "->"); }
            else token.type = TOKEN_MOINS;
            break;
        case ':':
            if (peek() == ':') { advance(); token.type = TOKEN_DOUBLE_DEUX_POINTS; strcpy(token.lexeme, "::"); }
            break;
        case '+': token.type = TOKEN_PLUS; break;
        case '*': token.type = TOKEN_MULT; break;
        case '/': token.type = TOKEN_DIV; break;
        case ';': token.type = TOKEN_POINT_VIRGULE; break;
        case '{': token.type = TOKEN_ACCOLADE_OUV; break;
        case '}': token.type = TOKEN_ACCOLADE_FERM; break;
        case '(': token.type = TOKEN_PAREN_OUV; break;
        case ')': token.type = TOKEN_PAREN_FERM; break;
        case '[': token.type = TOKEN_CROCHET_OUV; break;
        case ']': token.type = TOKEN_CROCHET_FERM; break;
    }
    return token;
}

void tokenize(const char* code) {
    source_code = (char*)code;
    pos = 0; ligne_lex = 1; colonne_lex = 1; token_count = 0;
    
    Token token;
    do {
        token = get_next_token();
        if (token.type == TOKEN_ERREUR) {
            printf(" ERREUR LEXICALE [Ligne %d, Colonne %d] : %s\n", 
                   token.ligne, token.colonne, token.lexeme);
            erreurs++;  // ← maintenant déclarée globalement → OK
            return;
        }
        if (token.type != TOKEN_COMMENTAIRE) {
            tokens[token_count++] = token;
        }
    } while (token.type != TOKEN_EOF);
}

/*═══════════════════════════════════════════════════════════
  PHASE 2 : TABLE DES SYMBOLES
═══════════════════════════════════════════════════════════*/
#define MAX_SYMBOLES 100

typedef struct {
    char nom[256];
    char type[50];
    int ligne_declaration;
    int utilisee;
} Symbole;

Symbole table_symboles[MAX_SYMBOLES];
int nb_symboles = 0;

int ajouter_symbole(const char* nom, const char* type, int ligne) {
    for (int i = 0; i < nb_symboles; i++) {
        if (strcmp(table_symboles[i].nom, nom) == 0) {
            printf(" ERREUR SÉMANTIQUE [Ligne %d] : Variable '%s' déjà déclarée ligne %d\n",
                   ligne, nom, table_symboles[i].ligne_declaration);
            erreurs++;  
            return -1;
        }
    }
    strcpy(table_symboles[nb_symboles].nom, nom);
    strcpy(table_symboles[nb_symboles].type, type);
    table_symboles[nb_symboles].ligne_declaration = ligne;
    table_symboles[nb_symboles].utilisee = 0;
    nb_symboles++;
    return nb_symboles - 1;
}

char* obtenir_type(const char* nom, int ligne) {
    for (int i = 0; i < nb_symboles; i++) {
        if (strcmp(table_symboles[i].nom, nom) == 0) {
            table_symboles[i].utilisee = 1;
            return table_symboles[i].type;
        }
    }
    printf(" ERREUR SÉMANTIQUE [Ligne %d] : Variable '%s' non déclarée\n", ligne, nom);
    erreurs++;  
    return "erreur";
}

int types_compatibles(const char* type1, const char* type2) {
    if (strcmp(type1, type2) == 0) return 1;
    if (strcmp(type1, "erreur") == 0 || strcmp(type2, "erreur") == 0) return 1;
    if ((strcmp(type1, "byte") == 0 && strcmp(type2, "nombre") == 0) ||
        (strcmp(type1, "nombre") == 0 && strcmp(type2, "byte") == 0)) return 1;
    return 0;
}

/*═══════════════════════════════════════════════════════════
  PHASE 3 : GÉNÉRATION QUADRUPLETS
═══════════════════════════════════════════════════════════*/
#define MAX_QUADRUPLETS 200

typedef struct {
    char operateur[10];
    char arg1[50];
    char arg2[50];
    char resultat[50];
} Quadruplet;

Quadruplet quadruplets[MAX_QUADRUPLETS];
int nb_quadruplets = 0;
int temp_counter = 0;

char* nouveau_temporaire() {
    static char temp[20];
    sprintf(temp, "t%d", temp_counter++);
    return temp;
}

char* nouvelle_etiquette() {
    static char label[20];
    static int label_counter = 0;
    sprintf(label, "L%d", label_counter++);
    return label;
}

void generer_quadruplet(const char* op, const char* arg1, const char* arg2, const char* res) {
    strcpy(quadruplets[nb_quadruplets].operateur, op);
    strcpy(quadruplets[nb_quadruplets].arg1, arg1);
    strcpy(quadruplets[nb_quadruplets].arg2, arg2);
    strcpy(quadruplets[nb_quadruplets].resultat, res);
    nb_quadruplets++;
}

/*═══════════════════════════════════════════════════════════
  PHASE 4 : ANALYSEUR SYNTAXIQUE + SÉMANTIQUE
═══════════════════════════════════════════════════════════*/
Token current_token;
int token_index = 0;


void erreur(const char* message) {
    printf(" ERREUR SYNTAXIQUE [Ligne %d] : %s (token: '%s')\n", 
           current_token.ligne, message, current_token.lexeme);
    erreurs++;
}

void avancer_token() {
    if (token_index < token_count - 1) {
        token_index++;
        current_token = tokens[token_index];
    }
}

int accepter_token(TokenType type) {
    if (current_token.type == type) {
        avancer_token();
        return 1;
    }
    return 0;
}

int attendre_token(TokenType type, const char* message) {
    if (current_token.type == type) {
        avancer_token();
        return 1;
    }
    erreur(message);
    return 0;
}

int verifier_token(TokenType type) {
    return current_token.type == type;
}

typedef struct {
    char addr[50];
    char type[50];
} ExpressionInfo;

ExpressionInfo parse_expression();
ExpressionInfo parse_terme();
ExpressionInfo parse_facteur();

ExpressionInfo parse_facteur() {
    ExpressionInfo info;
    strcpy(info.addr, "");
    strcpy(info.type, "");
    
    if (verifier_token(TOKEN_NOMBRE)) {
        strcpy(info.addr, current_token.lexeme);
        strcpy(info.type, "nombre");
        avancer_token();
    }
    else if (verifier_token(TOKEN_HEXADECIMAL)) {
        strcpy(info.addr, current_token.lexeme);
        strcpy(info.type, "hexadecimal");
        avancer_token();
    }
    else if (verifier_token(TOKEN_IDENTIFIANT)) {
        strcpy(info.addr, current_token.lexeme);
        strcpy(info.type, obtenir_type(current_token.lexeme, current_token.ligne));
        avancer_token();
    }
    else if (verifier_token(TOKEN_STRING)) {
        strcpy(info.addr, current_token.lexeme);
        strcpy(info.type, "plain");
        avancer_token();
    }
    else if (accepter_token(TOKEN_PAREN_OUV)) {
        info = parse_expression();
        attendre_token(TOKEN_PAREN_FERM, "Attendu ')'");
    }
    else {
        erreur("Attendu nombre, identifiant ou '('");
        avancer_token();
        strcpy(info.type, "erreur");
    }
    return info;
}

ExpressionInfo parse_terme() {
    ExpressionInfo gauche = parse_facteur();
    
    while (verifier_token(TOKEN_MULT) || verifier_token(TOKEN_DIV)) {
        char op[10];
        strcpy(op, current_token.lexeme);
        avancer_token();
        
        ExpressionInfo droite = parse_facteur();
        
        if (!types_compatibles(gauche.type, droite.type)) {
            printf(" ERREUR SÉMANTIQUE [Ligne %d] : Types incompatibles\n", current_token.ligne);
            erreurs++;
        }
        
        char* temp = nouveau_temporaire();
        generer_quadruplet(op, gauche.addr, droite.addr, temp);
        strcpy(gauche.addr, temp);
        strcpy(gauche.type, "nombre");
    }
    return gauche;
}

ExpressionInfo parse_expression() {
    ExpressionInfo gauche = parse_terme();
    
    while (verifier_token(TOKEN_PLUS) || verifier_token(TOKEN_MOINS)) {
        char op[10];
        strcpy(op, current_token.lexeme);
        avancer_token();
        
        ExpressionInfo droite = parse_terme();
        
        if (!types_compatibles(gauche.type, droite.type)) {
            printf(" ERREUR SÉMANTIQUE [Ligne %d] : Types incompatibles\n", current_token.ligne);
            erreurs++;
        }
        
        char* temp = nouveau_temporaire();
        generer_quadruplet(op, gauche.addr, droite.addr, temp);
        strcpy(gauche.addr, temp);
        strcpy(gauche.type, "nombre");
    }
    
    if (verifier_token(TOKEN_EGAL_EGAL) || verifier_token(TOKEN_DIFFERENT) ||
        verifier_token(TOKEN_INFERIEUR) || verifier_token(TOKEN_SUPERIEUR)) {
        char op[10];
        strcpy(op, current_token.lexeme);
        avancer_token();
        
        ExpressionInfo droite = parse_terme();
        char* temp = nouveau_temporaire();
        generer_quadruplet(op, gauche.addr, droite.addr, temp);
        strcpy(gauche.addr, temp);
        strcpy(gauche.type, "bool");
    }
    
    if (verifier_token(TOKEN_CHIFFREMENT) || verifier_token(TOKEN_DECHIFFREMENT)) {
        char op[10];
        strcpy(op, current_token.lexeme);
        avancer_token();
        
        ExpressionInfo droite = parse_terme();
        char* temp = nouveau_temporaire();
        generer_quadruplet(op, gauche.addr, droite.addr, temp);
        strcpy(gauche.addr, temp);
        strcpy(gauche.type, "cipher");
    }
    
    if (verifier_token(TOKEN_HASH_OP)) {
        avancer_token();
        char* temp = nouveau_temporaire();
        generer_quadruplet("#>", gauche.addr, "", temp);
        strcpy(gauche.addr, temp);
        strcpy(gauche.type, "hash");
    }
    
    return gauche;
}

void parse_instruction() {
    if (verifier_token(TOKEN_BYTE) || verifier_token(TOKEN_PLAIN) || 
        verifier_token(TOKEN_CIPHER) || verifier_token(TOKEN_HASH) || 
        verifier_token(TOKEN_KEY256)) {
        
        char type[50];
        strcpy(type, current_token.lexeme);
        avancer_token();
        
        attendre_token(TOKEN_DOUBLE_DEUX_POINTS, "Attendu '::'");
        
        if (verifier_token(TOKEN_IDENTIFIANT)) {
            char var_name[256];
            strcpy(var_name, current_token.lexeme);
            int ligne = current_token.ligne;
            avancer_token();
            
            ajouter_symbole(var_name, type, ligne);
            
            if (accepter_token(TOKEN_EGAL)) {
                ExpressionInfo expr = parse_expression();
                if (!types_compatibles(type, expr.type)) {
                    printf(" ERREUR SÉMANTIQUE [Ligne %d] : Type incompatible\n", ligne);
                    erreurs++;
                }
                generer_quadruplet("=", expr.addr, "", var_name);
            }
            attendre_token(TOKEN_POINT_VIRGULE, "Attendu ';'");
        }
    }
    else if (verifier_token(TOKEN_IDENTIFIANT)) {
        char var_name[256];
        strcpy(var_name, current_token.lexeme);
        int ligne = current_token.ligne;
        char* var_type = obtenir_type(var_name, ligne);
        avancer_token();
        
        attendre_token(TOKEN_EGAL, "Attendu '='");
        ExpressionInfo expr = parse_expression();
        
        if (!types_compatibles(var_type, expr.type)) {
            printf(" ERREUR SÉMANTIQUE [Ligne %d] : Type incompatible\n", ligne);
            erreurs++;
        }
        generer_quadruplet("=", expr.addr, "", var_name);
        attendre_token(TOKEN_POINT_VIRGULE, "Attendu ';'");
    }
    else if (verifier_token(TOKEN_FLECHE_DROITE)) {
        avancer_token();
        ExpressionInfo expr = parse_expression();
        generer_quadruplet("OUT", expr.addr, "", "");
        attendre_token(TOKEN_POINT_VIRGULE, "Attendu ';'");
    }
    else if (verifier_token(TOKEN_FLECHE_GAUCHE)) {
        avancer_token();
        if (verifier_token(TOKEN_IDENTIFIANT)) {
            char var_name[256];
            strcpy(var_name, current_token.lexeme);
            obtenir_type(var_name, current_token.ligne);
            avancer_token();
            generer_quadruplet("IN", "", "", var_name);
        }
        attendre_token(TOKEN_POINT_VIRGULE, "Attendu ';'");
    }
    else if (verifier_token(TOKEN_LOOP)) {
        avancer_token();
        attendre_token(TOKEN_CROCHET_OUV, "Attendu '['");
        
        char* debut = nouvelle_etiquette();
        generer_quadruplet("LABEL", debut, "", "");
        
        ExpressionInfo cond = parse_expression();
        
        char* fin = nouvelle_etiquette();
        generer_quadruplet("JZ", cond.addr, fin, "");
        
        attendre_token(TOKEN_CROCHET_FERM, "Attendu ']'");
        attendre_token(TOKEN_ACCOLADE_OUV, "Attendu '{'");
        
        while (!verifier_token(TOKEN_ACCOLADE_FERM) && !verifier_token(TOKEN_EOF)) {
            parse_instruction();
        }
        
        generer_quadruplet("JUMP", debut, "", "");
        generer_quadruplet("LABEL", fin, "", "");
        attendre_token(TOKEN_ACCOLADE_FERM, "Attendu '}'");
    }
    else {
        avancer_token();
    }
}

void parse_programme() {
    attendre_token(TOKEN_PROTOCOL, "Attendu '@protocol'");
    attendre_token(TOKEN_IDENTIFIANT, "Attendu nom du programme");
    
    if (verifier_token(TOKEN_KEYSPACE)) {
        avancer_token();
        attendre_token(TOKEN_ACCOLADE_OUV, "Attendu '{'");
        while (!verifier_token(TOKEN_ACCOLADE_FERM) && !verifier_token(TOKEN_EOF)) {
            parse_instruction();
        }
        attendre_token(TOKEN_ACCOLADE_FERM, "Attendu '}'");
    }
    
    attendre_token(TOKEN_MAIN, "Attendu '@main'");
    attendre_token(TOKEN_ACCOLADE_OUV, "Attendu '{'");
    while (!verifier_token(TOKEN_ACCOLADE_FERM) && !verifier_token(TOKEN_EOF)) {
        parse_instruction();
    }
    attendre_token(TOKEN_ACCOLADE_FERM, "Attendu '}'");
    attendre_token(TOKEN_ENDPROTOCOL, "Attendu '@endprotocol'");
}

/*═══════════════════════════════════════════════════════════
  AFFICHAGE DES RÉSULTATS
═══════════════════════════════════════════════════════════*/
void afficher_tokens() {
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║              PHASE 1 : TOKENS GÉNÉRÉS                  ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    for (int i = 0; i < token_count && i < 20; i++) {
        printf("[L%d:C%d] %-20s : \"%s\"\n", 
               tokens[i].ligne, tokens[i].colonne, 
               tokens[i].type == TOKEN_IDENTIFIANT ? "IDENTIFIANT" :
               tokens[i].type == TOKEN_NOMBRE ? "NOMBRE" : "...",
               tokens[i].lexeme);
    }
    if (token_count > 20) printf("... (%d tokens au total)\n", token_count);
}

void afficher_table_symboles() {
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║          PHASE 2 : TABLE DES SYMBOLES                  ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║ Nom              │ Type        │ Ligne │ Utilisée     ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    for (int i = 0; i < nb_symboles; i++) {
        printf("║ %-16s │ %-11s │ %-5d │ %-12s ║\n",
               table_symboles[i].nom, table_symboles[i].type,
               table_symboles[i].ligne_declaration,
               table_symboles[i].utilisee ? "Oui" : "Non");
    }
    printf("╚════════════════════════════════════════════════════════╝\n");
}

void afficher_quadruplets() {
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║       PHASE 3 : CODE INTERMÉDIAIRE (QUADRUPLETS)       ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║ #   │ Opérateur  │ Arg1      │ Arg2      │ Résultat  ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    for (int i = 0; i < nb_quadruplets; i++) {
        printf("║ %-3d │ %-10s │ %-9s │ %-9s │ %-9s ║\n",
               i, quadruplets[i].operateur, quadruplets[i].arg1,
               quadruplets[i].arg2, quadruplets[i].resultat);
    }
    printf("╚════════════════════════════════════════════════════════╝\n");
}

/*═══════════════════════════════════════════════════════════
  FONCTION PRINCIPALE
═══════════════════════════════════════════════════════════*/
int main() {
    printf("\n╔═════════════════════════════════════════════════════════╗\n");
    printf("║          COMPILATEUR CRYPTOLANG - PIPELINE COMPLET        ║\n");
    printf("║      Lexical → Syntaxique → Sémantique                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    

const char* code_source = 
    "@protocol ProgrammeCorrect\n"
    "\n"
    "@keyspace {\n"
    "    byte :: compteur = 5;\n"
    "}\n"
    "\n"
    "@main {\n"
    "    plain :: msg = \"Secret\";\n"
    "    \n"
    "    @loop [ compteur > 0 ] {\n"
    "        compteur = compteur - 1;\n"
    "    }\n"
    "    \n"
    "    hash :: h = msg #>;\n"
    "    -> h;\n"
    "}\n"
    "\n"
    "@endprotocol";
    
    printf("\n CODE SOURCE:\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("%s\n", code_source);
    printf("═══════════════════════════════════════════════════════════\n");
    
    // ▶ PHASE 1 : ANALYSE LEXICALE
printf("\n PHASE 1 : ANALYSE LEXICALE\n");
erreurs = 0; // réinitialiser
tokenize(code_source);

if (erreurs > 0) {
    printf("\n COMPILATION ÉCHOUÉE : Erreur lexicale détectée.\n");
    return 1;
}

printf(" %d tokens générés\n", token_count);
afficher_tokens();

// ▶ PHASE 2+ : seulement si pas d’erreur lexicale
printf("\n PHASE 2-3 : ANALYSE SYNTAXIQUE, SÉMANTIQUE ET GÉNÉRATION\n");
token_index = 0;
current_token = tokens[0];
parse_programme();

// Si erreur syntaxique/sémantique, on peut aussi arrêter proprement
if (erreurs > 0) {
    printf("\n COMPILATION ÉCHOUÉE : %d erreur(s) détectée(s).\n", erreurs);
    return 1;
}
    
    // ▶ RÉSULTATS FINAUX
    afficher_table_symboles();
    afficher_quadruplets();
    
    printf("\n═══════════════════════════════════════════════════════════\n");
    if (erreurs == 0) {
        printf(" COMPILATION RÉUSSIE : Aucune erreur!\n");
    } else {
        printf(" COMPILATION ÉCHOUÉE : %d erreur(s)\n", erreurs);
    }
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    return 0;
}