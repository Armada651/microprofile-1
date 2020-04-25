// This is free and unencumbered software released into the public domain.
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
// For more information, please refer to <http://unlicense.org/>

#ifndef MICROPROFILE_ENABLED
#error "microprofile.h must be included before including microprofiledraw.h"
#endif

#ifndef MICROPROFILEDRAW_ENABLED
#define MICROPROFILEDRAW_ENABLED MICROPROFILE_ENABLED
#endif

#ifndef MICROPROFILEDRAW_API
#define MICROPROFILEDRAW_API
#endif

#ifndef MICROPROFILEDRAW_GL
#define MICROPROFILEDRAW_GL 1
#endif

#if MICROPROFILEDRAW_ENABLED
#if MICROPROFILEDRAW_GL
MICROPROFILEDRAW_API void MicroProfileDrawInitGL();
#endif
#if MICROPROFILEDRAW_D3D11
MICROPROFILEDRAW_API void MicroProfileDrawInitD3D11(struct ID3D11DeviceContext* pContext);
#endif

MICROPROFILEDRAW_API void MicroProfileRender(uint32_t nWidth, uint32_t nHeight, float fScale);

MICROPROFILEDRAW_API void MicroProfileBeginDraw(uint32_t nWidth, uint32_t nHeight, float* pfProjection);
MICROPROFILEDRAW_API void MicroProfileBeginDraw(uint32_t nWidth, uint32_t nHeight, float fScale);
MICROPROFILEDRAW_API void MicroProfileEndDraw();

#ifdef MICROPROFILEDRAW_IMPL
struct MicroProfileDrawVertex
{
	float nX;
	float nY;
	uint32_t nColor;
	float fU;
	float fV;
};

struct MicroProfileDrawCommand
{
	uint32_t nCommand;
	uint32_t nNumVertices;
};

enum MicroProfileRenderer
{
	MicroProfileOpenGL,
	MicroProfileD3D11,
};

enum MicroProfileShader
{
	MicroProfileVertexShader,
	MicroProfileFragmentShader,
};

enum MicroProfileCommand
{
	MicroProfileLines,
	MicroProfileTriangles,
};

struct MicroProfileDrawContext
{
	enum
	{
		MAX_COMMANDS = 32,
		MAX_VERTICES = 16384,
	};

	bool bInitialized;
	MicroProfileRenderer eRenderer;

#if MICROPROFILEDRAW_GL
	GLuint nVAO;
	GLuint nVertexBuffer;
	GLuint nProgram;
	GLuint nTexture;

	int nAttributePosition;
	int nAttributeColor;
	int nAttributeTexture;
	int nUniformProjectionMatrix;
#endif

#if MICROPROFILEDRAW_D3D11
	ID3D11DeviceContext* pContext;

	struct
	{
		ID3D11Buffer* pVertexBuffer;
		ID3D11VertexShader* pVertexShader;
		ID3D11PixelShader* pPixelShader;
		ID3D11ShaderResourceView* pView;
		ID3D11InputLayout* pLayout;
		ID3D11BlendState* pBlend;
		ID3D11DepthStencilState* pDepthStencil;
		ID3D11RasterizerState* pRasterizer;
		ID3D11Buffer* pConstantProjectionMatrix;

		float BlendFactor[4];
		UINT SampleMask, StencilRef, Stride, Offset;
	} State[2];
#endif

	uint32_t nVertexPos;
	uint32_t nCommandPos;

	MicroProfileDrawVertex nVertices[MAX_VERTICES];
	MicroProfileDrawCommand nCommands[MAX_COMMANDS];
};

MicroProfileDrawContext g_MicroProfileDraw;

#define Q0(d, member, v) d[0].member = v
#define Q1(d, member, v) d[1].member = v; d[3].member = v
#define Q2(d, member, v) d[4].member = v
#define Q3(d, member, v) d[2].member = v; d[5].member = v

#define FONT_TEX_X 1024
#define FONT_TEX_Y 9
#define FONT_SIZE (FONT_TEX_X*FONT_TEX_Y * 4)

#define TOSTRING(s) #s
#define STRINGIZE(s) TOSTRING(s)

namespace
{
	extern const uint8_t g_MicroProfileFont[];
	extern const uint16_t g_MicroProfileFontDescription[]; 

	extern const char g_MicroProfileVertexShader_110[];
	extern const char g_MicroProfileFragmentShader_110[];

	extern const char g_MicroProfileVertexShader_150[];
	extern const char g_MicroProfileFragmentShader_150[];

	extern const char g_MicroProfileVertexShader_hlsl[];
	extern const char g_MicroProfileFragmentShader_hlsl[];
}

