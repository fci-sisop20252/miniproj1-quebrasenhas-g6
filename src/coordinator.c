#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO COORDENADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * 
 * Este programa coordena múltiplos workers para quebrar senhas MD5 em paralelo.
 * O MD5 JÁ ESTÁ IMPLEMENTADO - você deve focar na paralelização (fork/exec/wait).
 * 
 * Uso: ./coordinator <hash_md5> <tamanho> <charset> <num_workers>
 * 
 * Exemplo: ./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 4
 * 
 * SEU TRABALHO: Implementar os TODOs marcados abaixo
 */

#define MAX_WORKERS 16
#define RESULT_FILE "password_found.txt"

/**
 * Calcula o tamanho total do espaço de busca
 * 
 * @param charset_len Tamanho do conjunto de caracteres
 * @param password_len Comprimento da senha
 * @return Número total de combinações possíveis
 */
long long calculate_search_space(int charset_len, int password_len) {
    long long total = 1;
    for (int i = 0; i < password_len; i++) {
        total *= charset_len;
    }
    return total;
}

/**
 * Converte um índice numérico para uma senha
 * Usado para definir os limites de cada worker
 * 
 * @param index Índice numérico da senha
 * @param charset Conjunto de caracteres
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @param output Buffer para armazenar a senha gerada
 */
void index_to_password(long long index, const char *charset, int charset_len, 
                       int password_len, char *output) {
    for (int i = password_len - 1; i >= 0; i--) {
        output[i] = charset[index % charset_len];
        index /= charset_len;
    }
    output[password_len] = '\0';
}

/**
 * Função principal do coordenador
 */
