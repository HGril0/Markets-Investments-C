/* ============================================================
   PROJETO FINAL - MerCInvest
   Linguagens de Programacao 1 - 2025/2026
   Programa de gestao de mercado bolsista usando listas ligadas.
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
   DEFINICOES DE TAMANHOS MAXIMOS (incluindo o '\0' do final)
   ============================================================ */
#define MAX_TICKER  11   /* ex: "AAPL\0" */
#define MAX_NAME    51   /* nome da empresa */
#define MAX_SECTOR  31   /* sector (ex: "Tech") */

/* ============================================================
   ESTRUTURAS DE DADOS (listas ligadas obrigatorias)
   ============================================================ */

/* Um no da lista de cotacoes de uma empresa */
typedef struct QuoteNode {
    int   day;    /* data no formato YYYYMMDD, ex: 20250101 */
    float open;   /* preco de abertura */
    float high;   /* preco maximo do dia */
    float low;    /* preco minimo do dia */
    float close;  /* preco de fecho */
    int   volume; /* volume de transacoes */
    struct QuoteNode* next; /* aponta para a proxima cotacao (mais recente) */
} QuoteNode;

/* Um no da lista de empresas */
typedef struct CompanyNode {
    char ticker[MAX_TICKER]; /* simbolo unico da empresa, ex: "AAA" */
    char name[MAX_NAME];     /* nome completo, ex: "EmpresaA" */
    char sector[MAX_SECTOR]; /* sector de actividade, ex: "Tech" */
    int  quotes_count;       /* quantas cotacoes esta empresa tem */
    QuoteNode*          quotes_head; /* cabeca da lista de cotacoes */
    struct CompanyNode* next;        /* proxima empresa na lista */
} CompanyNode;

/* O mercado em si: contem a lista de todas as empresas */
typedef struct {
    CompanyNode* companies_head; /* cabeca da lista de empresas */
    int          company_count;  /* quantas empresas existem */
} Market;

/* ============================================================
   MACROS DE OUTPUT (nao alterar - formato exigido pelo Pandora)
   ============================================================ */
#define PRINT_OK()                  printf("OK\n")
#define PRINT_ERROR(code)           printf("ERROR %d\n", code)
#define PRINT_ERROR_PANDORA()       printf("ERROR PANDORA\n")
#define PRINT_COMPANIES_HEADER(n)   printf("COMPANIES %d\n", n)
#define PRINT_COMPANY(t,n,s,c)      printf("%s;%s;%s;%d\n", t, n, s, c)
#define PRINT_QUOTES_HEADER(t,n)    printf("QUOTES %s %d\n", t, n)
#define PRINT_QUOTE(d,c)            printf("%08d;%.2f\n", d, c)
#define PRINT_REMOVED(n)            printf("REMOVED %d\n", n)
#define PRINT_BANKRUPT(n)           printf("REMOVED %s\n", n)
#define PRINT_SUGGEST_HEADER(n)     printf("SUGGESTION %d\n", n)
#define PRINT_SUGGESTION(t,q)       printf("%s;%d\n", t, q)

/* ============================================================
   ESTRUTURA AUXILIAR PARA INVEST.SUGGEST
   Guarda uma empresa e a metrica calculada para a ordenar
   ============================================================ */
typedef struct {
    CompanyNode* company;
    double       metric;
} Candidate;


/* ============================================================
   PROTOTIPOS (declaracoes antecipadas para o compilador)
   ============================================================ */
static void libertar_mercado(Market* m);
static void terminar_com_erro(int codigo, Market* m);


/* ============================================================
   GESTAO DE ERROS
   Imprime o erro, liberta toda a memoria e sai do programa.
   ============================================================ */
static void terminar_com_erro(int codigo, Market* m) {
    PRINT_ERROR(codigo);
    libertar_mercado(m);
    exit(0);
}


/* ============================================================
   LIBERTAR MEMORIA
   ============================================================ */

/* Liberta todos os nos da lista de cotacoes de uma empresa */
static void libertar_cotacoes(QuoteNode* cabeca) {
    QuoteNode* atual = cabeca;
    while (atual != NULL) {
        QuoteNode* proximo = atual->next;
        free(atual);
        atual = proximo;
    }
}