bool MicroProfileCompileShader(void* pnHandle, int nType, const char* pShader)
{
#if MICROPROFILEDRAW_GL
	if (g_MicroProfileDraw.eRenderer == MicroProfileOpenGL)
	{
		GLuint nHandle = glCreateShader(nType == MicroProfileVertexShader ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
		glShaderSource(nHandle, 1, &pShader, 0);
		glCompileShader(nHandle);

		GLint compiled = 0;
		glGetShaderiv(nHandle, GL_COMPILE_STATUS, &compiled);

		if (!compiled)
		{
			char temp[4096];
			glGetShaderInfoLog(nHandle, 4096, NULL, temp);
			printf("SHADER FAILED TO COMPILE:\n%s\n", temp);
			return false;
		}

		*(GLuint*)pnHandle = nHandle;
		return true;
	}
#endif

#if MICROPROFILEDRAW_D3D11
	if (g_MicroProfileDraw.eRenderer == MicroProfileD3D11)
	{
		ID3DBlob* pError;
		HRESULT hr = D3DCompile(pShader, strlen(pShader) + 1,
			NULL, NULL, NULL, "main",
			nType == MicroProfileVertexShader ? "vs_5_0" : "ps_5_0",
			0, 0, (ID3DBlob**)pnHandle, &pError);

		if (pError)
		{
			printf("SHADER FAILED TO COMPILE:\n%s\n", (char*)pError->GetBufferPointer());
			pError->Release();
			return false;
		}

		return SUCCEEDED(hr);
	}
#endif

	return false;
}

#if MICROPROFILEDRAW_GL
bool MicroProfileLinkProgram(GLuint* pnHandle, GLuint nVertexShader, GLuint nFragmentShader)
{
	*pnHandle = glCreateProgram();
	glAttachShader(*pnHandle, nVertexShader);
	glAttachShader(*pnHandle, nFragmentShader);
	glLinkProgram(*pnHandle);

	GLint linked = 0;
	glGetProgramiv(*pnHandle, GL_LINK_STATUS, &linked);

	if(!linked)
	{
		char temp[4096];
		glGetProgramInfoLog(*pnHandle, 4096, NULL, temp);
		printf("PROGRAM FAILED TO LINK:\n%s\n", temp);
		return false;
	}

	return true;
}

void MicroProfileDrawInitGL()
{
	MicroProfileDrawContext& S = g_MicroProfileDraw;

	MP_ASSERT(!S.bInitialized);
	S.eRenderer = MicroProfileOpenGL;

	const GLubyte* pGLVersion = glGetString(GL_VERSION);
	const GLubyte* pGLSLVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

	int nGLVersion = (pGLVersion[0] - '0') * 10 + (pGLVersion[2] - '0');
	int nGLSLVersion = (pGLSLVersion[0] - '0') * 100 + (pGLSLVersion[2] - '0') * 10 + (pGLSLVersion[3] - '0');

	glGenBuffers(1, &S.nVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, S.nVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(S.nVertices), 0, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (nGLVersion >= 3)
		glGenVertexArrays(1, &S.nVAO);
	else
		S.nVAO = 0;

	GLuint nVertexShader;
	if(!MicroProfileCompileShader(&nVertexShader, MicroProfileVertexShader, nGLSLVersion >= 150 ? g_MicroProfileVertexShader_150 : g_MicroProfileVertexShader_110))
		return;

	GLuint nFragmentShader;
	if(!MicroProfileCompileShader(&nFragmentShader, MicroProfileFragmentShader, nGLSLVersion >= 150 ? g_MicroProfileFragmentShader_150 : g_MicroProfileFragmentShader_110))
		return;

	if(!MicroProfileLinkProgram(&S.nProgram, nVertexShader, nFragmentShader))
		return;

	S.nAttributePosition = glGetAttribLocation(S.nProgram, "VertexIn");
	S.nAttributeColor = glGetAttribLocation(S.nProgram, "ColorIn");
	S.nAttributeTexture = glGetAttribLocation(S.nProgram, "TCIn");

	S.nUniformProjectionMatrix = glGetUniformLocation(S.nProgram, "ProjectionMatrix");
	
	glUseProgram(S.nProgram);
	glUniform1i(glGetUniformLocation(S.nProgram, "Texture"), 0);
	glUniform1f(glGetUniformLocation(S.nProgram, "RcpFontHeight"), 1.f / FONT_TEX_Y);
	glUseProgram(0);

	uint32_t* pUnpacked = (uint32_t*)alloca(FONT_SIZE);
	int idx = 0;
	int end = FONT_TEX_X * FONT_TEX_Y / 8;
	for(int i = 0; i < end; i++)
	{
		unsigned char pValue = g_MicroProfileFont[i];
		for(int j = 0; j < 8; ++j)
		{
			pUnpacked[idx++] = pValue & 0x80 ? (uint32_t)-1 : 0;
			pValue <<= 1;
		}
	}

	pUnpacked[idx-1] = 0xffffffff;

	uint32_t* p4 = &pUnpacked[0];
	glGenTextures(1, &S.nTexture);
	glBindTexture(GL_TEXTURE_2D, S.nTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);     
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FONT_TEX_X, FONT_TEX_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &p4[0]);
	glBindTexture(GL_TEXTURE_2D, 0);

	S.bInitialized = true;
}
#endif

#if MICROPROFILEDRAW_D3D11
void MicroProfileDrawInitD3D11(ID3D11DeviceContext* pContext)
{
	MicroProfileDrawContext& S = g_MicroProfileDraw;

	MP_ASSERT(!S.bInitialized);
	S.eRenderer = MicroProfileD3D11;
	S.pContext = pContext;

	ID3D11Device* pDevice;
	pContext->GetDevice(&pDevice);

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(S.nVertices);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	pDevice->CreateBuffer(&bufferDesc, NULL, &S.State[0].pVertexBuffer);

	ID3DBlob* pVertexCode;
	if (!MicroProfileCompileShader(&pVertexCode, MicroProfileVertexShader, g_MicroProfileVertexShader_hlsl))
		return;

	ID3DBlob* pPixelCode;
	if (!MicroProfileCompileShader(&pPixelCode, MicroProfileFragmentShader, g_MicroProfileFragmentShader_hlsl))
		return;

	pDevice->CreateVertexShader(pVertexCode->GetBufferPointer(), pVertexCode->GetBufferSize(), NULL, &S.State[0].pVertexShader);
	pDevice->CreatePixelShader(pPixelCode->GetBufferPointer(), pPixelCode->GetBufferSize(), NULL, &S.State[0].pPixelShader);

	D3D11_INPUT_ELEMENT_DESC elements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	pDevice->CreateInputLayout(elements, sizeof(elements) / sizeof(D3D11_INPUT_ELEMENT_DESC),
		pVertexCode->GetBufferPointer(), pVertexCode->GetBufferSize(), &S.State[0].pLayout);

	pVertexCode->Release();
	pPixelCode->Release();

	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(float) * 16;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	pDevice->CreateBuffer(&bufferDesc, NULL, &S.State[0].pConstantProjectionMatrix);

	uint32_t* pUnpacked = (uint32_t*)alloca(FONT_SIZE);
	int idx = 0;
	int end = FONT_TEX_X * FONT_TEX_Y / 8;
	for (int i = 0; i < end; i++)
	{
		unsigned char pValue = g_MicroProfileFont[i];
		for (int j = 0; j < 8; ++j)
		{
			pUnpacked[idx++] = pValue & 0x80 ? (uint32_t)-1 : 0;
			pValue <<= 1;
		}
	}

	pUnpacked[idx - 1] = 0xffffffff;

	D3D11_SUBRESOURCE_DATA initialData;
	initialData.pSysMem = &pUnpacked[0];
	initialData.SysMemPitch = FONT_TEX_X * 4;
	initialData.SysMemSlicePitch = 0;

	ID3D11Texture2D* pTexture;
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = FONT_TEX_X;
	texDesc.Height = FONT_TEX_Y;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	pDevice->CreateTexture2D(&texDesc, &initialData, &pTexture);

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Texture2D.MipLevels = 1;
	viewDesc.Texture2D.MostDetailedMip = 0;
	pDevice->CreateShaderResourceView(pTexture, &viewDesc, &S.State[0].pView);
	pTexture->Release();

	D3D11_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	pDevice->CreateBlendState(&blendDesc, &S.State[0].pBlend);

	D3D11_DEPTH_STENCIL_DESC depthDesc;
	depthDesc.DepthEnable = false;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	depthDesc.StencilEnable = false;
	depthDesc.StencilReadMask = 0;
	depthDesc.StencilWriteMask = 0;
	depthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;
	depthDesc.BackFace = depthDesc.FrontFace;
	pDevice->CreateDepthStencilState(&depthDesc, &S.State[0].pDepthStencil);

	D3D11_RASTERIZER_DESC rastDesc;
	rastDesc.FillMode = D3D11_FILL_SOLID;
	rastDesc.CullMode = D3D11_CULL_NONE;
	rastDesc.FrontCounterClockwise = true;
	rastDesc.DepthBias = 0;
	rastDesc.DepthBiasClamp = 0.f;
	rastDesc.SlopeScaledDepthBias = 0.f;
	rastDesc.DepthClipEnable = true;
	rastDesc.ScissorEnable = false;
	rastDesc.MultisampleEnable = false;
	rastDesc.AntialiasedLineEnable = false;
	pDevice->CreateRasterizerState(&rastDesc, &S.State[0].pRasterizer);

	S.bInitialized = true;
}
#endif

void MicroProfileBeginDraw(uint32_t nWidth, uint32_t nHeight, float* pfProjection)
{
	(void)nWidth;
	(void)nHeight;

	MicroProfileDrawContext& S = g_MicroProfileDraw;

	if (!S.bInitialized)
		return;

#if MICROPROFILEDRAW_GL
	if (S.eRenderer == MicroProfileOpenGL)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		if (S.nVAO)
			glBindVertexArray(S.nVAO);

		glUseProgram(S.nProgram);
		glUniformMatrix4fv(S.nUniformProjectionMatrix, 1, 0, pfProjection);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, S.nTexture);

		glBindBuffer(GL_ARRAY_BUFFER, S.nVertexBuffer);

		int nStride = sizeof(MicroProfileDrawVertex);

		glVertexAttribPointer(S.nAttributePosition, 2, GL_FLOAT, 0, nStride, (void*)(offsetof(MicroProfileDrawVertex, nX)));
		glVertexAttribPointer(S.nAttributeColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, nStride, (void*)(offsetof(MicroProfileDrawVertex, nColor)));
		glVertexAttribPointer(S.nAttributeTexture, 2, GL_FLOAT, 0, nStride, (void*)(offsetof(MicroProfileDrawVertex, fU)));

		glEnableVertexAttribArray(S.nAttributePosition);
		glEnableVertexAttribArray(S.nAttributeColor);
		glEnableVertexAttribArray(S.nAttributeTexture);
	}
