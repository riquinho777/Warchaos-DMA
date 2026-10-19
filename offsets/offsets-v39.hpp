// C++17 / offsets-v39.hpp / reference catalog only; not included by the saved binary.
#pragma once
#include <cstdint>
#include <cstddef>
namespace warchaos_v39 {
namespace CryEntitySystem {
inline constexpr std::uintptr_t global_environment = 0x164F50; // module_rva: Ponteiro global para gEnv em CryEntitySystem.dll
inline constexpr std::uintptr_t render_proxy_vtable = 0x110580; // module_rva: Vtable validada do proxy principal
inline constexpr std::uintptr_t render_node_vtable = 0x110780; // module_rva: Vtable da subinterface IRenderNode em proxy+8
}
namespace Environment {
inline constexpr std::uintptr_t entity_system = 0xD8; // field: gEnv -> EntitySystem
inline constexpr std::uintptr_t renderer = 0x48; // field: gEnv -> renderer
inline constexpr std::uintptr_t game = 0xC8; // field: gEnv -> game
}
namespace EntitySystem {
inline constexpr std::uintptr_t entity_table = 0xC0; // field: Ponteiro para tabela de entidades
inline constexpr std::uintptr_t last_index = 0x40136; // field: WORD limite de indice
inline constexpr std::uintptr_t entry_stride = 0x8; // stride: Ponteiro de entidade por indice
inline constexpr std::uintptr_t id_index_mask = 0xFFFF; // mask: Indice = entityId & mascara
}
namespace Entity {
inline constexpr std::uintptr_t flags = 0xC; // field: BYTE de flags usado para localizar proxy
inline constexpr std::uintptr_t indirect_proxy_flag = 0x40; // mask: Flag indica proxy indireto
inline constexpr std::uintptr_t id = 0x18; // field: DWORD entity ID completo
inline constexpr std::uintptr_t world_matrix = 0x40; // field: Matrix34: 12 floats em ordem de linhas
inline constexpr std::uintptr_t proxy = 0xB0; // field: Proxy direto ou vetor de proxies conforme flag
inline constexpr std::uintptr_t proxy_count = 0xB8; // field: int numero de proxies no caminho indireto
inline constexpr std::uintptr_t name = 0xE8; // field: Ponteiro para string de nome
inline constexpr std::uintptr_t header_read_size = 0x100; // read_size: Bloco lido para entidade
}
namespace RenderProxy {
inline constexpr std::uintptr_t render_node_interface = 0x8; // field: Subinterface IRenderNode e respectiva vtable
inline constexpr std::uintptr_t render_temp = 0x40; // field: Ponteiro para dados temporarios de renderizacao
inline constexpr std::uintptr_t owner = 0x50; // field: Ponteiro para entidade dona
inline constexpr std::uintptr_t slots_begin = 0x58; // field: Inicio do vetor de slots
inline constexpr std::uintptr_t slots_end = 0x60; // field: Fim do vetor de slots
inline constexpr std::uintptr_t header_read_size = 0xA8; // read_size: Bloco coletado do proxy
}
namespace EntitySlot {
inline constexpr std::uintptr_t character = 0x78; // field: Slot -> character
}
namespace CryAnimation {
inline constexpr std::uintptr_t character_vtable = 0x27AEF8; // module_rva: Vtable usada para validar character
}
namespace Character {
inline constexpr std::uintptr_t skeleton_definition = 0xB58; // field: Ponteiro para definicao do esqueleto
inline constexpr std::uintptr_t pose_array = 0xB78; // field: Ponteiro atual para poses; pode mudar entre leituras
}
namespace SkeletonDefinition {
inline constexpr std::uintptr_t joints = 0x18; // field: Ponteiro para metadados dos joints
inline constexpr std::uintptr_t joint_count = 0x28; // field: DWORD quantidade de joints
inline constexpr std::ptrdiff_t count_prefix = -0x4; // field: DWORD antes do array joints
inline constexpr std::uintptr_t count_mask = 0x3FFFFFFF; // mask: Mascara para validar quantidade no prefixo
inline constexpr std::uintptr_t joint_metadata_stride = 0x148; // stride: Tamanho de metadados por joint
inline constexpr std::uintptr_t joint_name_pointer = 0x130; // field: Ponteiro para nome dentro dos metadados do joint
inline constexpr std::uintptr_t pose_stride = 0x1C; // stride: Pose por joint: sete floats (quaternion e translacao)
inline constexpr std::uintptr_t pose_rotation = 0x0; // field: Quat xyzw na pose
inline constexpr std::uintptr_t pose_translation = 0x10; // field: Float3 de translacao na pose
}
namespace CryRenderD3D9 {
inline constexpr std::uintptr_t renderer_vtable = 0x3D6648; // module_rva: Vtable usada para validar renderer
}
namespace Renderer {
inline constexpr std::uintptr_t render_thread = 0x9360; // field: Ponteiro para estrutura com indices de lanes
inline constexpr std::uintptr_t viewport = 0x98C0; // field: Quatro int: x y largura altura
inline constexpr std::uintptr_t lane_stride = 0x318; // stride: Deslocamento entre as duas lanes
inline constexpr std::uintptr_t view_stack = 0xE98; // field: Por lane: ponteiro para stack que contem ponteiro da matriz view
inline constexpr std::uintptr_t projection_stack = 0xEA0; // field: Por lane: ponteiro para stack que contem ponteiro da matriz projection
inline constexpr std::uintptr_t matrix_pointer_in_stack = 0x0; // field: Primeiro QWORD do stack aponta para 16 floats
inline constexpr std::uintptr_t matrix_read_size = 0x40; // read_size: 16 floats por matriz
}
namespace RenderThread {
inline constexpr std::uintptr_t alternate_lane = 0x28; // field: DWORD primeiro indice
inline constexpr std::uintptr_t preferred_lane = 0x2C; // field: DWORD segundo indice; priorizado na selecao
}
namespace Game {
inline constexpr std::uintptr_t framework = 0x48; // field: game -> framework
}
namespace Framework {
inline constexpr std::uintptr_t actor_system = 0x4E0; // field: framework -> ActorSystem
inline constexpr std::uintptr_t game_rules = 0x790; // field: framework -> GameRules
}
namespace CryAction {
inline constexpr std::uintptr_t actor_system_vtable = 0x4F0368; // module_rva: Vtable validada de ActorSystem
inline constexpr std::uintptr_t actor_lookup_method = 0x59340; // module_rva: Metodo validado por igualdade de ponteiro; nao chamado remotamente
}
namespace ActorSystem {
inline constexpr std::uintptr_t lookup_vtable_slot = 0x78; // vtable_slot: Offset em bytes na vtable
inline constexpr std::uintptr_t actor_tree_header = 0x30; // field: Ponteiro para header da arvore de atores
}
namespace ActorTree {
inline constexpr std::uintptr_t root = 0x8; // field: header -> raiz
inline constexpr std::uintptr_t left = 0x0; // field: Filho esquerdo
inline constexpr std::uintptr_t right = 0x10; // field: Filho direito
inline constexpr std::uintptr_t sentinel_flag = 0x19; // field: BYTE de sentinel/nil
inline constexpr std::uintptr_t entity_id = 0x20; // field: DWORD ID do ator
inline constexpr std::uintptr_t actor_pointer = 0x28; // field: Ponteiro do ator
inline constexpr std::uintptr_t node_read_size = 0x30; // read_size: Bloco do node
}
namespace CryGame {
inline constexpr std::uintptr_t health_getter = 0x219A40; // module_rva: Metodo de vida validado; leitura local XOR reproduz a decodificacao
inline constexpr std::uintptr_t game_rules_vtable = 0xBCDF98; // module_rva: Vtable de GameRules
inline constexpr std::uintptr_t get_team_method = 0x595810; // module_rva: Metodo validado por ponteiro; nao chamado remotamente
}
namespace Actor {
inline constexpr std::uintptr_t health_vtable_slot = 0xD8; // vtable_slot: Offset do ponteiro GetHealth na vtable
inline constexpr std::uintptr_t health_encoded = 0x60; // field: DWORD bits de vida codificados
inline constexpr std::uintptr_t health_key = 0x64; // field: DWORD chave XOR
inline constexpr std::uintptr_t read_size = 0x68; // read_size: Bloco do ator usado na amostra
}
namespace GameRules {
inline constexpr std::uintptr_t get_team_vtable_slot = 0x368; // vtable_slot: Offset na vtable para validar GetTeam
inline constexpr std::uintptr_t team_tree_header = 0x110; // field: Ponteiro para header da arvore de equipes
}
namespace TeamTree {
inline constexpr std::uintptr_t root = 0x8; // field: header -> raiz
inline constexpr std::uintptr_t left = 0x0; // field: Filho esquerdo
inline constexpr std::uintptr_t right = 0x10; // field: Filho direito
inline constexpr std::uintptr_t sentinel_flag = 0x19; // field: BYTE de sentinel/nil
inline constexpr std::uintptr_t entity_id = 0x1C; // field: DWORD ID da entidade
inline constexpr std::uintptr_t team_id = 0x20; // field: DWORD ID da equipe
inline constexpr std::uintptr_t node_read_size = 0x28; // read_size: Bloco lido por node
}
namespace Cry3DEngine {
inline constexpr std::uintptr_t frame_counter = 0x3776D8; // module_rva: DWORD contador de quadros, lido no lote e reconferido apos a leitura
}
namespace RenderTemp {
inline constexpr std::uintptr_t last_visible_frame = 0x60; // field: DWORD do registro recente de visibilidade do personagem
inline constexpr std::uintptr_t last_occluded_frame = 0x64; // field: DWORD do registro recente de oclusao do personagem
}
namespace CharacterPhysics {
inline constexpr std::uintptr_t physical_entity = 0xB38; // field: character -> entidade fisica
}
namespace CryPhysics {
inline constexpr std::uintptr_t validated_vtable_upper_bound = 0x3D3080; // validation_bound: Limite exclusivo RVA usado na validacao da vtable fisica; nao e campo de objeto
inline constexpr std::uintptr_t sphere_vtable = 0x24CCA8; // module_rva: Vtable reconhecida para sphere
inline constexpr std::uintptr_t mesh_vtable = 0x24E770; // module_rva: Vtable reconhecida para triangle mesh
inline constexpr std::uintptr_t capsule_vtable = 0x24DEA0; // module_rva: Vtable reconhecida para capsule
inline constexpr std::uintptr_t box_vtable = 0x24D6C0; // module_rva: Vtable reconhecida para box
}
namespace PhysicalEntity {
inline constexpr std::uintptr_t vtable = 0x0; // field: Ponteiro da vtable
inline constexpr std::uintptr_t position = 0xA8; // field: Float3 de posicao mundial
inline constexpr std::uintptr_t rotation = 0xB4; // field: Quat xyzw da entidade
inline constexpr std::uintptr_t parts = 0x120; // field: Ponteiro para array de partes
inline constexpr std::uintptr_t part_count = 0x128; // field: int quantidade de partes
inline constexpr std::uintptr_t header_read_size = 0x130; // read_size: Bloco de entidade fisica
}
namespace PhysicalPart {
inline constexpr std::uintptr_t stride = 0x88; // stride: Bytes por parte
inline constexpr std::uintptr_t position = 0x0; // field: Float3 posicao local
inline constexpr std::uintptr_t rotation = 0xC; // field: Quat xyzw local
inline constexpr std::uintptr_t scale = 0x1C; // field: Float escala
inline constexpr std::uintptr_t geometry_descriptor = 0x38; // field: Ponteiro para descritor
inline constexpr std::uintptr_t part_id = 0x48; // field: DWORD identificado com o joint correspondente
}
namespace GeometryDescriptor {
inline constexpr std::uintptr_t shape = 0x0; // field: Primeiro QWORD aponta para a forma IGeometry
inline constexpr std::uintptr_t shape_header_read_size = 0x90; // read_size: Bloco inicial de forma usado na descoberta
}
namespace Sphere {
inline constexpr std::uintptr_t center = 0x30; // field: Float3 centro local
inline constexpr std::uintptr_t radius = 0x3C; // field: Float raio
inline constexpr std::uintptr_t type_id = 0x4; // type_id: Identificador de tipo usado na cache local
}
namespace TriangleMesh {
inline constexpr std::uintptr_t indices = 0x40; // field: Ponteiro para indices int32; tres por triangulo
inline constexpr std::uintptr_t vertices = 0x58; // field: Ponteiro para vertices
inline constexpr std::uintptr_t vertex_stride = 0x60; // field: int stride em bytes; float3 no inicio de cada vertice
inline constexpr std::uintptr_t triangle_count = 0x80; // field: int quantidade de triangulos
inline constexpr std::uintptr_t vertex_count = 0x84; // field: int quantidade de vertices
inline constexpr std::uintptr_t type_id = 0x1; // type_id: Identificador de tipo usado na cache local
}
namespace Capsule {
inline constexpr std::uintptr_t center = 0x30; // field: Float3 centro local
inline constexpr std::uintptr_t axis = 0x3C; // field: Float3 eixo unitario
inline constexpr std::uintptr_t radius = 0x48; // field: Float raio
inline constexpr std::uintptr_t half_length = 0x4C; // field: Float meia distancia entre centros das extremidades
inline constexpr std::uintptr_t type_id = 0x6; // type_id: Identificador de tipo usado na cache local
}
namespace Box {
inline constexpr std::uintptr_t basis = 0x30; // field: Nove floats: tres linhas da base
inline constexpr std::uintptr_t oriented = 0x54; // field: int indica base orientada
inline constexpr std::uintptr_t center = 0x58; // field: Float3 centro local
inline constexpr std::uintptr_t half_extents = 0x64; // field: Float3 meios tamanhos
inline constexpr std::uintptr_t type_id = 0x0; // type_id: Identificador de tipo usado na cache local
}
}