/* Liberta todas as empresas e as suas cotacoes */
static void libertar_mercado(Market* m) {
    if (m == NULL) return;
    CompanyNode* atual = m->companies_head;
    while (atual != NULL) {
        CompanyNode* proxima = atual->next;
        libertar_cotacoes(atual->quotes_head);
        free(atual);
        atual = proxima;
    }
    m->companies_head = NULL;
    m->company_count  = 0;
}


/* ============================================================
   FUNCOES AUXILIARES DE STRING
   ============================================================ */

/* Remove '\n' e '\r' do final de uma string (limpeza apos fgets) */
static void remover_newline(char* str) {
    if (str == NULL) return;
    int len = (int)strlen(str);
    while (len > 0) {
        char ultimo = str[len - 1];
        if (ultimo == '\n' || ultimo == '\r') {
            str[len - 1] = '\0';
            len--;
        } else {
            break;
        }
    }
}

/* Verifica se uma string representa um numero inteiro positivo (sem sinal) */
static int e_inteiro_positivo(const char* str) {
    if (str == NULL || str[0] == '\0') return 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') return 0;
    }
    return 1;
}

/* Verifica se uma string representa um numero decimal positivo (ex: "10.50") */
static int e_decimal_positivo(const char* str) {
    int pontos  = 0;
    int digitos = 0;
    if (str == NULL || str[0] == '\0') return 0;
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if (c == '.') {
            pontos++;
            if (pontos > 1) return 0; /* mais de um ponto = invalido */
        } else if (c >= '0' && c <= '9') {
            digitos++;
        } else {
            return 0; /* caracter inesperado */
        }
    }
    return digitos > 0;
}


/* ============================================================
   OPERACOES SOBRE EMPRESAS
   ============================================================ */

/* Procura uma empresa pelo ticker; devolve o no ou NULL se nao existir */
static CompanyNode* encontrar_empresa(Market* m, const char* ticker) {
    CompanyNode* atual = m->companies_head;
    while (atual != NULL) {
        if (strcmp(atual->ticker, ticker) == 0) return atual;
        atual = atual->next;
    }
    return NULL;
}

/* Insere uma nova empresa na lista, mantendo a ordem alfabetica por ticker.
   Devolve o novo no, ou NULL se ja existir ou os dados forem invalidos. */
static CompanyNode* inserir_empresa(Market* m,
                                    const char* ticker,
                                    const char* nome,
                                    const char* sector) {
    /* Validar tamanhos */
    if (strlen(ticker) >= MAX_TICKER) return NULL;
    if (strlen(nome)   >= MAX_NAME)   return NULL;
    if (strlen(sector) >= MAX_SECTOR) return NULL;

    /* Encontrar o lugar certo na lista ordenada */
    CompanyNode** ptr = &m->companies_head;
    while (*ptr != NULL) {
        int cmp = strcmp((*ptr)->ticker, ticker);
        if (cmp == 0) return NULL;  /* ja existe */
        if (cmp > 0)  break;        /* encontramos onde inserir */
        ptr = &(*ptr)->next;
    }

    /* Criar o novo no */
    CompanyNode* novo = (CompanyNode*) malloc(sizeof(CompanyNode));
    if (novo == NULL) { terminar_com_erro(0, m); return NULL; }

    strcpy(novo->ticker, ticker);
    strcpy(novo->name,   nome);
    strcpy(novo->sector, sector);
    novo->quotes_count = 0;
    novo->quotes_head  = NULL;
    novo->next         = *ptr;  /* ligar ao no seguinte */
    *ptr               = novo;  /* o no anterior aponta para o novo */
    m->company_count++;
    return novo;
}

/* Remove uma empresa da lista e liberta toda a sua memoria */
static int remover_empresa(Market* m, CompanyNode* alvo) {
    CompanyNode** ptr = &m->companies_head;
    while (*ptr != NULL) {
        if (*ptr == alvo) {
            CompanyNode* a_apagar = *ptr;
            *ptr = a_apagar->next;
            libertar_cotacoes(a_apagar->quotes_head);
            free(a_apagar);
            m->company_count--;
            return 1;
        }
        ptr = &(*ptr)->next;
    }
    return 0;
}