#endif

#if MICROPROFILEDRAW_D3D11
	if (S.eRenderer == MicroProfileD3D11)
	{
		S.pContext->OMGetBlendState(&S.State[1].pBlend, S.State[1].BlendFactor, &S.State[1].SampleMask);
		S.pContext->OMGetDepthStencilState(&S.State[1].pDepthStencil, &S.State[1].StencilRef);
		S.pContext->RSGetState(&S.State[1].pRasterizer);

		S.pContext->IAGetVertexBuffers(0, 1, &S.State[1].pVertexBuffer, &S.State[1].Stride, &S.State[1].Offset);
		S.pContext->VSGetShader(&S.State[1].pVertexShader, NULL, NULL);
		S.pContext->VSGetConstantBuffers(0, 1, &S.State[1].pConstantProjectionMatrix);
		S.pContext->PSGetShader(&S.State[1].pPixelShader, NULL, 0);
		S.pContext->PSGetShaderResources(0, 1, &S.State[1].pView);
		S.pContext->IAGetInputLayout(&S.State[1].pLayout);

		S.pContext->OMSetBlendState(S.State[0].pBlend, S.State[0].BlendFactor, S.State[0].SampleMask);
		S.pContext->OMSetDepthStencilState(S.State[0].pDepthStencil, 0);
		S.pContext->RSSetState(S.State[0].pRasterizer);

		D3D11_MAPPED_SUBRESOURCE map;
		S.pContext->Map(S.State[0].pConstantProjectionMatrix, 0, D3D11_MAP_WRITE, 0, &map);
		memcpy(map.pData, pfProjection, sizeof(float) * 16);
		S.pContext->Unmap(S.State[0].pConstantProjectionMatrix, 0);

		S.pContext->IASetVertexBuffers(0, 1, &S.State[0].pVertexBuffer, &S.State[0].Stride, &S.State[0].Offset);
		S.pContext->VSSetShader(S.State[0].pVertexShader, NULL, 0);
		S.pContext->VSSetConstantBuffers(0, 1, &S.State[0].pConstantProjectionMatrix);
		S.pContext->PSSetShader(S.State[0].pPixelShader, NULL, 0);
		S.pContext->PSSetShaderResources(0, 1, &S.State[0].pView);
		S.pContext->IASetInputLayout(S.State[0].pLayout);
	}
