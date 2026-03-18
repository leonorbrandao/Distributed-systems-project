# README Projeto Etapa 4: Sistemas Distribuídos 2025/2026

Grupo SD-22\
Autores:
- Guilherme Diniz, Nº 61824
- Henrique Pinto, Nº 61805
- Maria Leonor, Nº 61779

## 1. Descrição Geral
Este projeto consiste no desenvolvimento de um sistema de gestão de inventário de um stand automóvel, desenvolvido em C.\
Esta etapa do projeto estende o sistema desenvolvido nas fases
anteriores, adicionando tolerância a falhas através da **replicação em
cadeia (Chain Replication)** com coordenação através do **ZooKeeper**.

O sistema suporta:

-   Múltiplos servidores replicados numa cadeia (head → ... → tail)
-   Propagação síncrona de operações de escrita
-   Leituras consistentes no servidor tail
-   Deteção automática de falhas usando ZNodes efémeros
-   Entrada dinâmica de novos servidores com sincronização inicial ao antecessor
-   Watchers no ZooKeeper para se adaptar devido a mudanças na cadeia

Os clientes comunicam com o **head** para escritas e com o **tail** para
leituras. Cada servidor mantém uma ligação ao Zookeeper e ao seu servidor sucessor (caso exista).


## 2. Estrutura do Projeto

O ficheiro ZIP entregue segue a estrutura:

    SD-22/
    ├── binary/          # executáveis (list_server e list_client)
    ├── include/         # headers (.h)
    ├── lib/             # para armazenar bibliotecas
    ├── object/          # ficheiros .o (gerados pelo make)
    ├── source/          # ficheiros fonte (.c)
    ├── sdmessage.proto  # definição das mensagens com protobuf
    ├── README.md
    └── Makefile

## 3. Compilação
O programa assume que os packages necessários para a compilação já estão instalados.


Para compilar:

    make

Para limpar ficheiros objeto, executáveis e logs:

    make clean

Os executáveis serão colocados em:

    binary/

## 4. Execução

Antes de correr servidores ou clientes, é necessário o ZooKeeper estar ativo:

    zkServer.sh start
Por default o ZooKeeper usa o porto 2181.

### 4.1 Servidor

Formato:

    ./binary/list_server <server_port> <zookeeper_ip:port>

Exemplo:

    ./binary/list_server 12345 127.0.0.1:2181

Ao iniciar, o servidor:

-   Liga-se ao ZooKeeper
-   Cria o seu ZNode efémero sequencial em `/chain`
-   Determina o antecessor e sucessor
-   Sincroniza o estado inicial com o antecessor (se existir)
-   Liga-se ao sucessor (se existir)
-   Ativa watchers aos filhos de `/chain` para se adaptar a reconfigurações na cadeia

### 4.2 Cliente

Formato:

    ./binary/list_client <zookeeper_ip:port>

Exemplo:

    ./binary/list_client 127.0.0.1:2181

O cliente:

-   Liga-se ao ZooKeeper
-   Descobre o servidor **head** (id mais baixo)
-   Descobre o servidor **tail** (id mais alto)
-   Ativa watchers aos filhos de `/chain`
-   Envia:
    -   pedidos que alteram a lista (`add`, `remove`, etc.) → servidor head
    -   pedidos de consultas (`get_by_year`, `size`, etc.) → servidor tail

Se existir apenas um servidor ativo, o cliente liga-se duas vezes ao
mesmo servidor.

## 5. Log do Servidor

Cada servidor gera:

    ip:port.log

Comportamento nesta fase:

-   **Head** regista apenas operações de modificação oriundas dos
    clientes
-   **Servidores intermédios** registam apenas modificações propagadas
-   **Tail** regista apenas operações de consulta

## 7. Limitações Conhecidas

-   O sistema assume que não existem operações pendentes durante falhas
    (como definido no enunciado)
-   Falhas simultâneas múltiplas não são garantidamente tratadas

## 8. Notas Finais

-   Os ficheiros `sdmessage.pb-c.*` já estão incluídos
-   O Makefile foi atualizado com dependências do ZooKeeper
