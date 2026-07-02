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

Depois de implementar essa interface com a memtable, comecei a implementar a busca nas SSTables, que é o próximo passo do projeto. Implementei, rodei testes e corrigi alguns bugs que apareceram
O bug mais feio que apareceu foi um onde a funçaõ de busca não encontrava a chave por que min_key e max_key nos indexes estavam corrompidos, isso fazia com que a função tentasse comparar a chave com lixo de memória. O problema era causado atribuindo os ponteiros de min_key e max_key da memtable para a sstable sem strdup, então quando a memtable era limpa, os ponteiros da sstable apontavam para lixo de memória. Corrigi isso usando strdup e agora o bug é outro. Durante a busca, a função não usa fseek para pular valores que não condizem com o que ela quer, isso causa desalinhamento de ponteiro do arquivo, além disso, os testes alocavam strings gigantescas de "A" para testar o flush, isso fazia com que a função de busca tentasse ler um valor gigante do arquivo, causando estouro do buffer de leitura e corrompendo o restante dos dados. Corrigi isso usando fseek para pular os valores que não condizem com a chave buscada e alocando um buffer de leitura do tamanho do valor lido do arquivo, assim evitando estouro de buffer e desalinhamento de ponteiro.

Depois desses lindo bugs, descobri que os dados dos nós não estavam sendo gravados no arquivo, então revisei a função que escrevia esses nós no arquivo, e encontrei a seguinte condição:
```
    if (!context || !context->file || !context->error || !node) return;
```
Vi "!context->error" e quis morrer, então corrigi para "context->error" (sem o !) e agora os dados estão sendo gravados corretamente no arquivo.

Com o CRUD da memtable e das SSTables implementado, comecei a trabalhar no Bloom Filter visto que é o menos complexo dos requisitos que faltam e depois de implementar ele eu posso só congelar o sistema e trabalhar com o merge K-vias depois.

Pra entender o Bloom Filter, eu usei o Gemini com o prompt:

```
Vou começar a implementar o Bloom Filter, me explica como funciona para ter uma ideia do que fazer
```

Ele me explicou detalhadamente o funcionamento do Bloom Filter, incluindo como ele usa múltiplas funções de hash para mapear elementos em um array de bits e como ele pode ter falsos positivos, mas nunca falsos negativos. Ele também me deu um roadmap para implementar o Bloom Filter em C.

Depois de pedir pra ele me mostrar alguns exemplos de como mover os bits em C, ele me mostrou uns algorítmos de hash, eu sinceramente fiquei meio em choque por que pra mim a lógica do Bloom Filter parecia algo meio sobrenatural, depois de pedir pra ele me explicar a lógica matemática por trás e explorar a fundo o Bloom Filter eu entendi melhor o funcionamento.

Depois de entender melhor a lógica do Bloom Filter, eu confesso que peguei os algorítmos de hash que ele me mostrou antes, depois estudei melhor a implementação das funções add e check, e implementei o Bloom Filter em C, testei e corrigi alguns bugs que apareceram.

Com o Bloom Filter implementado e integrado, comecei a trabalhar no WAL (Write-Ahead Logging). A ideia do WAL é registrar todas as operações de escrita antes de aplicá-las na memtable, garantindo que, em caso de falha, possamos recuperar o estado da memtable a partir do log. Foi um pouco problemático, pois acabou gerando loop infinito (O WAL chamava a função de insert, e a função de insert escrevia no WAL). Resolvi isso separando a escrita de WAL e dumping pra SSTable pra uma função de interface pro usuário, assim a memtable_insert só lidava com a memtable e era cega para o resto dos componentes

Depois de reescrever o insert da memtable, comecei a implementar funções de interface de usuário, as que vão ser executadas pelo menu, foi fácil até, pedi ajuda pro Gemini pra implementar um perf counter usando o prompt:
```
Queria implementar um perf timer, como faço isso?
```

Ele me deu um bloco de código que usava CLOCK_MONOTONIC, mas por ser uma definição exclusiva dos kernels POSIX eu pedi pra ele me dar uma alternativa que fosse cross-platform, então ele me deu um bloco de código que usava #ifdef _WIN32 para Windows e #else para Linux, e eu implementei o perf counter cross-platform com sucesso.

Com o perf counter implementado, comecei a implementar as funções de interface de usuário, que chamam as funções da memtable e das SSTables, e também fazem logging das operações. Depois de implementar essas funções, comecei a escrever testes para elas e passaram sem problemas.

O próximo passo é implementar o merge k-vias, que é o processo de compactação das SSTables entre os níveis e do display all keys. Mas antes, vou fazer os logs de DEBUG serem opcionais, depois vou fazer um menu para a aplicação, documentar e proteger algumas funções com static e _ antes do nome.

Bug lógico implementando testes: db_select() interpreta valores NULL como não encontrado e manda a responsablidade pras sstables, mas na verdade o valor NULL é um tombstone, então db_select() deveria retornar NULL e não mandar a responsabilidade pras sstables. Corrigi isso usando a struct SearchResult, que tem um campo "found" que indica se a chave foi encontrada ou não, e um campo "value" que indica o valor da chave, que pode ser NULL se a chave for um tombstone.

