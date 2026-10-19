# Versao 38 — continuidade da mira

Relato que orientou a revisao: a mira puxa por instantes e para mesmo com o inimigo exposto. A politica anterior rejeitava o alvo assim que o estado do cache mudava para Unknown, embora a apresentacao pudesse conservar a cor por 120 ms.

Agora a politica de mira pode manter por ate 80 ms o mesmo ID anteriormente confirmado Visible quando o estado atual e Unknown. Usa somente a posicao recebida no quadro atual: nao reutiliza a coordenada anterior, nao adquire novos IDs desconhecidos e nao ignora leitura atrasada. O prazo parte do timestamp do ultimo quadro confirmado; repetir um quadro nao prolonga esse prazo.

Occluded explicito interrompe imediatamente a elegibilidade do alvo, assim como soltura do botao, fim do prazo, saida da FOV e remocao/morte do ator. Se outro alvo confirmado e elegivel, a selecao normal pode considera-lo. A politica do trigger continua exigindo a visibilidade atual quando seu filtro esta ligado; a tolerancia da mira nao autoriza disparo.

Isso cobre lacunas curtas do cache. Nao torna esse cache um raycast de paredes e nao garante distinguir um inimigo exposto de uma oclusao ainda nao registrada pelo motor. Posicoes novas e ID constante sao obrigatorios durante a tolerancia.

Ganho 0.4 (forca 5), max_step 40, zona morta, FOV, botoes, cores e offsets foram preservados na copia inicial da v38. O pacote original v37 permanece separado. O loader desta pasta abre app/esp-fuser-v38.exe. O rodape mostra v38 e acrescenta o motivo atual da mira. Logs AIM contam botao ativo, alvo elegivel, movimentos, lacunas e leituras atrasadas.

Validacao: compilacao x64 do motor e loader; suite da politica anterior; testes novos de ID/posicao atual, limite exato de 80 ms, repeticao de quadro, aquisicao desconhecida, oclusao imediata, alvo removido, FOV, atraso, botao e trigger independente; verificacao de arquivos, interface, previa e lancamento do motor para listar monitores. Todos passaram. Nenhum movimento/clique foi emitido por esses testes. A sensacao do acompanhamento ainda precisa de teste em partida.
