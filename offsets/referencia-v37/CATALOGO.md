# Catalogo de offsets v37

Referencia extraida do codigo-fonte empacotado. Status: usado pela implementacao v37, nao garantia para outras builds. Arquivos do jogo identificados por SHA256 em offsets-v37.json.

RVA: somar a base atual do modulo. Campo: somar ao objeto correspondente. Stride, tamanho de leitura, mascara e ID de tipo nao sao enderecos.

## CryEntitySystem

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| global_environment | `0x164F50` | module_rva | Ponteiro global para gEnv em CryEntitySystem.dll |
| render_proxy_vtable | `0x110580` | module_rva | Vtable validada do proxy principal |
| render_node_vtable | `0x110780` | module_rva | Vtable da subinterface IRenderNode em proxy+8 |

## Environment

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| entity_system | `0xD8` | field | gEnv -> EntitySystem |
| renderer | `0x48` | field | gEnv -> renderer |
| game | `0xC8` | field | gEnv -> game |

## EntitySystem

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| entity_table | `0xC0` | field | Ponteiro para tabela de entidades |
| last_index | `0x40136` | field | WORD limite de indice |
| entry_stride | `0x8` | stride | Ponteiro de entidade por indice |
| id_index_mask | `0xFFFF` | mask | Indice = entityId & mascara |

## Entity

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| flags | `0xC` | field | BYTE de flags usado para localizar proxy |
| indirect_proxy_flag | `0x40` | mask | Flag indica proxy indireto |
| id | `0x18` | field | DWORD entity ID completo |
| world_matrix | `0x40` | field | Matrix34: 12 floats em ordem de linhas |
| proxy | `0xB0` | field | Proxy direto ou vetor de proxies conforme flag |
| proxy_count | `0xB8` | field | int numero de proxies no caminho indireto |
| name | `0xE8` | field | Ponteiro para string de nome |
| header_read_size | `0x100` | read_size | Bloco lido para entidade |

## RenderProxy

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| render_node_interface | `0x8` | field | Subinterface IRenderNode e respectiva vtable |
| render_temp | `0x40` | field | Ponteiro para dados temporarios de renderizacao |
| owner | `0x50` | field | Ponteiro para entidade dona |
| slots_begin | `0x58` | field | Inicio do vetor de slots |
| slots_end | `0x60` | field | Fim do vetor de slots |
| header_read_size | `0xA8` | read_size | Bloco coletado do proxy |

## EntitySlot

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| character | `0x78` | field | Slot -> character |

## CryAnimation

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| character_vtable | `0x27AEF8` | module_rva | Vtable usada para validar character |

## Character

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| skeleton_definition | `0xB58` | field | Ponteiro para definicao do esqueleto |
| pose_array | `0xB78` | field | Ponteiro atual para poses; pode mudar entre leituras |

## SkeletonDefinition

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| joints | `0x18` | field | Ponteiro para metadados dos joints |
| joint_count | `0x28` | field | DWORD quantidade de joints |
| count_prefix | `-0x4` | field | DWORD antes do array joints |
| count_mask | `0x3FFFFFFF` | mask | Mascara para validar quantidade no prefixo |
| joint_metadata_stride | `0x148` | stride | Tamanho de metadados por joint |
| joint_name_pointer | `0x130` | field | Ponteiro para nome dentro dos metadados do joint |
| pose_stride | `0x1C` | stride | Pose por joint: sete floats (quaternion e translacao) |
| pose_rotation | `0x0` | field | Quat xyzw na pose |
| pose_translation | `0x10` | field | Float3 de translacao na pose |

## CryRenderD3D9

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| renderer_vtable | `0x3D6648` | module_rva | Vtable usada para validar renderer |

## Renderer

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| render_thread | `0x9360` | field | Ponteiro para estrutura com indices de lanes |
| viewport | `0x98C0` | field | Quatro int: x y largura altura |
| lane_stride | `0x318` | stride | Deslocamento entre as duas lanes |
| view_stack | `0xE98` | field | Por lane: ponteiro para stack que contem ponteiro da matriz view |
| projection_stack | `0xEA0` | field | Por lane: ponteiro para stack que contem ponteiro da matriz projection |
| matrix_pointer_in_stack | `0x0` | field | Primeiro QWORD do stack aponta para 16 floats |
| matrix_read_size | `0x40` | read_size | 16 floats por matriz |

## RenderThread

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| alternate_lane | `0x28` | field | DWORD primeiro indice |
| preferred_lane | `0x2C` | field | DWORD segundo indice; priorizado na selecao |

## Game

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| framework | `0x48` | field | game -> framework |

## Framework

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| actor_system | `0x4E0` | field | framework -> ActorSystem |
| game_rules | `0x790` | field | framework -> GameRules |

## CryAction

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| actor_system_vtable | `0x4F0368` | module_rva | Vtable validada de ActorSystem |
| actor_lookup_method | `0x59340` | module_rva | Metodo validado por igualdade de ponteiro; nao chamado remotamente |

## ActorSystem

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| lookup_vtable_slot | `0x78` | vtable_slot | Offset em bytes na vtable |
| actor_tree_header | `0x30` | field | Ponteiro para header da arvore de atores |