#endif

	S.nVertexPos = 0;
	S.nCommandPos = 0;
}

void MicroProfileBeginDraw(uint32_t nWidth, uint32_t nHeight, float fScale)
{
	MicroProfileDrawContext& S = g_MicroProfileDraw;

	if (!S.bInitialized)
		return;

	float left = 0.f;
	float right = nWidth / fScale;
	float bottom = nHeight / fScale;
	float top = 0.f;
	float near_plane = -1.f;
	float far_plane = 1.f;

	float projection[16] = {};

	projection[0] = 2.0f / (right - left);
	projection[5] = 2.0f / (top - bottom);
	projection[10] = -2.0f / (far_plane - near_plane);
	projection[12] = - (right + left) / (right - left);
	projection[13] = - (top + bottom) / (top - bottom);
	projection[14] = - (far_plane + near_plane) / (far_plane - near_plane);
	projection[15] = 1.f;

	MicroProfileBeginDraw(nWidth, nHeight, projection);
}

void MicroProfileFlush()
{
	MicroProfileDrawContext& S = g_MicroProfileDraw;

	if(S.nVertexPos == 0)
		return;

	MICROPROFILE_SCOPEI("MicroProfile", "Flush", 0xffff3456);

#if MICROPROFILEDRAW_GL
	if (S.eRenderer == MicroProfileOpenGL)
	{
		glBufferSubData(GL_ARRAY_BUFFER, 0, S.nVertexPos * sizeof(MicroProfileDrawVertex), S.nVertices);
	}
#endif

#if MICROPROFILEDRAW_D3D11
	if (S.eRenderer == MicroProfileD3D11)
	{
		D3D11_MAPPED_SUBRESOURCE map;
		S.pContext->Map(S.State[0].pVertexBuffer, 0, D3D11_MAP_WRITE, 0, &map);
		memcpy(map.pData, S.nVertices, S.nVertexPos * sizeof(MicroProfileDrawVertex));
		S.pContext->Unmap(S.State[0].pVertexBuffer, 0);
	}
#endif

	int nOffset = 0;

	for(int i = 0; i < int(S.nCommandPos); ++i)
	{
		int nCount = S.nCommands[i].nNumVertices;

#if MICROPROFILEDRAW_GL
		if (S.eRenderer == MicroProfileOpenGL)
		{
			glDrawArrays(S.nCommands[i].nCommand == MicroProfileLines ? GL_LINES : GL_TRIANGLES, nOffset, nCount);
		}
#endif

#if MICROPROFILEDRAW_D3D11
		if (S.eRenderer == MicroProfileD3D11)
		{
			S.pContext->IASetPrimitiveTopology(S.nCommands[i].nCommand == MicroProfileLines ?
				D3D11_PRIMITIVE_TOPOLOGY_LINELIST : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			S.pContext->Draw(nCount, nOffset);
		}
#endif

		nOffset += nCount;
	}

	S.nVertexPos = 0;
	S.nCommandPos = 0;
}

MicroProfileDrawVertex* MicroProfilePushVertices(uint32_t nCommand, int nCount)
{
	MP_ASSERT(nCount <= MicroProfileDrawContext::MAX_VERTICES);

	MicroProfileDrawContext& S = g_MicroProfileDraw;

	if(S.nVertexPos + nCount > MicroProfileDrawContext::MAX_VERTICES)
		MicroProfileFlush();

	if(S.nCommandPos && S.nCommands[S.nCommandPos-1].nCommand == nCommand)
	{
		S.nCommands[S.nCommandPos-1].nNumVertices += nCount;
	}
	else
	{
		if (S.nCommandPos == MicroProfileDrawContext::MAX_COMMANDS)
			MicroProfileFlush();

		S.nCommands[S.nCommandPos].nCommand = nCommand;
		S.nCommands[S.nCommandPos].nNumVertices = nCount;
		S.nCommandPos++;
	}

	uint32_t nOut = S.nVertexPos;
	S.nVertexPos += nCount;

	return &S.nVertices[nOut];
}

void MicroProfileEndDraw()
{
	MicroProfileDrawContext& S = g_MicroProfileDraw;

	if (!S.bInitialized)
		return;

	MicroProfileFlush();

#if MICROPROFILEDRAW_GL
	if (S.eRenderer == MicroProfileOpenGL)
	{
		glDisableVertexAttribArray(S.nAttributePosition);
		glDisableVertexAttribArray(S.nAttributeColor);
		glDisableVertexAttribArray(S.nAttributeTexture);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glUseProgram(0);

		if (S.nVAO)
			glBindVertexArray(0);

		glDisable(GL_BLEND);
	}
#endif

#if MICROPROFILEDRAW_D3D11
	if (S.eRenderer == MicroProfileD3D11)
	{
		S.pContext->OMSetBlendState(S.State[1].pBlend, S.State[1].BlendFactor, S.State[1].SampleMask);
		S.pContext->OMSetDepthStencilState(S.State[1].pDepthStencil, 0);
		S.pContext->RSSetState(S.State[1].pRasterizer);

		S.pContext->IASetVertexBuffers(0, 1, &S.State[1].pVertexBuffer, &S.State[1].Stride, &S.State[1].Offset);
		S.pContext->VSSetShader(S.State[1].pVertexShader, NULL, 0);
		S.pContext->VSSetConstantBuffers(0, 1, &S.State[1].pConstantProjectionMatrix);
		S.pContext->PSSetShader(S.State[1].pPixelShader, NULL, 0);
		S.pContext->PSSetShaderResources(0, 1, &S.State[1].pView);
		S.pContext->IASetInputLayout(S.State[1].pLayout);
	}
#endif
}

void MicroProfileRender(uint32_t nWidth, uint32_t nHeight, float fScale)
{
	MicroProfileBeginDraw(nWidth, nHeight, fScale);
	MicroProfileDraw(nWidth, nHeight);
	MicroProfileEndDraw();
}

void MicroProfileDrawText(int nX, int nY, uint32_t nColor, const char* pText, uint32_t nLen)
{
	MICROPROFILE_SCOPEI("MicroProfile", "TextDraw", 0xff88ee);

	const float fOffsetU = 5.f / 1024.f;
	MP_ASSERT(nLen <= strlen(pText));
	float fX = (float)nX;
	float fY = (float)nY;
	float fY2 = fY + (MICROPROFILE_TEXT_HEIGHT+1);

	MicroProfileDrawVertex* pVertex = MicroProfilePushVertices(MicroProfileTriangles, 6 * nLen);
	if (!pVertex) return;

	const char* pStr = pText;
	nColor = 0xff000000|((nColor&0xff)<<16)|(nColor&0xff00)|((nColor>>16)&0xff);

	for(uint32_t j = 0; j < nLen; ++j)
	{
		int16_t nOffset = g_MicroProfileFontDescription[uint8_t(*pStr++)];
		float fOffset = nOffset / 1024.f;
		Q0(pVertex,nX, fX);
		Q0(pVertex,nY, fY);
		Q0(pVertex,nColor, nColor);
		Q0(pVertex,fU, fOffset);
		Q0(pVertex,fV, 0.f);
		
		Q1(pVertex, nX, fX+MICROPROFILE_TEXT_WIDTH);
		Q1(pVertex, nY, fY);
		Q1(pVertex, nColor, nColor);
		Q1(pVertex, fU, fOffset+fOffsetU);
		Q1(pVertex, fV, 0.f);

		Q2(pVertex, nX, fX+MICROPROFILE_TEXT_WIDTH);
		Q2(pVertex, nY, fY2);
		Q2(pVertex, nColor, nColor);
		Q2(pVertex, fU, fOffset+fOffsetU);
		Q2(pVertex, fV, 1.f);


		Q3(pVertex, nX, fX);
		Q3(pVertex, nY, fY2);
		Q3(pVertex, nColor, nColor);
		Q3(pVertex, fU, fOffset);
		Q3(pVertex, fV, 1.f);

		fX += MICROPROFILE_TEXT_WIDTH+1;
		pVertex += 6;
	}
}
void MicroProfileDrawBox(int nX0, int nY0, int nX1, int nY1, uint32_t nColor, MicroProfileBoxType Type)
{
	MicroProfileDrawVertex* pVertex = MicroProfilePushVertices(MicroProfileTriangles, 6);
	if (!pVertex) return;

	if(Type == MicroProfileBoxTypeFlat)
	{
		nColor = ((nColor&0xff)<<16)|((nColor>>16)&0xff)|(0xff00ff00&nColor);
		Q0(pVertex, nX, (float)nX0);
		Q0(pVertex, nY, (float)nY0);
		Q0(pVertex, nColor, nColor);
		Q0(pVertex, fU, 2.f);
		Q0(pVertex, fV, 2.f);
		Q1(pVertex, nX, (float)nX1);
		Q1(pVertex, nY, (float)nY0);
		Q1(pVertex, nColor, nColor);
		Q1(pVertex, fU, 2.f);
		Q1(pVertex, fV, 2.f);
		Q2(pVertex, nX, (float)nX1);
		Q2(pVertex, nY, (float)nY1);
		Q2(pVertex, nColor, nColor);
		Q2(pVertex, fU, 2.f);
		Q2(pVertex, fV, 2.f);
		Q3(pVertex, nX, (float)nX0);
		Q3(pVertex, nY, (float)nY1);
		Q3(pVertex, nColor, nColor);
		Q3(pVertex, fU, 2.f);
		Q3(pVertex, fV, 2.f);
	}
	else
	{
		uint32_t r = 0xff & (nColor>>16);
		uint32_t g = 0xff & (nColor>>8);
		uint32_t b = 0xff & nColor;
		uint32_t nMax = MicroProfileMax(MicroProfileMax(MicroProfileMax(r, g), b), 30u);
		uint32_t nMin = MicroProfileMin(MicroProfileMin(MicroProfileMin(r, g), b), 180u);

		uint32_t r0 = 0xff & ((r + nMax)/2);
		uint32_t g0 = 0xff & ((g + nMax)/2);
		uint32_t b0 = 0xff & ((b + nMax)/2);

		uint32_t r1 = 0xff & ((r+nMin)/2);
		uint32_t g1 = 0xff & ((g+nMin)/2);
		uint32_t b1 = 0xff & ((b+nMin)/2);
		uint32_t nColor0 = (r0<<0)|(g0<<8)|(b0<<16)|(0xff000000&nColor);
		uint32_t nColor1 = (r1<<0)|(g1<<8)|(b1<<16)|(0xff000000&nColor);
		Q0(pVertex, nX, (float)nX0);
		Q0(pVertex, nY, (float)nY0);
		Q0(pVertex, nColor, nColor0);
		Q0(pVertex, fU, 2.f);
		Q0(pVertex, fV, 2.f);
		Q1(pVertex, nX, (float)nX1);
		Q1(pVertex, nY, (float)nY0);
		Q1(pVertex, nColor, nColor0);
		Q1(pVertex, fU, 3.f);
		Q1(pVertex, fV, 2.f);
		Q2(pVertex, nX, (float)nX1);
		Q2(pVertex, nY, (float)nY1);
		Q2(pVertex, nColor, nColor1);
		Q2(pVertex, fU, 3.f);
		Q2(pVertex, fV, 3.f);
		Q3(pVertex, nX, (float)nX0);
		Q3(pVertex, nY, (float)nY1);
		Q3(pVertex, nColor, nColor1);
		Q3(pVertex, fU, 2.f);
		Q3(pVertex, fV, 3.f);
	}
}


void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor)
{
	if(!nVertices) return;

	MicroProfileDrawVertex* pVertex = MicroProfilePushVertices(MicroProfileLines, 2*(nVertices-1));
	nColor = ((nColor&0xff)<<16)|(nColor&0xff00ff00)|((nColor>>16)&0xff);
	for(uint32_t i = 0; i < nVertices-1; ++i)
	{
		pVertex[0].nX = pVertices[i*2];
		pVertex[0].nY = pVertices[i*2+1] ;
		pVertex[0].nColor = nColor;
		pVertex[0].fU = 2.f;
		pVertex[0].fV = 2.f;
		pVertex[1].nX = pVertices[(i+1)*2];
		pVertex[1].nY = pVertices[(i+1)*2+1] ;
		pVertex[1].nColor = nColor;
		pVertex[1].fU = 2.f;
		pVertex[1].fV = 2.f;
		pVertex += 2;
	}
}