/* ============================================================
   VERIFICACAO DE FALENCIA
   Uma empresa vai a falencia se os ULTIMOS 6 closes estiverem
   em queda estrita: c1 > c2 > c3 > c4 > c5 > c6
   ============================================================ */
static int esta_falida(CompanyNode* empresa) {
    /* Precisa de pelo menos 6 cotacoes */
    if (empresa == NULL || empresa->quotes_count < 6) return 0;

    /* Saltar as cotacoes mais antigas ate ficar com as ultimas 6 */
    int a_saltar = empresa->quotes_count - 6;
    QuoteNode* ptr = empresa->quotes_head;
    for (int i = 0; i < a_saltar && ptr != NULL; i++) {
        ptr = ptr->next;
    }

    /* Agora verificar se cada close e maior que o seguinte */
    QuoteNode* anterior = ptr;
    if (anterior == NULL) return 0;
    ptr = ptr->next;
    int verificacoes = 5;

    while (ptr != NULL && verificacoes > 0) {
        /* Se nao esta em queda estrita, nao esta falida */
        if (!(anterior->close > ptr->close)) return 0;
        anterior = ptr;
        ptr      = ptr->next;
        verificacoes--;
    }

    return verificacoes == 0 ? 1 : 0;
}

/* Verifica se a empresa faliu e, se sim, remove-a.
   O parametro 'anunciar' controla se imprime o cabecalho MARKET.PURGE.BANKRUPT */
static void verificar_e_remover_falida(Market* m, CompanyNode* empresa, int anunciar) {
    if (empresa == NULL) return;
    if (esta_falida(empresa)) {
        if (anunciar) printf("MARKET.PURGE.BANKRUPT\n");
        PRINT_BANKRUPT(empresa->ticker);
        remover_empresa(m, empresa);
    }
}


/* ============================================================
   OPERACOES SOBRE COTACOES
   ============================================================ */

/* Adiciona uma cotacao nova, ou atualiza uma existente (regra do duplicado).
   A lista fica sempre ordenada por data (crescente). */
static int inserir_ou_atualizar_cotacao(Market* m, CompanyNode* empresa,
                                        int dia, float open, float high,
                                        float low, float close, int volume) {
    if (empresa == NULL) return -1;

    QuoteNode** ptr = &empresa->quotes_head;

    /* Percorrer a lista para encontrar o lugar certo */
    while (*ptr != NULL) {
        if ((*ptr)->day == dia) {
            /* Ja existe cotacao para este dia: aplicar desconto de 1% */
            (*ptr)->close = (*ptr)->close * 0.99f;
            return 1;
        }
        if ((*ptr)->day > dia) break; /* inserir antes deste */
        ptr = &(*ptr)->next;
    }

    /* Criar nova cotacao */
    QuoteNode* nova = (QuoteNode*) malloc(sizeof(QuoteNode));
    if (nova == NULL) { terminar_com_erro(0, m); return -1; }

    nova->day    = dia;
    nova->open   = open;
    nova->high   = high;
    nova->low    = low;
    nova->close  = close;
    nova->volume = volume;
    nova->next   = *ptr;
    *ptr         = nova;
    empresa->quotes_count++;
    return 1;
}


/* ============================================================
   PARSER DE CSV (separador ";")
   Divide uma linha pelos pontos e virgula.
   Devolve o numero de campos encontrados, ou -1 se houver mais
   campos do que o maximo permitido.
   ============================================================ */
static int dividir_por_ponto_virgula(char* linha, char* campos[], int max_campos) {
    int total = 0;
    char* ptr = linha;
    if (*ptr == '\0') return 0;
    campos[total++] = ptr;
    while (*ptr != '\0') {
        if (*ptr == ';') {
            *ptr = '\0'; /* termina o campo atual */
            if (total < max_campos) {
                campos[total++] = ptr + 1;
            } else {
                return -1; /* campos a mais */
            }
        }
        ptr++;
    }
    return total;
}


/* ============================================================
   CARREGAR FICHEIRO CSV
   Le um ficheiro .csv e insere os dados no mercado.
   Linhas com "C" sao empresas; linhas com "Q" sao cotacoes.
   ============================================================ */
