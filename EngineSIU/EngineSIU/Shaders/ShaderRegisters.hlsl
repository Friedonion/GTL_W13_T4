
#ifndef SHADER_REGISTER_INCLUDE
#define SHADER_REGISTER_INCLUDE

float LinearToSRGB(float val)
{
    float low  = 12.92 * val;
    float high = 1.055 * pow(val, 1.0 / 2.4) - 0.055;
    // linear가 임계값보다 큰지 판별 후 선형 보간
    float t = step(0.0031308, val); // linear >= 0.0031308이면 t = 1, 아니면 t = 0
    return lerp(low, high, t);
}

float3 LinearToSRGB(float3 color)
{
    color.r = LinearToSRGB(color.r);
    color.g = LinearToSRGB(color.g);
    color.b = LinearToSRGB(color.b);
    return color;
}

float SRGBToLinear(float val)
{
    float low  = val / 12.92;
    float high = pow((val + 0.055) / 1.055, 2.4);
    float t = step(0.04045, val); // srgb가 0.04045 이상이면 t = 1, 아니면 t = 0
    return lerp(low, high, t);
}

float3 SRGBToLinear(float3 color)
{
    color.r = SRGBToLinear(color.r);
    color.g = SRGBToLinear(color.g);
    color.b = SRGBToLinear(color.b);
    return color;
}

struct FMaterial
{
    uint TextureFlag;
    float3 DiffuseColor;
    
    float3 SpecularColor;
    float Shininess;
    
    float3 EmissiveColor;
    float Transparency;
    
    float Metallic;
    float Roughness;
    float2 MaterialPadding;
};

#define TEXTURE_FLAG_DIFFUSE       (1 << 0)
#define TEXTURE_FLAG_SPECULAR      (1 << 1)
#define TEXTURE_FLAG_NORMAL        (1 << 2)
#define TEXTURE_FLAG_EMISSIVE      (1 << 3)
#define TEXTURE_FLAG_ALPHA         (1 << 4)
#define TEXTURE_FLAG_AMBIENT       (1 << 5)
#define TEXTURE_FLAG_SHININESS     (1 << 6)
#define TEXTURE_FLAG_METALLIC      (1 << 7)
#define TEXTURE_FLAG_ROUGHNESS     (1 << 8)

#define TEXTURE_SLOT_DIFFUSE       (0)
#define TEXTURE_SLOT_SPECULAR      (1)
#define TEXTURE_SLOT_NORMAL        (2)
#define TEXTURE_SLOT_EMISSIVE      (3)
#define TEXTURE_SLOT_ALPHA         (4)
#define TEXTURE_SLOT_AMBIENT       (5)
#define TEXTURE_SLOT_SHININESS     (6)
#define TEXTURE_SLOT_METALLIC      (7)
#define TEXTURE_SLOT_ROUGHNESS     (8)

Texture2D MaterialTextures[9] : register(t0);

SamplerState SamplerLinearWrap : register(s0);
SamplerState SamplerLinearClamp : register(s1);
SamplerState SamplerPointWrap : register(s2);
SamplerState SamplerPointClamp : register(s3);

struct VS_INPUT_StaticMesh
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV : TEXCOORD;
};

struct VS_INPUT_SkeletalMesh
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV : TEXCOORD;
    uint4 BoneIndices : BONE_INDICES;
    float4 BoneWeights : BONE_WEIGHTS;
};

struct PS_INPUT_CommonMesh
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD0;
    float3 WorldNormal : TEXCOORD1;
    float4 WorldTangent : TEXCOORD2;
    float3 WorldPosition : TEXCOORD3;
};

////////
/// 공용: 11 ~ 13
///////
cbuffer ObjectBuffer : register(b12)
{
    row_major matrix WorldMatrix;
    row_major matrix InverseTransposedWorld;
    
    float4 UUID;
    
    bool bIsSelected;
    float3 ObjectPadding;
};

/**
 * 기존에는 View 버퍼와 Projection 버퍼의
 * 업데이트 시기가 달라서 분리했지만, 
 * 여러 뷰포트를 렌더하는 경우 다음 뷰포트로 넘어가면
 * Projection 버퍼를 업데이트해야 하므로,
 * 버퍼를 분리하는 의미가 사라졌으므로 통합.
 */
