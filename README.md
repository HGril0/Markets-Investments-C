
MerCInvest
Programa em C para gerir empresas cotadas em bolsa e as suas cotações diárias.

Projeto da UC de Linguagens de Programação 1 (2025/2026), Universidade Lusófona.

O que faz
Guarda empresas (ordenadas por ticker) e cotações (ordenadas por data) em listas ligadas, com memória gerida à mão (malloc/free).
Lê comandos do stdin: LOAD, COMPANY.LIST/ADD/EDIT/REMOVE, QUOTE.LIST/ADD/REMOVE, MARKET.PURGE.BANKRUPT, INVEST.SUGGEST.
Carrega dados de um CSV separado por ;.
Deteta empresas falidas (6 fechos consecutivos a descer).
INVEST.SUGGEST <dinheiro> <estratégia> sugere até 5 empresas: maior volume, maior média de fecho ou menor último fecho.
Em caso de erro, escreve ERROR <código>, liberta a memória e termina.
Como correr
gcc main.c -o mercinvest
./mercinvest
Escreve MODE FUNC e depois os comandos, até END.

O que aprendi
Listas ligadas, gestão manual de memória, parsing de texto e CSV, ordenação com critérios de desempate.