static int carregar_csv(Market* m, const char* caminho) {
    FILE* ficheiro = fopen(caminho, "r");
    if (ficheiro == NULL) return 0; /* nao conseguiu abrir */

    char linha[1024];

    while (fgets(linha, sizeof(linha), ficheiro) != NULL) {
        char copia[1024];
        char* campos[16];
        int num_campos;

        remover_newline(linha);
        if (linha[0] == '\0') continue; /* linha vazia, ignorar */

        strcpy(copia, linha);
        num_campos = dividir_por_ponto_virgula(copia, campos, 16);
        if (num_campos <= 0) continue;

        /* --- Linha de empresa: C;TICKER;NOME;SECTOR --- */
        if (strcmp(campos[0], "C") == 0) {
            if (num_campos != 4) { fclose(ficheiro); terminar_com_erro(0, m); }

            const char* ticker = campos[1];
            const char* nome   = campos[2];
            const char* sector = campos[3];

            if (*ticker == '\0' || *nome == '\0' || *sector == '\0') {
                fclose(ficheiro); terminar_com_erro(0, m);
            }

            /* Inserir so se ainda nao existir */
            if (encontrar_empresa(m, ticker) == NULL) {
                inserir_empresa(m, ticker, nome, sector);
            }

        /* --- Linha de cotacao: Q;TICKER;DIA;OPEN;HIGH;LOW;CLOSE;VOLUME --- */
        } else if (strcmp(campos[0], "Q") == 0) {
            if (num_campos != 8) { fclose(ficheiro); terminar_com_erro(0, m); }

            const char* ticker = campos[1];
            if (!e_inteiro_positivo(campos[2])) { fclose(ficheiro); terminar_com_erro(0, m); }
            if (!e_decimal_positivo(campos[3]))  { fclose(ficheiro); terminar_com_erro(0, m); }
            if (!e_decimal_positivo(campos[4]))  { fclose(ficheiro); terminar_com_erro(0, m); }
            if (!e_decimal_positivo(campos[5]))  { fclose(ficheiro); terminar_com_erro(0, m); }
            if (!e_decimal_positivo(campos[6]))  { fclose(ficheiro); terminar_com_erro(0, m); }
            if (!e_inteiro_positivo(campos[7])) { fclose(ficheiro); terminar_com_erro(0, m); }

            int   dia    = atoi(campos[2]);
            float open   = (float)atof(campos[3]);
            float high   = (float)atof(campos[4]);
            float low    = (float)atof(campos[5]);
            float close  = (float)atof(campos[6]);
            int   volume = atoi(campos[7]);

            CompanyNode* empresa = encontrar_empresa(m, ticker);
            if (empresa == NULL) continue; /* empresa desconhecida, ignorar */

            inserir_ou_atualizar_cotacao(m, empresa, dia, open, high, low, close, volume);
            /* Verificar falencia SEM anunciar cabecalho (isso so acontece em MARKET.PURGE.BANKRUPT) */
            verificar_e_remover_falida(m, empresa, 0);

        } else {
            fclose(ficheiro);
            terminar_com_erro(0, m);
        }
    }

    fclose(ficheiro);
    return 1;
}


/* ============================================================
   TOKENIZADOR DE STDIN (separador: espaco)
   Divide uma linha de texto pelos espacos em branco.
   ============================================================ */
static int dividir_por_espacos(char* linha, char* tokens[], int max_tokens) {
    int total = 0;
    char* ptr = linha;

    /* Ignorar espacos iniciais */
    while (*ptr == ' ') ptr++;
    if (*ptr == '\0') return 0;

    tokens[total++] = ptr;

    while (*ptr != '\0') {
        if (*ptr == ' ') {
            *ptr = '\0';
            ptr++;
            while (*ptr == ' ') ptr++; /* ignorar espacos extra */
            if (*ptr == '\0') break;
            if (total >= max_tokens) return -1;
            tokens[total++] = ptr;
        } else {
            ptr++;
        }
    }
    return total;
}


/* ============================================================
   COMANDOS
   ============================================================ */

/* LOAD <ficheiro.csv> */
static void cmd_load(Market* m, int n, char* tokens[]) {
    if (n != 2) terminar_com_erro(0, m);
    if (carregar_csv(m, tokens[1])) {
        PRINT_OK();
    } else {
        terminar_com_erro(1, m); /* nao conseguiu abrir o ficheiro */
    }
}

