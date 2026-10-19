# Versao 39 — interrupcoes de desenho

Log da v38: aproximadamente 10 a 12 falhas de camera/leitura em alguns intervalos de cinco segundos, com picos de leitura entre 80 e 174 ms. O estado visual era preservado por no maximo 24 ms em falha transitoria e apagado ao atingir 60 ms de idade. Esses limites podem produzir apagamentos completos em falhas breves. As contagens de leituras parciais e troca de pose eram zero nos intervalos examinados.

Mudancas:

- A descoberta de personagens deixa de reconstruir todas as malhas/esferas/caixas/capsulas a cada ciclo quando o layout permanece identico. Confere character, entidade fisica, vtable, array/count de partes, IDs, descritores e ponteiros/vtables de formas. Descritores e tabelas sao conferidos em scatter. Mudanca ou falha provoca redescoberta. A cache e descartada quando o ator sai da lista.
- A cache guarda geometria do modelo, nao posicoes: camera e transformacoes das partes continuam relidas no loop. A validacao de amostra permanece ativa. Presume que os vertices da geometria persistente nao sejam modificados no mesmo objeto sem mudanca desses identificadores.
- A retencao de quadro em falha transitoria passa de 24 para 60 ms, sem renovar o timestamp antigo. Apagamento por idade na apresentacao passa de 60 para 90 ms. Durante uma pausa pode haver congelamento visual curto; nao ha extrapolacao.
- O limite de idade da mira/trigger continua 35 ms. Quadros visualmente retidos nao renovam dados para comandos. Falha nao transitoria, troca de equipe/local desconhecido e dados de mortos continuam eliminando marcadores conforme a leitura atual.

Testes passaram: compilacao do motor/loader; limites de retencao; cache com memoria simulada cobrindo acerto, troca de entidade fisica, quantidade, joint, descritor, forma/vtable, falha de leitura e character. A coleta ao vivo encontrou apenas o jogador local e nenhum adversario, portanto nao mediu reaproveitamento real de geometria nem melhora de latencia em partida. Nao houve envio de comandos Ferrum nos testes.

Mira, visible check, configuracoes e offsets de memoria continuam como na v38. A pasta anterior permanece disponivel para retorno. Usar o Loader.exe desta pasta e encerrar o loader/motor anterior antes do teste.
