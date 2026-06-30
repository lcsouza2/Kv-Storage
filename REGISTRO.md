Comecei o projeto planejando o esqueleto da aplicação e qual parte codificar primeiro, foi decidido começar pelas SSTables visto que são o core de SO e principal mecanismo no armazenamento dos dado

Usei o Gemini para esclarecer alguns conceitos e ajudar con algumas decisões arquiteturais, os prompts foram:

1. O meu é o tema 5, KV-Store com LSM tree, parece complexo e tenho dificuldade com alguns dos requisitos, pode aprofundar os conceitos de memtable e arvores AVL / treap? (PDF anexado)

2. Agora explique níveis de SSTables com índice esparso e Bloom filter; merge k-vias na compactação entre níveis.


Implementando a lib de logging precisei de uma forma de suportar strings formatadas, não sabia como fazer então pedi ajuda ao Gemini com ideias, usei o prompt:

```
*Codigo colado*
Como eu adiciono suport a strings formatadas?
```

Ele sugeriu usar a lib stdarg.h, que é uma lib padrão do C para lidar com argumentos variáveis em funções. Com ela, você pode criar funções que aceitam um número variável de argumentos, permitindo que você formate strings de maneira flexível.

Consegui implementar logging corretamente então parti para implementação das SSTables, implementei uma função para criar uma SSTable, que gera um arquivo físico no disco e retorna uma struct com informações sobre a SSTable criada. A função também gera o caminho do arquivo com base no nível da SSTable e no número de arquivos já criados nesse nível.

Criada e testada essa função, comecei a implementar árvores AVL para armazenar as chaves e valores na memória antes de serem persistidos em disco. O auto complete do Copilot no VSCode implementou algumnas coisas como rotação, mas eu ainda tinha dúvidas sobre o funcionamento do código. Então eu copiei o código gerado e pedi para o Gemini explicar o que cada parte fazia, ele explicou detalhadamente o funcionamento da árvore AVL, incluindo como as rotações funcionam para manter a árvore balanceada. Após entender o código eu separei o que era baixo nível e criei uma interface para a árvore AVL, além de corrigir alguns bugs que estavam presentes no código gerado pelo Copilot.

Com a árvore implementada, comecei a trabalhar na contagem de bytes da árvore e no dumping na SSTable. A contagem de bytes é importante para saber quando a memtable está cheia e precisa ser persistida em disco. Pra isso, precisei alterar algumas coisas na estrutura do nó adaptando ele pra suportar a chave

Adaptando a árvore e os testes, eu tive um problema com referências de ponteiros, fiz uma correção, os testes passaram, então pedi ajuda para o Gemini com edge cases e se tava certa a correção proposta, usei o prompt:
```
Alterei a árvore para suportar strings e recebi esse erro quando compilei e rodei os testes:

❯ make test
gcc -Wall -Wextra -Werror -I./include -o run_tests tests/unit/*.c tests/main.c src/avl_tree.o src/logging.o src/sstables.o  
./run_tests
[INFO]: Running tests...
[INFO]: sstable created: ./data/sstables/L0_0.dat
[INFO]: test_create_sstable_success passed.
free(): invalid pointer
make: *** [Makefile:20: test] Abortado (arquivo core criado)

Revisei o código e percebi que tinha usado:
node->key = key
node->value = value

alterei pra usar:
node->key = strdup(key)
node->value = strdup(value)

sendo essa a assinatura da função:
AVLNode *_create_node(char* key, char *value)

Os testes passaram

Consegue encontrar algum edge case ou erro de uso da linguagem?
```

Ele disse que a correção tava certa e que um possível edge case seria se o strdup tentasse alocar memória e falhasse, retornando NULL.

Enquanto rodava testes percebi que a falta de separação do ambiente de testes causaria um erro grave, os SSTables criadas em um teste eram armazenadas no mesmo diretório que as SSTables criadas em produção, podendo apagar dados reais na limpeza dos testes, então separei isso através de variáveis de ambiente, agora os testes criam as SSTables em um diretório separado do diretório de produção.

Enquanto eu criava as funções para fazer flush da memtable para a SSTable, tive que tomar uma decisão arquitetural "complexa", então pedi ajuda para o Gemini com o prompt:

```
 Preciso tomar uma decisão arquitetural importante e gostaria da sua ajuda para decidir:

A pergunta principal: O flushing dos dados deveria aocntecer nas sstables ou na memtable?

Pense nesse trecho de código:
*Código copiado e colado*

para a função flush eu teria que aplicar IoC ou algo assim passando um endereço de função ou o ponteiro do arquivo para a Memtable iterar pela árvore e ir formatando e adicionando ao arquivo, avaliando essa splução eu teria:

Melhor organização de código, o que é SSTable fica no módulo de SSTable e o que é Memtable fica no módulo de Memtable, em contraponto, isso criaria dispersão de tarefas de uma única função de flushing

Ele concordou com a ideia o IoC, então implementei uma função que percorre a Memtable em ordem e executa um callback para cada nó

Com o dump para o arquivo "pronto", implementei verificação de necessidade desse dump e um index rápido de metadados das sstables em memória.

Refatorei o código das sstables pois tava com um acoplamento absurdo e acabou com a separação de responsabilidades, agora o módulo de sstables tem um core mais estável e fornece uma interface de interação

Com o código refatorado, escrevi testes para o flush e validei o funcionamento corrigindo bugs

Após isso, escrevi funções para buscar e excluir chaves na memtable, não foi um problema tão grande graças, só tive que alterar as funções da árvore AVL pra adicionar tombstones e atualizar onde necessário, além de escrever testes para essas funções e validar o funcionamento