/* COMPANY.LIST */
static void cmd_listar_empresas(Market* m, int n) {
    if (n != 1) terminar_com_erro(0, m);
    PRINT_COMPANIES_HEADER(m->company_count);
    CompanyNode* atual = m->companies_head;
    while (atual != NULL) {
        PRINT_COMPANY(atual->ticker, atual->name, atual->sector, atual->quotes_count);
        atual = atual->next;
    }
}

/* QUOTE.LIST <TICKER> ALL | RANGE <inicio> <fim> */
static void cmd_listar_cotacoes(Market* m, int n, char* tokens[]) {
    if (n < 3) terminar_com_erro(0, m);

    CompanyNode* empresa = encontrar_empresa(m, tokens[1]);
    if (empresa == NULL) terminar_com_erro(2, m);

    if (strcmp(tokens[2], "ALL") == 0) {
        /* Listar todas as cotacoes */
        if (n != 3) terminar_com_erro(0, m);
        PRINT_QUOTES_HEADER(empresa->ticker, empresa->quotes_count);
        QuoteNode* q = empresa->quotes_head;
        while (q != NULL) {
            PRINT_QUOTE(q->day, q->close);
            q = q->next;
        }

    } else if (strcmp(tokens[2], "RANGE") == 0) {
        /* Listar cotacoes num intervalo de datas */
        if (n != 5) terminar_com_erro(0, m);
        if (!e_inteiro_positivo(tokens[3]) || !e_inteiro_positivo(tokens[4]))
            terminar_com_erro(0, m);

        int inicio = atoi(tokens[3]);
        int fim    = atoi(tokens[4]);

        /* Se os limites vierem trocados, trocar internamente */
        if (inicio > fim) { int temp = inicio; inicio = fim; fim = temp; }

        /* Contar primeiro para imprimir o cabecalho correto */
        int contagem = 0;
        QuoteNode* q = empresa->quotes_head;
        while (q != NULL) {
            if (q->day >= inicio && q->day <= fim) contagem++;
            q = q->next;
        }

        PRINT_QUOTES_HEADER(empresa->ticker, contagem);
        q = empresa->quotes_head;
        while (q != NULL) {
            if (q->day >= inicio && q->day <= fim) PRINT_QUOTE(q->day, q->close);
            q = q->next;
        }

    } else {
        terminar_com_erro(0, m);
    }
}

/* QUOTE.ADD <TICKER> <DIA> <OPEN> <HIGH> <LOW> <CLOSE> <VOLUME> */
static void cmd_adicionar_cotacao(Market* m, int n, char* tokens[]) {
    if (n != 8) terminar_com_erro(3, m);

    if (!e_inteiro_positivo(tokens[2])) terminar_com_erro(3, m);
    if (!e_decimal_positivo(tokens[3]))  terminar_com_erro(3, m);
    if (!e_decimal_positivo(tokens[4]))  terminar_com_erro(3, m);
    if (!e_decimal_positivo(tokens[5]))  terminar_com_erro(3, m);
    if (!e_decimal_positivo(tokens[6]))  terminar_com_erro(3, m);
    if (!e_inteiro_positivo(tokens[7])) terminar_com_erro(3, m);

    int   dia    = atoi(tokens[2]);
    float open   = (float)atof(tokens[3]);
    float high   = (float)atof(tokens[4]);
    float low    = (float)atof(tokens[5]);
    float close  = (float)atof(tokens[6]);
    int   volume = atoi(tokens[7]);

    /* Validacao OHLC: high e o maximo, low e o minimo */
    if (high < open || high < close || high < low)  terminar_com_erro(3, m);
    if (low  > open || low  > close)                terminar_com_erro(3, m);

    CompanyNode* empresa = encontrar_empresa(m, tokens[1]);
    if (empresa == NULL) terminar_com_erro(3, m);

    inserir_ou_atualizar_cotacao(m, empresa, dia, open, high, low, close, volume);
    PRINT_OK();
    /* Verificar falencia ANUNCIANDO o cabecalho */
    verificar_e_remover_falida(m, empresa, 1);
}