cbuffer CameraBuffer : register(b13)
{
    row_major matrix ViewMatrix;
    row_major matrix InvViewMatrix;
    
    row_major matrix ProjectionMatrix;
    row_major matrix InvProjectionMatrix;
    
    float3 ViewWorldLocation; // TODO: 가능하면 버퍼에서 빼기
    float ViewPadding;
    
    float NearClip;
    float FarClip;
    float2 ProjectionPadding;
}

   
static const float2 diskSamples64[64] =
{
    float2(0.0, 0.0),
            float2(-0.12499844227275288, 0.000624042775189866), float2(0.1297518688031755, -0.12006020382326336),
            float2(-0.017851253586055427, 0.21576916541852392), float2(-0.1530983013115895, -0.19763833164521946),
            float2(0.27547541035593626, 0.0473106572479027), float2(-0.257522587854559, 0.16562643733622642),
            float2(0.0842605283808073, -0.3198048832600703), float2(0.1645196099088727, 0.3129429627830483),
            float2(-0.3528833088400373, -0.12687935349026194), float2(0.36462214742013344, -0.1526456341030772),
            float2(-0.17384046457324884, 0.37637015407303087), float2(-0.1316547617859344, -0.4125130588224921),
            float2(0.3910687393754993, 0.2240317858770442), float2(-0.45629121277761536, 0.10270505898899496),
            float2(0.27645268679640483, -0.3974278701387824), float2(0.06673001731984558, 0.49552709793561556),
            float2(-0.39574431915605623, -0.33016879600548193), float2(0.5297612167716342, -0.024557141621887494),
            float2(-0.3842909284448636, 0.3862583103507092), float2(0.0230336562454131, -0.5585422550532486),
            float2(0.36920334463249477, 0.43796562686149154), float2(-0.5814490172413539, -0.07527974727019048),
            float2(0.4903718680780365, -0.3448339179919178), float2(-0.13142003698572613, 0.5981043168868373),
            float2(-0.31344141845114937, -0.540721256470773), float2(0.608184438565748, 0.19068741092811003),
            float2(-0.5882602609696388, 0.27536315179038107), float2(0.25230610046544444, -0.6114259003901626),
            float2(0.23098706800827415, 0.6322736546883326), float2(-0.6076303951666067, -0.31549215975943595),
            float2(0.6720886334230931, -0.1807536135834609), float2(-0.37945598830371974, 0.5966683776943834),
            float2(-0.1251555455510758, -0.7070792667147104), float2(0.5784815570900413, 0.44340623372555477),
            float2(-0.7366710399837763, 0.0647362251696953), float2(0.50655463562529, -0.553084443034271),
            float2(8.672987356252326e-05, 0.760345311340794), float2(-0.5205650355786364, -0.5681215043747359),
            float2(0.7776435491294021, 0.06815798190547596), float2(-0.6273416101921778, 0.48108471615868836),
            float2(0.1393236805531513, -0.7881712453757264), float2(0.4348773806743975, 0.6834703093608201),
            float2(-0.7916014213464706, -0.21270211499241704), float2(0.7357897682897174, -0.38224784745000717),
            float2(-0.2875567908732709, 0.7876776574352392), float2(-0.3235695699691864, -0.7836151691933712),
            float2(0.7762165924462436, 0.3631291803355136), float2(-0.8263007976064866, 0.2592816844184794),
            float2(0.4386452756167397, -0.7571098481588484), float2(0.18988542402304126, 0.8632459242554175),
            float2(-0.7303253445407815, -0.5133224046555819), float2(0.8939004035324556, -0.11593993515830946),
            float2(-0.5863762307291154, 0.6959079795748251), float2(-0.03805753378232556, -0.9177699189461416),
            float2(0.653979655650389, 0.657027860897389), float2(-0.9344208130797295, -0.04310155546401203),
            float2(0.7245109901504777, -0.6047386420191574), float2(-0.12683493131695708, 0.9434844461875473),
            float2(-0.5484582700240663, -0.7880790100251422), float2(0.9446610338564589, 0.2124041692463835),
            float2(-0.8470120123194587, 0.48548496473788055), float2(0.29904134279525085, -0.9377229203230629),
            float2(0.41623562331748715, 0.9006236205438447),
};

#endif
