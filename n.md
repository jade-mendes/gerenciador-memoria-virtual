## 📘 Documentação da Linguagem N

### Índice

1. [Introdução](#introdução)
2. [Sintaxe Geral](#sintaxe-geral)
3. [Tipos e Variáveis](#tipos-e-variáveis)
4. [Operações Aritméticas](#operações-aritméticas)
5. [Memória Virtual](#memória-virtual)
6. [Entrada e Saída](#entrada-e-saída)
7. [Controle de Fluxo](#controle-de-fluxo)
8. [Processos](#processos)
9. [Execução e Depuração](#execução-e-depuração)
10. [Erros Comuns](#erros-comuns)
11. [Exemplos de Código](#exemplos-de-código)

---

### Introdução

A linguagem **N** é uma linguagem de baixo nível com sintaxe simples, usada para simular execução de processos com memória virtual. Ela é orientada a instruções diretas e semântica próxima de um assembly de alto nível.

---

### Sintaxe Geral

* Cada instrução termina obrigatoriamente com ponto e vírgula `;`.
* Uma linha pode conter **apenas uma instrução**.
* Comentários iniciam com `//`.
* A linguagem é **case sensitive** (`Create` ≠ `create`).
* Os nomes de variáveis podem ter até **7 caracteres**.

---

### Tipos e Variáveis

#### Tipos Suportados:

* `int`: Número inteiro de 4 bytes.
* `string`: Sequência de caracteres (armazenada em memória como bytes).

#### Regras:

* Atribuições declaram variáveis automaticamente.
* Não há escopo: todas variáveis são globais ao processo.
* Variáveis são mapeadas a endereços de memória virtual via um **hashmap interno**.
* Se uma variável for usada antes de ser inicializada, o processo é **terminado imediatamente**.

#### Exemplo:

```n
valor = 42;
texto = "Oi!";
```

---

### Operações Aritméticas

Suporta apenas os operadores `+` e `-`.
Cada linha pode conter apenas **uma operação**.

```n
soma = valor + 10;
texto = texto - 4; // Lê os 4 primeiros bytes de texto como int e subtrai 4
```

* Não há verificação de tipo: somar `int` com `string` resultará na leitura dos 4 bytes da string como `int`.
* Não há conversões de tipo explícitas.
* Não há precedência de operadores (um por linha).

---

### Memória Virtual

```n
variavel = mmap(endereco, tamanho);
```

* Aloca `tamanho` bytes no endereço virtual especificado.
* A realocação no mesmo endereço não altera a execução.
* A linguagem diferencia entre valor e ponteiro:

```n
&tmp = mem1 + 10;   // tmp passa a apontar para mem1 + 10
tmp = &tmp - 5;     // tmp passa a ter o valor do endereço (tmp - 5)
```

---

### Entrada e Saída

#### Entrada:

```n
input_n(variavel);        // Lê número inteiro
input_s(variavel, tam);   // Lê string (até tam bytes)
```

* `input_s` **sempre exige** o segundo argumento.
* Se `input_n` falhar ao converter a entrada, o processo é encerrado.

#### Saída:

```n
print_n(var);     // Imprime como número
print_p(var);     // Imprime valor em formato hexadecimal
print_p(&var);    // Imprime o endereço apontado
print_s(var);     // Imprime string
```

---

### Controle de Fluxo

Baseado em **labels** e **jumps condicionais**:

```n
label(loop);
    res = res - 1;
    jump_gt(loop, res, 0);
```

#### Jumps disponíveis:

* `jump_eq(label, a, b)` — salta se `a == b`
* `jump_neq(label, a, b)` — salta se `a != b`
* `jump_gt(label, a, b)` — salta se `a > b`
* `jump_lt(label, a, b)` — salta se `a < b`

---

### Processos

#### Criação:

```n
Create(nome); // Procura o arquivo ./process/nome
```

* Cada processo é um arquivo `.n` separado.
* O simulador executa os processos em uma **fila de execução**, com paralelismo simulado.
* Processos **não compartilham memória** entre si.

#### Término:

```n
Terminate(); // Encerra o processo atual
```

* Um processo não possui subprocessos. Todos têm mesma hierarquia.

---

### Execução e Depuração

* O simulador executa a linguagem **ciclo a ciclo**, visível na interface.
* É o próprio mecanismo de depuração.
* Toda execução falha é detectada em tempo de simulação (leitura inválida, variáveis não declaradas, input malformado, etc).

---

### Erros Comuns

| Erro                       | Consequência                     |
|----------------------------|----------------------------------|
| Uso de variável não criada | Processo é terminado             |
| Falha ao converter input   | Processo é terminado             |
| Instrução malformada       | Processo é suspenso ou bloqueado |
| Acesso inválido na memória | Processo é encerrado             |

---

### Exemplos de Código

**Criar múltiplos processos:**

```n
cont = mmap(0x1000, 4);

&vezes = "Quantos vezes você quer criar o processo test?";

print_s(vezes);
input_n(cont);

label(loop1);
    cont = cont - 1;
    Create(test);
    jump_gt(loop1, cont, 0);
```

---

Posso agora te entregar esse conteúdo como um arquivo `.md` (Markdown), `.pdf` ou outro formato se quiser. Deseja que eu gere esse arquivo? Quer adicionar mais seções (como estilo, boas práticas, etc)?
