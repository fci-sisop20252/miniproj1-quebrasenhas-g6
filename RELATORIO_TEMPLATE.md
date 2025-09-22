# Relatório: Mini-Projeto 1 - Quebra-Senhas Paralelo

**Aluno(s):** Beatriz Di Palma (10439477), Guilherme Limeira De Souza (10439777), Luiza Marinho de Mesquita (10438045), Vinicius Pereira Allegretti (10437502)  
---

## 1. Estratégia de Paralelização


**Como você dividiu o espaço de busca entre os workers?**

O número total de combinações de senhas (total_space) é dividido igualmente pelo número de workers (num_workers). Isso resulta em um "tamanho de bloco" base (passwords_per_worker) que cada worker irá processar. Se a divisão não for perfeita o resto da divisão (remaining) é calculado. Para distribuir essas senhas restantes, cada um dos primeiros remaining workers recebe uma senha extra para verificar.

**Código relevante:** Cole aqui a parte do coordinator.c onde você calcula a divisão:
```c
// TODO 2: Dividir o espaço de busca entre os workers
// Calcular quantas senhas cada worker deve verificar
    // long long passwords_per_worker = ?
    long long passwords_per_worker = total_space / num_workers;
    // long long remaining = ?
    long long remaining = total_space % num_workers;
// TODO 4: Calcular intervalo de senhas para este worker
    long long start_index = (long long)i * passwords_per_worker + (i < remaining ? i : remaining);
    long long passwords_for_this_worker = passwords_per_worker + (i < remaining ? 1 : 0);
    long long end_index = start_index + passwords_for_this_worker - 1;
```

---

## 2. Implementação das System Calls

**Descreva como você usou fork(), execl() e wait() no coordinator:**

Nós usamos um loop for para criar todos os workers. Dentro do loop, a chamada fork() criava um processo filho. No código do processo pai, nós guardávamos o PID do filho recém-criado em um array e o loop continuava para criar o próximo. Já no código do processo filho, nós preparávamos os argumentos que ele precisaria, como sua faixa de senhas, e então usávamos execl() para que ele se transformasse no programa worker. Após criar todos os workers, o pai entrava em um segundo loop que usava waitpid() para esperar por cada um dos PIDs que tínhamos guardado, garantindo que o programa só continuasse depois que todos os workers tivessem finalizado suas tarefas.

**Código do fork/exec:**
```c
    // TODO 3: Criar os processos workers usando fork()
    printf("Iniciando %d workers...\n", num_workers);

     // IMPLEMENTE AQUI: Loop para criar workers
    for (int i = 0; i < num_workers; i++) {

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
```

---

## 3. Comunicação Entre Processos

**Como você garantiu que apenas um worker escrevesse o resultado?**

Para garantir que apenas um worker escrevesse o resultado, nós usamos a técnica de criação de arquivo atômica. Na função save_result do worker, a chamada open() foi feita com as flags O_CREAT e O_EXCL juntas. Isso faz com que a criação do arquivo só funcione se ele ainda não existir. Dessa forma, o primeiro worker que encontra a senha consegue criar o arquivo password_found.txt com sucesso, mas qualquer outro que tente fazer o mesmo depois terá a chamada open() falhando. Isso evita uma condição de corrida e garante que apenas o primeiro resultado seja salvo.

**Como o coordinator consegue ler o resultado?**

[Explique como o coordinator lê o arquivo de resultado e faz o parse da informação]

Foi implementada a comunicação entre os workers e o coordenador através do sistema de arquivos. Depois que o coordenador esperava todos os workers terminarem, ele simplesmente tentava abrir o arquivo password_found.txt. Se o arquivo existisse, significava que uma senha foi encontrada. O coordenador então lia o conteúdo do arquivo, que estava no formato "ID:senha", e usava funções como strchr() e atoi() para separar e interpretar essas informações, podendo assim exibir o resultado final na tela.

## 4. Análise de Performance
Complete a tabela com tempos reais de execução:
O speedup é o tempo do teste com 1 worker dividido pelo tempo com 4 workers.

| Teste | 1 Worker | 2 Workers | 4 Workers | Speedup (4w) |
|-------|----------|-----------|-----------|--------------|
| Hash: 202cb962ac59075b964b07152d234b70<br>Charset: "0123456789"<br>Tamanho: 3<br>Senha: "123" | 0m0.005s | 0m0.007s | 0m0.016s | 0.31x |
| Hash: 5d41402abc4b2a76b9719d911017c592<br>Charset: "abcdefghijklmnopqrstuvwxyz"<br>Tamanho: 5<br>Senha: "hello" | 0m4.656s |0m7.910s | 0m1.960s | 4.25x |

**O speedup foi linear? Por quê?**
[Analise se dobrar workers realmente dobrou a velocidade e explique o overhead de criar processos]

O speedup para o primeiro teste foi de 0.31x, o que representa uma lentidão. Isso é esperado porque a tarefa de encontrar a senha "123" é extremamente rápida. O custo (overhead) de criar e gerenciar 4 processos paralelos foi maior do que o tempo economizado na busca, fazendo com que a versão paralela fosse mais lenta do que a versão com um único worker.
No teste com a senha "Hello", nós observamos um speedup de 4.25x com 4 workers, um resultado que foi melhor que o linear. Isso aconteceu porque, ao dividir a tarefa, o trabalho de cada worker se tornou mais eficiente para o processador gerenciar. Com um único worker, a tarefa era grande demais, forçando-o a buscar a senha em todo o espaço de busca. Ao dividir o trabalho entre quatro workers, a tarefa de cada um ficou pequena o suficiente para caber inteiramente em sua própria "mesa de trabalho". Isso eliminou as idas lentas ao espaço de busca, fazendo com que cada worker fosse muito mais rápido e eficiente, o que resultou em um ganho de performance geral superior ao esperado.

## 5. Desafios e Aprendizados
**Qual foi o maior desafio técnico que você enfrentou?**

Nosso maior desafio técnico foi fazer a chamada execl() funcionar corretamente. Nós tivemos bastante dificuldade em entender como passar os argumentos do coordenador para os workers, pois precisamos aprender que todos eles, incluindo os números, tinham que ser convertidos para strings antes de serem passados. Além disso, a ordem dos argumentos era crucial, e a lista precisava obrigatoriamente terminar com NULL. Debugar isso foi complicado, pois os workers falhavam sem um erro claro, até que finalmente entendemos que a "ponte" de comunicação entre os processos estava com defeito e conseguimos montar a lista de argumentos da forma correta.
---

## Comandos de Teste Utilizados

```bash
# Teste básico
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Teste de performance
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 1
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4

# Teste com senha maior
time ./coordinator "5d41402abc4b2a76b9719d911017c592" 5 "abcdefghijklmnopqrstuvwxyz" 4
```
---

**Checklist de Entrega:**
- [x] Código compila sem erros
- [x] Todos os TODOs foram implementados
- [x] Testes passam no `./tests/simple_test.sh`
- [x] Relatório preenchido
