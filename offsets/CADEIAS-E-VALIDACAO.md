# Cadeias e validacao da implementacao v37

Notacao: `read64(x)` le um ponteiro de oito bytes no endereco x. Campos escalares nao devem ser desreferenciados como ponteiros. As leituras atuais usam MemProcFS com NOCACHE e conferem o total recebido.

## Ambiente, entidades e proxies

```
env      = read64(base(CryEntitySystem.dll) + 0x164F50)
system   = read64(env + 0xD8)
renderer = read64(env + 0x48)
game     = read64(env + 0xC8)
table    = read64(system + 0xC0)
entity   = read64(table + (entityId & 0xFFFF) * 8)
```

O limite de indice e o WORD em system+0x40136. Entity+0x18 deve coincidir com o ID completo, incluindo a parte alta. Entity+0x40 e Matrix34, doze floats em ordem de linhas. Entity+0xE8 aponta para o nome; o codigo solicita 96 bytes.

Proxy vem de entity+0xB0. Se o BYTE entity+0xC tem bit 0x40, a implementacao exige count em entity+0xB8 positivo e desreferencia o primeiro elemento do vetor. Valida proxy vtable contra CryEntitySystem+0x110580 e owner em proxy+0x50 contra entity. A subinterface IRenderNode fica em proxy+8; sua vtable esperada e CryEntitySystem+0x110780.

## Atores, HP e equipes

```
framework   = read64(game + 0x48)
actorSystem = read64(framework + 0x4E0)
rules       = read64(framework + 0x790)
actorHeader = read64(actorSystem + 0x30)
teamHeader  = read64(rules + 0x110)
root        = read64(header + 8)
```

Arvores: esquerda +0, direita +0x10, flag sentinel +0x19. No actor node, ID +0x20 e ator +0x28; no team node, ID +0x1C e equipe +0x20. Nao misturar os dois layouts. Vtables/metodos esperados constam no catalogo e sao validados sem executar funcoes remotas.

Vida: `bits = read32(actor+0x60) XOR read32(actor+0x64)`; reinterpretar bits como float, sem conversao numerica inteira para float. A implementacao trata chave zero como morto e rejeita floats invalidos/fora do limite. O HP exibido e ceil(valor), nao porcentagem. Vida maxima e armadura nao sao conhecidas aqui.

## Esqueleto

```
slotsBegin = read64(proxy + 0x58)
slot       = read64(slotsBegin)
character  = read64(slot + 0x78)
definition = read64(character + 0xB58)
joints     = read64(definition + 0x18)
count      = read32(definition + 0x28)
poses      = read64(character + 0xB78)
name       = read64(joints + jointIndex*0x148 + 0x130)
pose       = poses + jointIndex*0x1C
```

Pose: quaternion xyzw nos primeiros quatro floats, translacao float3 a partir de +0x10. Posicao mundial do osso = Matrix34 da entidade aplicada a essa translacao. A contagem e confrontada com `(read32(joints-4) & 0x3FFFFFFF)`. Character vtable e validada contra CryAnimation+0x27AEF8.

Nao fixar Head=13: o codigo busca `Bip01 Head` e os demais nomes no arquivo JSON. Cache de metadados so e reutilizada quando ID, character, definition, joints e count coincidem. Ponteiro poses e relido e reconferido por amostra.

## Camera e WorldToScreen

Validar renderer vtable CryRenderD3D9+0x3D6648. Estrutura de thread em read64(renderer+0x9360), indices DWORD +0x28 e +0x2C; priorizar o segundo. Para lane 0 ou 1:

```
viewStack = read64(renderer + lane*0x318 + 0xE98)
projStack = read64(renderer + lane*0x318 + 0xEA0)
view      = read64(viewStack)
proj      = read64(projStack)
viewport  = renderer + 0x98C0
```

Matrizes: dezesseis floats, coluna major na funcao projection.transform. `clip = projection * view * world`, dividir xyz por w positivo. Viewport sao quatro int (x,y,largura,altura). A projecao aplica o comportamento percentual do cliente e escala para a saida logica 1920x1080. A formula completa esta em projection.hpp; copiar uma formula generica ignorando viewport pode deslocar o ESP.

Raio central usa inversa de projection*view e origem obtida da inversa da view. Formas fisicas sao testadas localmente por collision-ray.hpp. Nao ha chamada de raycast dentro do jogo.

## Visibilidade

```
frame          = read32(base(Cry3DEngine.dll)+0x3776D8)
temp           = read64(proxy+0x40)
visibleFrame   = read32(temp+0x60)
occludedFrame  = read32(temp+0x64)
```

Ponteiro temp, vtable e tamanhos recebidos sao reconferidos. Na v37, o contador global e relido apos o scatter; exige avancar entre 0 e 16 quadros sem regressao. Visible requer registro de 0 a 2 quadros e mais recente que o de oclusao. Oclusao recente mais nova ou empatada prevalece; falta de informacao e Unknown. Cor pode conservar o estado confirmado por 120 ms, mas essa conservacao nao autoriza trigger. Detalhes e wraparound estao em visibility.hpp. Isso nao comprova LOS ate uma parte individual do corpo.

## Colisao

`physical = read64(character+0xB38)`; em physical, posicao mundial +0xA8, quat xyzw +0xB4, array de partes +0x120 e count int +0x128. Partes tem stride 0x88. Parte: posicao +0, quat +0xC, escala +0x1C, descritor +0x38, ID de parte +0x48. `shape=read64(descriptor)`.

Sphere, triangle mesh, capsule e box sao reconhecidas pelas vtables listadas no catalogo. A identificacao por vtable depende da build. Geometria de vertices e indices e descoberta; transformacoes sao relidas por amostra. A parte da cabeca e associada ao joint descoberto pelo nome.

Transformacao: `world = entityRotation * (partPosition + partRotation * (local * scale)) + entityPosition`. Quaternions validos e escala positiva sao exigidos. Mesh usa indices int32, tres por triangulo, e float3 no inicio de cada vertice de stride dinamico. Capsule usa eixo, raio e meia distancia entre os centros das extremidades. Box orientada usa as linhas da base para testar ponto/raio.

As formas sao do cliente. O contorno da cabeca e a projecao da superficie/esfera ou envoltorio dos vertices, nao a confirmacao de dano do servidor. O trigger escolhe a forma inimiga intersectada mais proxima, sem intersecao adicional contra obstaculos do mapa.