namespace
{
	const char g_MicroProfileVertexShader_110[] =
		"#version 110\n\
		uniform mat4 ProjectionMatrix; \
		attribute vec3 VertexIn; attribute vec4 ColorIn; attribute vec2 TCIn; \
		varying vec2 TC; varying vec4 Color; \
		void main() { Color = ColorIn; TC = TCIn; gl_Position = ProjectionMatrix * vec4(VertexIn, 1.0); }";

	const char g_MicroProfileVertexShader_150[] =
		"#version 150\n\
		uniform mat4 ProjectionMatrix; \
		in vec3 VertexIn; in vec4 ColorIn; in vec2 TCIn; \
		out vec2 TC; out vec4 Color; \
		void main() { Color = ColorIn; TC = TCIn; gl_Position = ProjectionMatrix * vec4(VertexIn, 1.0); }";

	const char g_MicroProfileFragmentShader_110[] =
		"#version 110\n\
		uniform sampler2D Texture; uniform float RcpFontHeight; \
		varying vec2 TC; varying vec4 Color; \
		void main() { \
			vec4 c0 = texture2D(Texture, TC.xy); \
			vec4 c1 = texture2D(Texture, TC.xy + vec2(0.0, RcpFontHeight)); \
			gl_FragColor = c0.w < 0.5 ? vec4(0, 0, 0, c1.w) : c0 * Color; \
		} \
	";