/* QUOTE.REMOVE <TICKER> <DIA> */
static void cmd_remover_cotacao(Market* m, int n, char* tokens[]) {
    if (n != 3) terminar_com_erro(0, m);
    if (!e_inteiro_positivo(tokens[2])) terminar_com_erro(0, m);

    CompanyNode* empresa = encontrar_empresa(m, tokens[1]);
    if (empresa == NULL) terminar_com_erro(3, m);

    int dia = atoi(tokens[2]);

    QuoteNode** ptr = &empresa->quotes_head;
    while (*ptr != NULL) {
        if ((*ptr)->day == dia) {
            QuoteNode* a_apagar = *ptr;
            *ptr = a_apagar->next;
            free(a_apagar);
            empresa->quotes_count--;
            PRINT_OK();
            return;
        }
        ptr = &(*ptr)->next;
    }

    /* Dia nao encontrado: comportamento idempotente, diz OK na mesma */
    PRINT_OK();
}

/* QUOTE.REMOVE.RANGE <TICKER> <INICIO> <FIM> */
static void cmd_remover_cotacoes_range(Market* m, int n, char* tokens[]) {
    if (n != 4) terminar_com_erro(0, m);
    if (!e_inteiro_positivo(tokens[2]) || !e_inteiro_positivo(tokens[3]))
        terminar_com_erro(0, m);

    CompanyNode* empresa = encontrar_empresa(m, tokens[1]);
    if (empresa == NULL) terminar_com_erro(3, m);

    int inicio = atoi(tokens[2]);
    int fim    = atoi(tokens[3]);
    if (inicio > fim) { int temp = inicio; inicio = fim; fim = temp; }

    int removidas = 0;
    QuoteNode** ptr = &empresa->quotes_head;
    while (*ptr != NULL) {
        if ((*ptr)->day >= inicio && (*ptr)->day <= fim) {
            QuoteNode* a_apagar = *ptr;
            *ptr = a_apagar->next;
            free(a_apagar);
            empresa->quotes_count--;
            removidas++;
        } else {
            ptr = &(*ptr)->next;
        }
    }
    PRINT_REMOVED(removidas);
}

/* COMPANY.ADD <TICKER> <NOME> <SECTOR> */
static void cmd_adicionar_empresa(Market* m, int n, char* tokens[]) {
    if (n != 4) terminar_com_erro(0, m);

    const char* ticker = tokens[1];
    const char* nome   = tokens[2];
    const char* sector = tokens[3];

    if (encontrar_empresa(m, ticker) != NULL) terminar_com_erro(4, m);
    if (strlen(ticker) >= MAX_TICKER) terminar_com_erro(0, m);
    if (strlen(nome)   >= MAX_NAME)   terminar_com_erro(0, m);
    if (strlen(sector) >= MAX_SECTOR) terminar_com_erro(0, m);

    if (inserir_empresa(m, ticker, nome, sector) == NULL)
        terminar_com_erro(4, m);

    PRINT_OK();
}

/* COMPANY.EDIT <TICKER> <NOVO_NOME> <NOVO_SECTOR> */
static void cmd_editar_empresa(Market* m, int n, char* tokens[]) {
    if (n != 4) terminar_com_erro(0, m);

    const char* nome   = tokens[2];
    const char* sector = tokens[3];

    if (strlen(nome)   >= MAX_NAME)   terminar_com_erro(0, m);
    if (strlen(sector) >= MAX_SECTOR) terminar_com_erro(0, m);

    CompanyNode* empresa = encontrar_empresa(m, tokens[1]);
    if (empresa == NULL) terminar_com_erro(5, m);

    strcpy(empresa->name,   nome);
    strcpy(empresa->sector, sector);
    PRINT_OK();
}

/* COMPANY.REMOVE <TICKER> */
static void cmd_remover_empresa(Market* m, int n, char* tokens[]) {
    if (n != 2) terminar_com_erro(0, m);

    CompanyNode* empresa = encontrar_empresa(m, tokens[1]);
    if (empresa == NULL) terminar_com_erro(6, m);

    remover_empresa(m, empresa);
    PRINT_OK();
}

