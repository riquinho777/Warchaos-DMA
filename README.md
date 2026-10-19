# Warchaos — código-fonte v39

Fontes do loader Windows (Dear ImGui / DirectX 11) e do aplicativo de leitura DMA com integração serial Ferrum, configurações e catálogo de offsets.

## Compilar

1. Instale Visual Studio Build Tools com C++ e Windows SDK.
2. Obtenha os headers e o runtime compatíveis do [MemProcFS](https://github.com/ufrisk/MemProcFS) e [LeechCore](https://github.com/ufrisk/LeechCore), observando suas licenças.
3. Abra o Developer Command Prompt **x64**.
4. Defina `MEMPROCFS_INCLUDE` para a pasta com `vmmdll.h` e `leechcore.h`.
5. Execute `projeto\Compilar.cmd`. O script gera `app\esp-fuser-v39.exe` e `Loader.exe`.
6. Disponibilize os runtimes necessários em `app` conforme a documentação das dependências. Este repositório não redistribui DLLs proprietárias ou drivers.

## Organização

- `projeto/`: código completo desta versão, scripts e testes.
- `projeto/vendor/imgui/`: Dear ImGui com sua licença original.
- `offsets/`: catálogo e estruturas versionadas.
- `documentacao/`: notas das versões 38 e 39.

Abra `Loader.exe` após compilar. Insert abre/fecha o menu. Feche leitores antigos antes de usar a placa DMA.

## Estado

Snapshot histórico da versão 39. Não foi recompilado nem testado em partida durante a preparação deste repositório. Offsets dependem da versão do jogo; não são atualizados automaticamente. Visible check usa o cache do cliente, sem garantia de raycast geométrico completo. Requer hardware e dependências compatíveis, não sendo uma aplicação pronta para qualquer PC.

Logs de jogadores, capturas de memória, configurações pessoais, credenciais e binários não fazem parte desta publicação.

O código original é disponibilizado sob MIT (arquivo LICENSE). Dependências de terceiros mantêm suas próprias licenças; essa licença não concede direitos sobre o jogo ou seus recursos.