## ActorTree

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| root | `0x8` | field | header -> raiz |
| left | `0x0` | field | Filho esquerdo |
| right | `0x10` | field | Filho direito |
| sentinel_flag | `0x19` | field | BYTE de sentinel/nil |
| entity_id | `0x20` | field | DWORD ID do ator |
| actor_pointer | `0x28` | field | Ponteiro do ator |
| node_read_size | `0x30` | read_size | Bloco do node |

## CryGame

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| health_getter | `0x219A40` | module_rva | Metodo de vida validado; leitura local XOR reproduz a decodificacao |
| game_rules_vtable | `0xBCDF98` | module_rva | Vtable de GameRules |
| get_team_method | `0x595810` | module_rva | Metodo validado por ponteiro; nao chamado remotamente |

## Actor

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| health_vtable_slot | `0xD8` | vtable_slot | Offset do ponteiro GetHealth na vtable |
| health_encoded | `0x60` | field | DWORD bits de vida codificados |
| health_key | `0x64` | field | DWORD chave XOR |
| read_size | `0x68` | read_size | Bloco do ator usado na amostra |

## GameRules

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| get_team_vtable_slot | `0x368` | vtable_slot | Offset na vtable para validar GetTeam |
| team_tree_header | `0x110` | field | Ponteiro para header da arvore de equipes |

## TeamTree

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| root | `0x8` | field | header -> raiz |
| left | `0x0` | field | Filho esquerdo |
| right | `0x10` | field | Filho direito |
| sentinel_flag | `0x19` | field | BYTE de sentinel/nil |
| entity_id | `0x1C` | field | DWORD ID da entidade |
| team_id | `0x20` | field | DWORD ID da equipe |
| node_read_size | `0x28` | read_size | Bloco lido por node |

## Cry3DEngine

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| frame_counter | `0x3776D8` | module_rva | DWORD contador de quadros, lido no lote e reconferido apos a leitura |

## RenderTemp

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| last_visible_frame | `0x60` | field | DWORD do registro recente de visibilidade do personagem |
| last_occluded_frame | `0x64` | field | DWORD do registro recente de oclusao do personagem |

## CharacterPhysics

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| physical_entity | `0xB38` | field | character -> entidade fisica |

## CryPhysics

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| validated_vtable_upper_bound | `0x3D3080` | validation_bound | Limite exclusivo RVA usado na validacao da vtable fisica; nao e campo de objeto |
| sphere_vtable | `0x24CCA8` | module_rva | Vtable reconhecida para sphere |
| mesh_vtable | `0x24E770` | module_rva | Vtable reconhecida para triangle mesh |
| capsule_vtable | `0x24DEA0` | module_rva | Vtable reconhecida para capsule |
| box_vtable | `0x24D6C0` | module_rva | Vtable reconhecida para box |

## PhysicalEntity

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| vtable | `0x0` | field | Ponteiro da vtable |
| position | `0xA8` | field | Float3 de posicao mundial |
| rotation | `0xB4` | field | Quat xyzw da entidade |
| parts | `0x120` | field | Ponteiro para array de partes |
| part_count | `0x128` | field | int quantidade de partes |
| header_read_size | `0x130` | read_size | Bloco de entidade fisica |

## PhysicalPart

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| stride | `0x88` | stride | Bytes por parte |
| position | `0x0` | field | Float3 posicao local |
| rotation | `0xC` | field | Quat xyzw local |
| scale | `0x1C` | field | Float escala |
| geometry_descriptor | `0x38` | field | Ponteiro para descritor |
| part_id | `0x48` | field | DWORD identificado com o joint correspondente |

## GeometryDescriptor

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| shape | `0x0` | field | Primeiro QWORD aponta para a forma IGeometry |
| shape_header_read_size | `0x90` | read_size | Bloco inicial de forma usado na descoberta |

## Sphere

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| center | `0x30` | field | Float3 centro local |
| radius | `0x3C` | field | Float raio |
| type_id | `0x4` | type_id | Identificador de tipo usado na cache local |

## TriangleMesh

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| indices | `0x40` | field | Ponteiro para indices int32; tres por triangulo |
| vertices | `0x58` | field | Ponteiro para vertices |
| vertex_stride | `0x60` | field | int stride em bytes; float3 no inicio de cada vertice |
| triangle_count | `0x80` | field | int quantidade de triangulos |
| vertex_count | `0x84` | field | int quantidade de vertices |
| type_id | `0x1` | type_id | Identificador de tipo usado na cache local |

## Capsule

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| center | `0x30` | field | Float3 centro local |
| axis | `0x3C` | field | Float3 eixo unitario |
| radius | `0x48` | field | Float raio |
| half_length | `0x4C` | field | Float meia distancia entre centros das extremidades |
| type_id | `0x6` | type_id | Identificador de tipo usado na cache local |

## Box

| Nome | Valor | Categoria | Uso |
| --- | --- | --- | --- |
| basis | `0x30` | field | Nove floats: tres linhas da base |
| oriented | `0x54` | field | int indica base orientada |
| center | `0x58` | field | Float3 centro local |
| half_extents | `0x64` | field | Float3 meios tamanhos |
| type_id | `0x0` | type_id | Identificador de tipo usado na cache local |