/* MARKET.PURGE.BANKRUPT - remove todas as empresas falidas */
static void cmd_purgar_falidas(Market* m, int n) {
    if (n != 1) terminar_com_erro(0, m);

    printf("MARKET.PURGE.BANKRUPT\n");

    CompanyNode* atual = m->companies_head;
    while (atual != NULL) {
        CompanyNode* proxima = atual->next;
        if (esta_falida(atual)) {
            PRINT_BANKRUPT(atual->ticker);
            remover_empresa(m, atual);
        }
        atual = proxima;
    }
}


/* ============================================================
   FUNCOES AUXILIARES PARA INVEST.SUGGEST
   ============================================================ */

/* Devolve o preco de fecho mais recente (ultimo no da lista) */
static double ultimo_close(const CompanyNode* empresa) {
    if (empresa->quotes_head == NULL) return -1.0;
    QuoteNode* ptr = empresa->quotes_head;
    while (ptr->next != NULL) ptr = ptr->next;
    return (double)ptr->close;
}

/* Soma o volume total de todas as cotacoes */
static double volume_total(const CompanyNode* empresa) {
    double soma = 0.0;
    QuoteNode* ptr = empresa->quotes_head;
    while (ptr != NULL) { soma += ptr->volume; ptr = ptr->next; }
    return soma;
}

/* Calcula a media dos closes de todas as cotacoes */
static double media_close(const CompanyNode* empresa) {
    if (empresa->quotes_count == 0) return 0.0;
    double soma = 0.0;
    QuoteNode* ptr = empresa->quotes_head;
    while (ptr != NULL) { soma += ptr->close; ptr = ptr->next; }
    return soma / empresa->quotes_count;
}

/* Comparador para ordenar de forma DESCENDENTE (maior metrica primeiro) */
static int comparar_descendente(const void* a, const void* b) {
    const Candidate* ca = (const Candidate*)a;
    const Candidate* cb = (const Candidate*)b;
    if (ca->metric < cb->metric) return  1;
    if (ca->metric > cb->metric) return -1;
    /* Empate: ordem alfabetica do ticker */
    return strcmp(ca->company->ticker, cb->company->ticker);
}

/* Comparador para ordenar de forma ASCENDENTE (menor metrica primeiro) */
static int comparar_ascendente(const void* a, const void* b) {
    const Candidate* ca = (const Candidate*)a;
    const Candidate* cb = (const Candidate*)b;
    if (ca->metric > cb->metric) return  1;
    if (ca->metric < cb->metric) return -1;
    return strcmp(ca->company->ticker, cb->company->ticker);
}

/* INVEST.SUGGEST <DINHEIRO> <ESTRATEGIA>
   Estrategia 1: Top Volume (maior volume acumulado)
   Estrategia 2: Blue Chips (maior media de close)
   Estrategia 3: Bargain Hunters (close mais recente mais baixo) */
static void cmd_sugerir_investimento(Market* m, int n, char* tokens[]) {
    if (n != 3) terminar_com_erro(0, m);
    if (!e_inteiro_positivo(tokens[1])) terminar_com_erro(0, m);
    if (!e_inteiro_positivo(tokens[2])) terminar_com_erro(0, m);

    int dinheiro  = atoi(tokens[1]);
    int estrategia = atoi(tokens[2]);
    if (estrategia < 1 || estrategia > 3) terminar_com_erro(0, m);

    /* Contar quantas empresas tem pelo menos 1 cotacao */
    int total = 0;
    CompanyNode* atual = m->companies_head;
    while (atual != NULL) {
        if (atual->quotes_count > 0) total++;
        atual = atual->next;
    }

    if (total == 0) { PRINT_SUGGEST_HEADER(0); return; }

    /* Criar array de candidatos */
    Candidate* candidatos = (Candidate*) malloc(sizeof(Candidate) * total);
    if (candidatos == NULL) { terminar_com_erro(0, m); return; }

    int i = 0;
    atual = m->companies_head;
    while (atual != NULL) {
        if (atual->quotes_count > 0) {
            candidatos[i].company = atual;
            if      (estrategia == 1) candidatos[i].metric = volume_total(atual);
            else if (estrategia == 2) candidatos[i].metric = media_close(atual);
            else                      candidatos[i].metric = ultimo_close(atual);
            i++;
        }
        atual = atual->next;
    }

    /* Ordenar conforme a estrategia */
    if (estrategia == 3) {
        qsort(candidatos, total, sizeof(Candidate), comparar_ascendente);
    } else {
        qsort(candidatos, total, sizeof(Candidate), comparar_descendente);
    }

    /* Escolher as top 5 (ou menos se nao houver 5) */
    int top = total < 5 ? total : 5;
    PRINT_SUGGEST_HEADER(top);

    /* Cada parte = dinheiro / 5 */
    int parte = dinheiro / 5;

    for (i = 0; i < top; i++) {
        double preco    = ultimo_close(candidatos[i].company);
        int    quantidade = 0;
        if (preco > 0.0) {
            quantidade = (int)((double)parte / preco);
        }
        PRINT_SUGGESTION(candidatos[i].company->ticker, quantidade);
    }

    free(candidatos);
}


