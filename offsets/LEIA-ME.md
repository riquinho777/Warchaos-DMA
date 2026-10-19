# Offsets e estruturas — Warchaos v39

Pasta destinada a acompanhar o projeto quando entregue a outro desenvolvedor. Descreve a implementacao salva na v39, sem alterar o executavel. Nao e um dump novo de offsets de outra versao do jogo.

## Arquivos

- `CATALOGO.md`: 119 constantes nomeadas, agrupadas por modulo e estrutura.
- `offsets-v39.hpp`: as mesmas constantes em namespaces C++17, para consulta ou adaptacao. O motor atual continua usando os valores do codigo-fonte original; este header nao foi integrado ao motor.
- `offsets-v39.json`: catalogo estruturado, categoria, explicacao, referencia de codigo e SHA256 dos oito binarios locais fornecidos para a analise.
- `inventario-hex-fonte.json`: cada linha com literal hexadecimal nos arquivos ativos de leitura/projecao. Tambem inclui limites, mascaras e tamanhos; nao significa que cada numero seja um offset.
- `CADEIAS-E-VALIDACAO.md`: como seguir ponteiros, resolver dados dinamicos e interpretar campos.
- `nomes-dos-ossos.json`: nomes usados pela descoberta do esqueleto; indices sao resolvidos em execucao.

## Interpretacao

Os enderecos de modulo sao RVAs: endereco atual = base do modulo + RVA. Bases de DLL e ponteiros de personagens sao resolvidos em execucao e nao devem ser copiados de logs de uma partida. `field` e relativo ao objeto indicado, `vtable_slot` e deslocamento em bytes na vtable. `stride`, `mask`, `read_size`, `validation_bound` e `type_id` sao categorias distintas de enderecos.

O SHA256 identifica a copia em disco analisada, nao comprova que o processo atual carregou um arquivo identico. Revalidar em caso de mudanca de cliente. Os arquivos do jogo nao foram incluidos nesta pasta; seus nomes e hashes permitem identificar a build de referencia.

O codigo-fonte correspondente esta em `../projeto/`. A referencia completa de comportamento continua sendo esse codigo. O inventario inclui tambem caminhos de diagnostico presentes no mesmo arquivo; esses caminhos nao sao todos usados no loop do ESP.

## Estado conhecido

Entidades, equipes, nomes, HP, pose, camera e formas de colisao sao usados na v39. A visibilidade e um cache do personagem completo; nao e linha de visao da cabeca nem raycast das paredes. Nenhum offset de uma estrutura de obstaculos do mapa e apresentado como validado. Vida maxima e armadura nao foram implementadas. A identificacao local ainda compara o nickname `Drevok` em `Reader.discover`; outro desenvolvedor deve revisar isso se mudar de conta.

O layout de memoria nao mudou entre v37 e v39. A referencia anterior foi preservada em referencia-v37/.