int main(int argc, char *argv[]) {
     // TODO 1: Validar argumentos de entrada
    // Verificar se argc == 5 (programa + 4 argumentos)
    // Se não, imprimir mensagem de uso e sair com código 1

    // IMPLEMENTE AQUI: verificação de argc e mensagem de erro
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <hash_md5> <tamanho> <charset> <num_workers>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s \"900150983cd24fb0d6963f7d28e17f72\" 3 \"abc\" 4\n", argv[0]);
        exit(1);
    }
    // Parsing dos argumentos (após validação)
    const char *target_hash = argv[1];
    int password_len = atoi(argv[2]);
    const char *charset = argv[3];
    int num_workers = atoi(argv[4]);
    int charset_len = strlen(charset);

    // TODO 1: Adicionar validações dos parâmetros
    // - password_len deve estar entre 1 e 10
    // - num_workers deve estar entre 1 e MAX_WORKERS
    // - charset não pode ser vazio

    if (password_len < 1 || password_len > 10) {
        fprintf(stderr, "Erro: tamanho da senha deve estar entre 1 e 10.\n");
        exit(1);
    }
    if (num_workers < 1 || num_workers > MAX_WORKERS) {
        fprintf(stderr, "Erro: número de workers deve estar entre 1 e %d.\n", MAX_WORKERS);
        exit(1);
    }
    if (charset_len == 0) {
        fprintf(stderr, "Erro: charset não pode ser vazio.\n");
        exit(1);
    }


    printf("=== Mini-Projeto 1: Quebra de Senhas Paralelo ===\n");
    printf("Hash MD5 alvo: %s\n", target_hash);
    printf("Tamanho da senha: %d\n", password_len);
    printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
    printf("Número de workers: %d\n", num_workers);

    // Calcular espaço de busca total
    long long total_space = calculate_search_space(charset_len, password_len);
    printf("Espaço de busca total: %lld combinações\n\n", total_space);

    // Remover arquivo de resultado anterior se existir
    unlink(RESULT_FILE);

    // Registrar tempo de início
    time_t start_time = time(NULL);

    // TODO 2: Dividir o espaço de busca entre os workers
    // Calcular quantas senhas cada worker deve verificar
    // DICA: Use divisão inteira e distribua o resto entre os primeiros workers

    // IMPLEMENTE AQUI:
    // long long passwords_per_worker = ?
    long long passwords_per_worker = total_space / num_workers;
    // long long remaining = ?
    long long remaining = total_space % num_workers;

    // Arrays para armazenar PIDs dos workers
    //pid_t workers[MAX_WORKERS];

    // TODO 3: Criar os processos workers usando fork()
    printf("Iniciando %d workers...\n", num_workers);
    pid_t workers[MAX_WORKERS];
     // IMPLEMENTE AQUI: Loop para criar workers
    for (int i = 0; i < num_workers; i++) {
           // TODO 4: Calcular intervalo de senhas para este worker
    //start_index é o primeiro índice que o worker vai testar
    //end_index é o último índice que ele deve testar
        long long start_index = (long long)i * passwords_per_worker + (i < remaining ? i : remaining);
        long long passwords_for_this_worker = passwords_per_worker + (i < remaining ? 1 : 0);
        long long end_index = start_index + passwords_for_this_worker - 1;
        // TODO 5: Converter indices para senhas de inicio e fim
    //converte os indices numericos para sehas reais
        char start_pass[64], end_pass[64];
        index_to_password(start_index, charset, charset_len, password_len, start_pass);
        index_to_password(end_index, charset, charset_len, password_len, end_pass);
        // TODO 6: Usar fork() para criar processo filho (pid)
        pid_t pid = fork();
        if (pid < 0) {
            perror("Erro no fork");
            exit(1);
        } 
        // TODO 6: Usar fork() para criar processo filho (pid)
        
        else if (pid > 0) {
            workers[i] = pid;
            printf("Worker %d iniciado (PID = %d) range [%s - %s]\n", i, pid, start_pass, end_pass);
        } 
        // TODO 8: No processo filho: usar execl() para executar worker
        else {
            char len_arg[16], id_arg[16];
            snprintf(len_arg, sizeof(len_arg), "%d", password_len);
            snprintf(id_arg, sizeof(id_arg), "%d", i);
            execl("./worker", "worker", target_hash, start_pass, end_pass, charset, len_arg, id_arg, (char *)NULL);
            perror("Erro no execl");
            exit(1);
        }
        // TODO 9: Tratar erros de fork() e execl()
    //feito o TODO 9 na linha 148 e na linha 161.
    }
    
    printf("\nTodos os workers foram iniciados. Aguardando conclusão...\n");
    // TODO 10: Aguardar todos os workers terminarem usando wait()
    // IMPORTANTE: O pai deve aguardar TODOS os filhos para evitar zumbis
    //COMEÇAR AQUIII!!!
    // TODO 11
    // IMPLEMENTE AQUI:
    // - Loop para aguardar cada worker terminar
    // - Usar wait() para capturar status de saída
    // - Identificar qual worker terminou
    // - Verificar se terminou normalmente ou com erro
    // - Contar quantos workers terminaram
    for (int i = 0; i < num_workers; i++) {
        int status;
        pid_t child_pid = waitpid(workers[i], &status, 0);

        if (WIFEXITED(status)) {
            printf("Worker PID %d terminou com o código %d\n", child_pid, WEXITSTATUS(status));
        }
    }

    // Registrar tempo de fim
    time_t end_time = time(NULL);
    double elapsed_time = difftime(end_time, start_time);
    (void)elapsed_time;

    printf("\n=== Resultado ===\n");
    printf("Tempo total de execução: %.2f segundos.\n", elapsed_time);

    // TODO 12: Verificar se algum worker encontrou a senha
    // Ler o arquivo password_found.txt se existir

    // IMPLEMENTE AQUI:

    int fd = open(RESULT_FILE, O_RDONLY);
    if (fd < 0) {
        printf("Nenhuma senha foi encontrada por nenhum worker.\n");
    } else {
        char buffer[256];
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            char *colon = strchr(buffer, ':');
            if (colon != NULL) {
                *colon = '\0';
                int worker_id = atoi(buffer);
                char *password = colon + 1;
                password[strcspn(password, "\r\n")] = 0;
                printf("O worker %d encontrou a senha: %s\n", worker_id, password);
                char final_hash[33];
                md5_string(password, final_hash);
                printf("Hash verificado: %s\n", final_hash);
            }
        }
        close(fd);
    }

    return 0;
}