/* ============================================================
   DESPACHADOR DE COMANDOS
   Le uma linha de texto e chama a funcao correta.
   Devolve 0 quando encontra "END", 1 para continuar.
   ============================================================ */
static int processar_comando(Market* m, char* linha) {
    char buffer[1024];
    char* tokens[16];
    int n;

    remover_newline(linha);
    if (linha[0] == '\0') return 1; /* linha vazia: ignorar */
    if (strcmp(linha, "END") == 0) return 0; // terminar

    strcpy(buffer, linha);
    n = dividir_por_espacos(buffer, tokens, 16);
    if (n <= 0) terminar_com_erro(0, m);

    if      (strcmp(tokens[0], "LOAD")                == 0) cmd_load(m, n, tokens);
    else if (strcmp(tokens[0], "COMPANY.LIST")        == 0) cmd_listar_empresas(m, n);
    else if (strcmp(tokens[0], "QUOTE.LIST")          == 0) cmd_listar_cotacoes(m, n, tokens);
    else if (strcmp(tokens[0], "QUOTE.ADD")           == 0) cmd_adicionar_cotacao(m, n, tokens);
    else if (strcmp(tokens[0], "QUOTE.REMOVE")        == 0) cmd_remover_cotacao(m, n, tokens);
    else if (strcmp(tokens[0], "QUOTE.REMOVE.RANGE")  == 0) cmd_remover_cotacoes_range(m, n, tokens);
    else if (strcmp(tokens[0], "COMPANY.ADD")         == 0) cmd_adicionar_empresa(m, n, tokens);
    else if (strcmp(tokens[0], "COMPANY.EDIT")        == 0) cmd_editar_empresa(m, n, tokens);
    else if (strcmp(tokens[0], "COMPANY.REMOVE")      == 0) cmd_remover_empresa(m, n, tokens);
    else if (strcmp(tokens[0], "MARKET.PURGE.BANKRUPT")==0) cmd_purgar_falidas(m, n);
    else if (strcmp(tokens[0], "INVEST.SUGGEST")      == 0) cmd_sugerir_investimento(m, n, tokens);
    else terminar_com_erro(0, m); /* comando desconhecido */

    return 1;
}
int main(void) {
    Market mercado = { NULL, 0 }; /* mercado comeca vazio */
    char linha[1024];

    /* Ler o primeiro comando: deve ser MODE FUNC ou MODE Pandora */
    if (fgets(linha, sizeof(linha), stdin) == NULL)
        terminar_com_erro(0, &mercado);
    remover_newline(linha);

    if (strcmp(linha, "MODE FUNC") == 0) {
        /* Modo de testes: processar comandos ate receber END */
        while (fgets(linha, sizeof(linha), stdin) != NULL) {
            if (!processar_comando(&mercado, linha)) break;
        }

    } else if (strcmp(linha, "MODE Pandora") == 0) {
        /* Modo Pandora: ler o nome do ficheiro CSV a carregar */
        if (fgets(linha, sizeof(linha), stdin) == NULL) {
            PRINT_ERROR_PANDORA();
            libertar_mercado(&mercado);
            return 0;
        }
        remover_newline(linha);
        if (linha[0] == '\0' || !carregar_csv(&mercado, linha)) {
            PRINT_ERROR_PANDORA();
            libertar_mercado(&mercado);
            return 0;
        }
        PRINT_OK();

    } else {
        /* Modo desconhecido */
        terminar_com_erro(0, &mercado);
    }

    libertar_mercado(&mercado);
    return 0;
}