	const char g_MicroProfileFragmentShader_150[] =
		"#version 150\n\
		uniform sampler2D Texture; uniform float RcpFontHeight; \
		in vec2 TC; in vec4 Color; \
		out vec4 result; \
		void main() { \
			vec4 c0 = texture(Texture, TC.xy); \
			vec4 c1 = texture(Texture, TC.xy + vec2(0.0, RcpFontHeight)); \
			result = c0.w < 0.5 ? vec4(0, 0, 0, c1.w) : c0 * Color; \
		} \
	";

	const char g_MicroProfileVertexShader_hlsl[] =
		"float4x4 ProjectionMatrix; \
		struct VS_INPUT { float3 Vertex : POSITION; float4 Color : COLOR; float2 TC : TEXCOORD; }; \
		struct VS_OUTPUT { float4 Vertex : POSITION; float4 Color : COLOR; float2 TC : TEXCOORD; }; \
		VS_OUTPUT main(const VS_INPUT In) { VS_OUTPUT Out; \
			Out.Color = In.Color; Out.TC = In.TC; Out.Position = mul(float4(In.Vertex, 1.0), ProjectionMatrix); return Out; }";

	const char g_MicroProfileFragmentShader_hlsl[] =
		"SamplerState Sampler { Filter = MIN_MAG_MIP_POINT; AddressU = Clamp; AddressV = Clamp; }; \
		Texture2D Texture; \
		struct PS_INPUT { float2 TC : TEXCOORD; float4 Color : COLOR; }; \
		float4 main(PS_INPUT In) { \
			float4 c0 = Texture.Sample(Sampler, In.TC); \
			float4 c1 = Texture.Sample(Sampler, In.TC + float2(0.0, 1.f / " STRINGIZE(FONT_TEX_Y) "));"
			"return c0.w < 0.5 ? float4(0, 0, 0, c1.w) : c0 * In.Color; \
		} \
	";