Pensei melhor e lembrei que entre um boot e outro do banco as sstables existentes não são devidamente indexadas na memória, então parti para implementar a função de indexação das sstables existentes na inicialização do banco, que vai ler os arquivos de sstables do diretório e criar os índices em memória para cada uma delas. Isso vai permitir que o banco funcione corretamente mesmo após reinicializações, sem perder o estado das sstables. Decidi usar a abordagem de um arquivo Manifest, que armazena metadados das tabelas existentes, na sintaxe <Operação><Nível><Número do arquivo><MinKey><MaxKey>. A função de indexação vai ler esse arquivo Manifest e criar os índices em memória para cada sstable existente, garantindo que o banco funcione corretamente mesmo após reinicializações.

Durante a implementação lembrei da situação "E se o tamanho do Bloom Filter mudar entre reinicializações?", então decidi que o arquivo Manifest vai armazenar o tamanho do Bloom Filter, assim a função de indexação vai ler esse tamanho e criar o Bloom Filter com o tamanho correto, garantindo que o banco funcione corretamente mesmo após reinicializações.

Implementei isso aí, criei a função que restaura o WAL e o index das sstables no boot do banco, e agora o banco funciona corretamente mesmo após reinicializações, sem perder o estado das sstables e do Bloom Filter.

Por enquanto é isso aí, hoje é 01/07/2026 as 01:38 da manhã, 13 horas tem prova do Bruno de BST e AVL e eu tô aqui quase tendo um derrame depois de ficar mais de 16 horas mexendo com C. Amanhã depois da prova eu começo Merge K-vias e aproveito ele pra fazer o display all keys, que é o último requisito do projeto. Depois disso, é só gravar aquele vídeo que tá pedindo no documento.

Acordei, fiz um café forte que só o cão e vimn direto pro PC começar a implementar merge k-vias por que eu descobri que ainda tinha um relatório de 8 páginas no mínimo pra entregar. Pra entender melhor o merge k-vias, eu pedi pro Gemini me explicar com o prompt:
```
Me explica melhor o merge k-vias, como ele funciona e como eu lido com as tombstones e chaves repetidas
```

Ele me explicou que basicamente você armazena a raiz num buffer, pega a próxima chave e valor da sstable, compara com a raiz que tá no buffer, se foram diferentes o buffer vai pro destino, se forem iguais você armazena o buffer e libera a chave nova, já que o que está guardado no buffer é o mais recente. Ele também me deu um roadmap de como implementar o merge k-vias em C.

Comecei a implementar, apareceram vários problemas, commitei tudo com [WIP] nos commits e larguei de lado pois precisava ir fazer a prova.
O que falta implementar para o merge k-vias ficar lindo:
- [*] Limitar o tamanho dos arquivos pra 10 * nível * tamanho_da_memtable
- [*] Escrever testes
- [*] Adicionar um watcher que verifica a quantidade de arquivos em cada nível e chama o merge k-vias numa thread paralela quando necessário
- [*] Implementar o display all keys, que vai usar das mesmas funções de iteração k-vias
- [*] Adicionar contador de limite de tamanho de arquivo, na função de compactar sstables
- [*] Implementar o menu do usuário

Acabei de receber a notícia de que o Bruno não vai adiar o trabalho, então provável que essa madrugada seja a base de café e código.
Tô implementando o merge k-vias, reaproveitei alguns códigos do Gemini e do Copilot, o módulo virou uma sopa imanutenível, vou ser obrigado a refatorar

Faltam algumas horas pras apresentações do trabalho e eu ainda preciso verificar memory leaks, fazer o relatório e gravar o vídeo. Então não tenho escrito muito nesse arquivo.
Mas as últimas alterações foram em torno do k-vias iteration, que usa um MinHeap pra iterar pelas sstables, desacoplei o MinHeap do resto, agora o MinHeap é uma estrutura de dados burra que só sabe gerenciar a estrutura de dados, a lógica de negócios é passada pra ele através de callbacks e gerenciada pelo módulo de k-vias iteration. Implementei o level compactor e depois disso o display all keys, o display all keys foi bobeira, reutilizei muito código do compactor, só que ao invés de escrever pra um arquivo eu escrevia pro stdout. Além disso, implementei alguns utilitários de testes.

Implementei uma função recursiva que verifica se as sstables precisam de merge usando a variável de limite de arquivos de SSTable, agora quando a memtable é persistida em disco, a função de verificação é chamada e caso seja necessário, o merge k-vias é chamado (infelizmente na mesma thread por enquanto)

Quando escrevi testes pra essas funções eu descobri uns leaks bizarros, aí eu precisei instalar a libasan pra debuggar, e tome, 5MB de RAM vazados por que eu esquecia de limpar alguns dados nos testes

Implementei um menu pro usuário, compilou, tá tudo funcionando, por incrível que pareça, agora vou implementar paralelismo
Tô conseguindo fazer aqui, tô travando mutex em vários lugares diferentes e eu já tenho uma trhead worker rodando o compactador quando necessário, mas apareceu um bug, por algum motivo desconhecido o L4 das sstables não existe, o código pula do L3 pro L5

Descobri que não tava pulando, só que quando a thread worker tá fazendo merge no stress test é um fluxo de dados tão avassalador que o L4 é criado e deletado tão rápido que parece estar sendo ignorado, pra comprovar eu adicionei uns debugs rapidinho na função, se o nível fosse 4 ele printava uma pensagem lá, aí eu pude ter certeza que ta só o filé a aplicação.

Agora que tá tudo rodando bonitinho, fiz um slide no gamma, vou implementar o teste de benchmark, gravar o vídeo e escrever o relatório, e se Deus quiser, dormir pelo menos 15 minutos pq apresentar virado é sinistro
