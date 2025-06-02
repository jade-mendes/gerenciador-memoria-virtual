# Documentação da Linguagem N

A linguagem N é uma linguagem de montagem simples projetada para controlar um simulador de memória virtual. Ela oferece instruções básicas para gerenciamento de processos, alocação de memória, operações aritméticas e controle de fluxo.

## Características Gerais
- **Sintaxe**: Baseada em instruções simples com parâmetros
- **Comentários**: Usando `//` para comentários de linha única
- **Limites**: Todos os nomes (variáveis, processos, labels) têm no máximo 7 caracteres
- **Tipos de dados**: Suporte a inteiros (32 bits), ponteiros e strings

## Instruções

### 1. Criação de Processos
`Create(process_name);`  
Cria um novo processo e o adiciona na fila de pronto.

Exemplo:
```n
Create(main);
Create(backgr);
```

### 2. Término de Processos
`Terminate();`  
Termina o processo atualmente em execução.

Exemplo:
```n
Terminate();
```

### 3. Alocação de Memória
`var = mmap(adrr_like, size);`  
Aloca uma variável na memória virtual.

Parâmetros:
- `adrr_like`: Endereço virtual em hexadecimal (ex: `0x1000`)
- `size`: Tamanho em bytes

Exemplo:
```n
mem1 = mmap(0x1000, 32);
mem2 = mmap(0x2000, 64);
```

### 4. Instruções de Impressão
`print_n(var);`  
Imprime o conteúdo como inteiro de 32 bits

`print_p(var);`  
Imprime o endereço (virtual) como ponteiro

`print_s(var);`  
Imprime o conteúdo como string (terminada em `\0`)

Exemplo:
```n
print_n(mem1);
print_p(mem2);
print_s(tmp);
```

### 5. Instruções de Entrada
`input_n(var);`  
Lê um número inteiro do usuário e armazena no endereço da variável

`input_s(var, size);`  
Lê uma string do usuário e armazena no endereço da variável (máximo de `size` bytes)

Exemplo:
```n
input_n(in1);
input_s(in2, 12);
```

### 6. Operações com Variáveis
`var1 = var2;`  
Copia o **valor** de var2 para var1

`&var1 = &var2;`  
Faz var1 **apontar** para o mesmo endereço de var2

`var1 = var2 + num;`  
Soma constante numérica ao **valor** de var2

`&var1 = &var1 + num;`  
Ajusta o **ponteiro** em bytes (ex: `+4` para próximo inteiro)

`var1 = var2 - num;`  
Subtrai constante numérica do **valor** de var2

`var1 = var2 + var3;`  
Soma **valores** de variáveis

`var1 = var2 - var3;`  
Subtrai **valores** de variáveis

Exemplo:
```n
mem1 = mem2;       // Copia valor
&mem1 = &mem2;     // Compartilha endereço
tmp = mem1 + 10;   // Soma valor
&mem1 = &mem1 + 4; // Ajusta ponteiro
res = mem1 + mem2;
dif = mem2 - mem1;
```

### 7. Controle de Fluxo
`label(name);`  
Define um ponto de salto com o nome especificado

`jump(index);`  
Salta para a instrução no índice especificado

`jump_eq(index, var, num);`  
Salta se **valor** de var for igual a num

`jump_eq(index, var1, var2);`  
Salta se **valores** de var1 e var2 forem iguais

Exemplo:
```n
label(loop);
    jump_eq(loop, res, 0);
label(end);
    jump_eq(end, dif, dif);
```

## Regras de Sintaxe
1. Todas as instruções terminam com `;`
2. Nomes são case-sensitive e limitados a 7 caracteres
3. Endereços devem ser especificados em hexadecimal (ex: `0x1000`)
4. Valores numéricos podem ser decimais ou hexadecimais
5. Comentários começam com `//` e vão até o final da linha
6. Operador `&` indica operações com ponteiros/endereços

## Exemplo Completo
```n
// Programa exemplo em linguagem N
Create(main);
Create(backgr);

mem1 = mmap(0x1000, 32);
mem2 = mmap(0x2000, 64);
in1 = mmap(0x3000, 4);
str = mmap(0x4000, 20);

input_n(in1);
input_s(str, 20);

mem1 = mem2;        // Copia valor
&mem1 = &mem2;      // Compartilha endereço
tmp = mem1 + 10;    // Soma valor
&mem1 = &mem1 + 4;  // Ajusta ponteiro
res = mem1 + mem2;
dif = mem2 - mem1;

print_n(in1);
print_p(mem2);
print_s(str);

label(loop);
    print_n(res);
    res = res - 1;
    jump_eq(loop, res, 0);

label(end);
    jump_eq(end, dif, dif);

Terminate();
```

## Compilação e Execução
Para compilar programas em linguagem N:
```bash
./parser programa.n
```

O parser gerará uma lista de instruções interpretáveis pelo simulador de memória virtual.

## Limitações Conhecidas
- Nomes de variáveis/processos limitados a 7 caracteres
- Não suporta operações matemáticas complexas
- Não possui sistema de funções ou subrotinas
- Tipagem estática implícita
- Ponteiros não têm aritmética baseada em tipo (sempre em bytes)

Esta documentação cobre todos os aspectos essenciais da linguagem N para desenvolvimento de programas para o simulador de memória virtual.