	const uint16_t g_MicroProfileFontDescription[] =
	{
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x201,0x209,0x211,0x219,0x221,0x229,0x231,0x239,0x241,0x249,0x251,0x259,0x261,0x269,0x271,
		0x1b1,0x1b9,0x1c1,0x1c9,0x1d1,0x1d9,0x1e1,0x1e9,0x1f1,0x1f9,0x279,0x281,0x289,0x291,0x299,0x2a1,
		0x2a9,0x001,0x009,0x011,0x019,0x021,0x029,0x031,0x039,0x041,0x049,0x051,0x059,0x061,0x069,0x071,
		0x079,0x081,0x089,0x091,0x099,0x0a1,0x0a9,0x0b1,0x0b9,0x0c1,0x0c9,0x2b1,0x2b9,0x2c1,0x2c9,0x2d1,
		0x0ce,0x0d9,0x0e1,0x0e9,0x0f1,0x0f9,0x101,0x109,0x111,0x119,0x121,0x129,0x131,0x139,0x141,0x149,
		0x151,0x159,0x161,0x169,0x171,0x179,0x181,0x189,0x191,0x199,0x1a1,0x2d9,0x2e1,0x2e9,0x2f1,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
		0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,0x0ce,
	};

	const uint8_t g_MicroProfileFont[] = 
	{
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x10,0x78,0x38,0x78,0x7c,0x7c,0x3c,0x44,0x38,0x04,0x44,0x40,0x44,0x44,0x38,0x78,
		0x38,0x78,0x38,0x7c,0x44,0x44,0x44,0x44,0x44,0x7c,0x00,0x00,0x40,0x00,0x04,0x00,
		0x18,0x00,0x40,0x10,0x08,0x40,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x10,0x38,0x7c,0x08,0x7c,0x1c,0x7c,0x38,0x38,
		0x10,0x28,0x28,0x10,0x00,0x20,0x10,0x08,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x04,0x00,0x20,0x38,0x38,0x70,0x00,0x1c,0x10,0x00,0x1c,0x10,0x70,0x30,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x28,0x44,0x44,0x44,0x40,0x40,0x40,0x44,0x10,0x04,0x48,0x40,0x6c,0x44,0x44,0x44,
		0x44,0x44,0x44,0x10,0x44,0x44,0x44,0x44,0x44,0x04,0x00,0x00,0x40,0x00,0x04,0x00,
		0x24,0x00,0x40,0x00,0x00,0x40,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x30,0x44,0x04,0x18,0x40,0x20,0x04,0x44,0x44,
		0x10,0x28,0x28,0x3c,0x44,0x50,0x10,0x10,0x08,0x54,0x10,0x00,0x00,0x00,0x04,0x00,
		0x00,0x08,0x00,0x10,0x44,0x44,0x40,0x40,0x04,0x28,0x00,0x30,0x10,0x18,0x58,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x44,0x44,0x40,0x44,0x40,0x40,0x40,0x44,0x10,0x04,0x50,0x40,0x54,0x64,0x44,0x44,
		0x44,0x44,0x40,0x10,0x44,0x44,0x44,0x28,0x28,0x08,0x00,0x38,0x78,0x3c,0x3c,0x38,
		0x20,0x38,0x78,0x30,0x18,0x44,0x10,0x6c,0x78,0x38,0x78,0x3c,0x5c,0x3c,0x3c,0x44,
		0x44,0x44,0x44,0x44,0x7c,0x00,0x4c,0x10,0x04,0x08,0x28,0x78,0x40,0x08,0x44,0x44,
		0x10,0x00,0x7c,0x50,0x08,0x50,0x00,0x20,0x04,0x38,0x10,0x00,0x00,0x00,0x08,0x10,
		0x10,0x10,0x7c,0x08,0x08,0x54,0x40,0x20,0x04,0x44,0x00,0x30,0x10,0x18,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x44,0x78,0x40,0x44,0x78,0x78,0x40,0x7c,0x10,0x04,0x60,0x40,0x54,0x54,0x44,0x78,
		0x44,0x78,0x38,0x10,0x44,0x44,0x54,0x10,0x10,0x10,0x00,0x04,0x44,0x40,0x44,0x44,
		0x78,0x44,0x44,0x10,0x08,0x48,0x10,0x54,0x44,0x44,0x44,0x44,0x60,0x40,0x10,0x44,
		0x44,0x44,0x28,0x44,0x08,0x00,0x54,0x10,0x18,0x18,0x48,0x04,0x78,0x10,0x38,0x3c,
		0x10,0x00,0x28,0x38,0x10,0x20,0x00,0x20,0x04,0x10,0x7c,0x00,0x7c,0x00,0x10,0x00,
		0x00,0x20,0x00,0x04,0x10,0x5c,0x40,0x10,0x04,0x00,0x00,0x60,0x10,0x0c,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x7c,0x44,0x40,0x44,0x40,0x40,0x4c,0x44,0x10,0x04,0x50,0x40,0x44,0x4c,0x44,0x40,
		0x54,0x50,0x04,0x10,0x44,0x44,0x54,0x28,0x10,0x20,0x00,0x3c,0x44,0x40,0x44,0x7c,
		0x20,0x44,0x44,0x10,0x08,0x70,0x10,0x54,0x44,0x44,0x44,0x44,0x40,0x38,0x10,0x44,
		0x44,0x54,0x10,0x44,0x10,0x00,0x64,0x10,0x20,0x04,0x7c,0x04,0x44,0x20,0x44,0x04,
		0x10,0x00,0x7c,0x14,0x20,0x54,0x00,0x20,0x04,0x38,0x10,0x10,0x00,0x00,0x20,0x10,
		0x10,0x10,0x7c,0x08,0x10,0x58,0x40,0x08,0x04,0x00,0x00,0x30,0x10,0x18,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x44,0x44,0x44,0x44,0x40,0x40,0x44,0x44,0x10,0x44,0x48,0x40,0x44,0x44,0x44,0x40,
		0x48,0x48,0x44,0x10,0x44,0x28,0x6c,0x44,0x10,0x40,0x00,0x44,0x44,0x40,0x44,0x40,
		0x20,0x3c,0x44,0x10,0x08,0x48,0x10,0x54,0x44,0x44,0x44,0x44,0x40,0x04,0x12,0x4c,
		0x28,0x54,0x28,0x3c,0x20,0x00,0x44,0x10,0x40,0x44,0x08,0x44,0x44,0x20,0x44,0x08,
		0x00,0x00,0x28,0x78,0x44,0x48,0x00,0x10,0x08,0x54,0x10,0x10,0x00,0x00,0x40,0x00,
		0x10,0x08,0x00,0x10,0x00,0x40,0x40,0x04,0x04,0x00,0x00,0x30,0x10,0x18,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x44,0x78,0x38,0x78,0x7c,0x40,0x3c,0x44,0x38,0x38,0x44,0x7c,0x44,0x44,0x38,0x40,
		0x34,0x44,0x38,0x10,0x38,0x10,0x44,0x44,0x10,0x7c,0x00,0x3c,0x78,0x3c,0x3c,0x3c,
		0x20,0x04,0x44,0x38,0x48,0x44,0x38,0x44,0x44,0x38,0x78,0x3c,0x40,0x78,0x0c,0x34,
		0x10,0x6c,0x44,0x04,0x7c,0x00,0x38,0x38,0x7c,0x38,0x08,0x38,0x38,0x20,0x38,0x70,
		0x10,0x00,0x28,0x10,0x00,0x34,0x00,0x08,0x10,0x10,0x00,0x20,0x00,0x10,0x00,0x00,
		0x20,0x04,0x00,0x20,0x10,0x3c,0x70,0x00,0x1c,0x00,0x7c,0x1c,0x10,0x70,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x38,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x40,0x04,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	};
}
#endif
#endif
