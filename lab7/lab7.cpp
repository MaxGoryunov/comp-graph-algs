#include <d3d11.h>
#include <directxcolors.h>
#include <windows.h>
#include "DDSTextureLoader.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui.h"
#include <directxmath.h>
#include "framework.h"
#include "resource.h"
#include <d3dcompiler.h>
#include <vector>
#include <algorithm>
#include <iostream>

// InitDevice -> CreateDevice (for future labs)
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

//struct SimpleVertex
//{
//    XMFLOAT3 Pos;
//    XMFLOAT4 Color;
//};

struct SimpleVertex
{
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	XMFLOAT2 tex;
};

struct SkyboxVertex
{
	XMFLOAT3 pos;
};


struct Lighting {
	XMFLOAT4 pos;
	XMFLOAT4 color;
	XMFLOAT4 attenuation;
};

struct InstanceData {
	XMMATRIX modelMatrix;
	int textureIndex;
	XMFLOAT3 padding;
};

struct Plane {
	float a, b, c, d;
};

struct FullScreenVertex {
	float x, y, z, w;
	float u, v;
};


// Consants

struct CBWorld
{
	XMMATRIX mWorld;
};

struct CBViewProjection
{
	XMMATRIX mView;
	XMMATRIX mProjection;
};

// Cubes

struct ColorCube {
	XMFLOAT4 color;
	XMMATRIX position;
	bool hasTexture;
	int texture = 0;
};

struct TransCube {
	XMFLOAT4 color;
	XMMATRIX position;
};

WCHAR windowClass[256] = L"Lab1WindowClass";
WCHAR windowName[256] = L"Горюнов Максим Юрьевич";

HINSTANCE               g_hInst = nullptr;
HWND                    g_hWnd = nullptr;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
XMVECTORF32 g_clearColor = Colors::HotPink;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11SamplerState* g_pSamplerLinear = nullptr;

ID3D11VertexShader* g_pCubeVertexShader = nullptr;
ID3D11PixelShader* g_pCubePixelShader = nullptr;
ID3D11InputLayout* g_pCubeInputLayout = nullptr;
ID3D11Buffer* g_pCubeVertexBuffer = nullptr;
ID3D11Buffer* g_pCubeModelBuffer = nullptr;
ID3D11Buffer* g_pCubeVPBuffer = nullptr;
ID3D11Buffer* g_pCubeIndexBuffer = nullptr;
ID3D11ShaderResourceView* g_pCubeTextureRV = nullptr;
ID3D11VertexShader* g_pSkyboxVertexShader = nullptr;
ID3D11PixelShader* g_pSkyboxPixelShader = nullptr;
ID3D11InputLayout* g_pSkyboxInputLayout = nullptr;
ID3D11Buffer* g_pSkyboxVertexBuffer = nullptr;
ID3D11Buffer* g_pSkyboxVPBuffer = nullptr;
ID3D11Buffer* g_pSkyboxIndexBuffer = nullptr;
ID3D11ShaderResourceView* g_pSkyboxTextureRV = nullptr;
ID3D11VertexShader* g_pColorCubeVertexShader = nullptr;
ID3D11PixelShader* g_pColorCubePixelShader = nullptr;
ID3D11InputLayout* g_pColorCubeInputLayout = nullptr;
ID3D11Buffer* g_pColorCubeModelBuffer = nullptr;
ID3D11Buffer* g_pColorCubeColorBuffer = nullptr;
ID3D11BlendState* g_pBlendState = nullptr;
ID3D11DepthStencilState* g_pTransparentDepthState = nullptr;
ID3D11DepthStencilState* g_pTransparentDepthStencilState = nullptr;
ID3D11BlendState* g_pTransparentBlendState = nullptr;
ID3D11ShaderResourceView* g_pCubeNormalMapRV = nullptr;
ID3D11Buffer* g_pLightBuffer = nullptr;
ID3D11Buffer* g_pCameraBuffer = nullptr;
//ID3D11VertexShader* g_pVertexShader = nullptr;
//ID3D11PixelShader* g_pPixelShader = nullptr;
//ID3D11InputLayout* g_pVertexLayout = nullptr;
//ID3D11Buffer* g_pVertexBuffer = nullptr;
//ID3D11Buffer* g_pIndexBuffer = nullptr;
//ID3D11Buffer* g_pConstantBufferWorld = nullptr;
//ID3D11Buffer* g_pConstantBufferViewProjection = nullptr;
XMMATRIX                g_World;
XMMATRIX                g_View;
XMMATRIX                g_Projection;
float g_CubeAngle = 0.0f;
float g_CameraAngle = 0.0f;
bool g_OnHold = false;
POINT g_MousePrev = { 0, 0 };
XMFLOAT3 g_CameraPos;
float g_CameraAzim = 0.0f;
float g_CameraPlace = 0.0f;
const int MAX_INSTANCES = 100;
ID3D11Buffer* g_pInstanceCB = nullptr;
std::vector<InstanceData> g_Instances;
std::vector<ID3D11ShaderResourceView*> g_CubeTextures;

bool g_EnablePostProcessFilter = false;
ID3D11Texture2D* g_pPostProcessTex = nullptr;
ID3D11RenderTargetView* g_pPostProcessRTV = nullptr;
ID3D11ShaderResourceView* g_pPostProcessSRV = nullptr;
ID3D11VertexShader* g_pPostProcessVS = nullptr;
ID3D11PixelShader* g_pPostProcessPS = nullptr;
ID3D11Buffer* g_pFullScreenVB = nullptr;
ID3D11InputLayout* g_pFullScreenLayout = nullptr;
bool g_EnableFrustumCulling = false;

int g_totalInstances = 0;
int g_finalInstanceCount = 0;

FullScreenVertex g_FullScreenTriangle[3] = { {-1.0f, -1.0f, 0, 1, 0.0f, 1.0f},
											{-1.0f, 3.0f, 0, 1, 0.0f, -1.0f},
											{3.0f, -1.0f, 0, 1, 2.0f, 1.0f} };
// Yellow-ish?
Lighting g_Lightings[2] = {
	{ XMFLOAT4(2.0f, 2.0f, 2.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 0.5f, 0.0f), XMFLOAT4(1.0f, 0.1f, 0.01f, 0.0f) },
	{ XMFLOAT4(0.0f, 2.0f, -2.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 0.5f, 0.0f), XMFLOAT4(1.0f, 0.1f, 0.01f, 0.0f) } };

//Texture Cube
ColorCube g_TexCube = { XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMMatrixTranslation(0.0f, 0.0f, 0.0f), true};
// Solid Cube
ColorCube g_ColCube = { XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), XMMatrixTranslation(1.0f, 2.0f, 1.5f), true };
// Glass cube 1
TransCube g_TransCube1 = { XMFLOAT4(0.0f, 1.0f, 1.0f, 0.5f), XMMatrixTranslation(-2.0f, -1.0f, 0.0f)};
// Glass cube 2
TransCube g_TransCube2 = { XMFLOAT4(1.0f, 0.0f, 1.0f, 0.5f), XMMatrixTranslation(2.5f, 1.0f, 0.0f)};
// Light wraps
ColorCube g_LightCube1 = { XMFLOAT4(g_Lightings[0].color.x, g_Lightings[0].color.y, g_Lightings[0].color.z, 1.0f),
	  XMMatrixScaling(0.1f, 0.1f, 0.1f) * XMMatrixTranslation(g_Lightings[0].pos.x, g_Lightings[0].pos.y, g_Lightings[0].pos.z),
	  false };
ColorCube g_LightCube2 = { XMFLOAT4(g_Lightings[1].color.x, g_Lightings[1].color.y, g_Lightings[1].color.z, 1.0f),
	  XMMatrixScaling(0.2f, 0.2f, 0.2f) * XMMatrixTranslation(g_Lightings[1].pos.x, g_Lightings[1].pos.y, g_Lightings[1].pos.z),
	  false };

WORD g_CubeIndices[] = {
	0, 1, 2, 3, 4, 5,
	6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17,
	18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35
};

WORD g_SkyboxIndices[] = {
	0, 1, 2, 3, 4, 5,
	6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17,
	18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29,
	30, 31, 32, 33, 34, 35
};

std::vector<ColorCube*> g_ColorCubes;
std::vector<TransCube*> g_TransCubes;

   

//-------------------------
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
HRESULT InitGraphics();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();
//-------------------------


void ExtractFrustumPlanes(const XMMATRIX& M, Plane planes[6]) {
	planes[0].a = M.r[0].m128_f32[3] + M.r[0].m128_f32[0];
	planes[0].b = M.r[1].m128_f32[3] + M.r[1].m128_f32[0];
	planes[0].c = M.r[2].m128_f32[3] + M.r[2].m128_f32[0];
	planes[0].d = M.r[3].m128_f32[3] + M.r[3].m128_f32[0];

	planes[1].a = M.r[0].m128_f32[3] - M.r[0].m128_f32[0];
	planes[1].b = M.r[1].m128_f32[3] - M.r[1].m128_f32[0];
	planes[1].c = M.r[2].m128_f32[3] - M.r[2].m128_f32[0];
	planes[1].d = M.r[3].m128_f32[3] - M.r[3].m128_f32[0];

	planes[2].a = M.r[0].m128_f32[3] + M.r[0].m128_f32[1];
	planes[2].b = M.r[1].m128_f32[3] + M.r[1].m128_f32[1];
	planes[2].c = M.r[2].m128_f32[3] + M.r[2].m128_f32[1];
	planes[2].d = M.r[3].m128_f32[3] + M.r[3].m128_f32[1];

	planes[3].a = M.r[0].m128_f32[3] - M.r[0].m128_f32[1];
	planes[3].b = M.r[1].m128_f32[3] - M.r[1].m128_f32[1];
	planes[3].c = M.r[2].m128_f32[3] - M.r[2].m128_f32[1];
	planes[3].d = M.r[3].m128_f32[3] - M.r[3].m128_f32[1];

	planes[4].a = M.r[0].m128_f32[3] + M.r[0].m128_f32[2];
	planes[4].b = M.r[1].m128_f32[3] + M.r[1].m128_f32[2];
	planes[4].c = M.r[2].m128_f32[3] + M.r[2].m128_f32[2];
	planes[4].d = M.r[3].m128_f32[3] + M.r[3].m128_f32[2];

	planes[5].a = M.r[0].m128_f32[3] - M.r[0].m128_f32[2];
	planes[5].b = M.r[1].m128_f32[3] - M.r[1].m128_f32[2];
	planes[5].c = M.r[2].m128_f32[3] - M.r[2].m128_f32[2];
	planes[5].d = M.r[3].m128_f32[3] - M.r[3].m128_f32[2];

	for (int i = 0; i < 6; i++) {
		XMVECTOR v =
			XMVectorSet(planes[i].a, planes[i].b, planes[i].c, planes[i].d);
		v = XMPlaneNormalize(v);
		planes[i].a = XMVectorGetX(v);
		planes[i].b = XMVectorGetY(v);
		planes[i].c = XMVectorGetZ(v);
		planes[i].d = XMVectorGetW(v);
	}
}

bool IsSphereInFrustum(const Plane planes[6], const XMVECTOR& center,
	float radius) {
	for (int i = 0; i < 6; i++) {
		float distance =
			XMVectorGetX(XMVector3Dot(
				center, XMVectorSet(planes[i].a, planes[i].b, planes[i].c, 0.0f))) +
			planes[i].d;
		if (distance < -radius)
			return false;
	}
	return true;
}


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}


	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();

		}
		//OutputDebugString(_T("Rendering...\n"));
	}

	CleanupDevice();

	return (int)msg.wParam;
}


HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = windowClass;
	wcex.hIconSm = nullptr;
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	g_hInst = hInstance;
	RECT rc = { 0, 0, 1000, 600 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(windowClass, windowName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);
	//UpdateWindow(g_hWnd); ???
	return S_OK;
}

//For fx
HRESULT CompileShadersFX(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			MessageBoxA(nullptr, (char*)pErrorBlob->GetBufferPointer(),
				"Could not compile FX.", MB_OK);
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}


void InitImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(FindWindow(windowClass, windowName));

	ImGui_ImplDX11_Init(g_pd3dDevice, g_pImmediateContext);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_LBUTTONDOWN:
		g_OnHold = true;
		g_MousePrev.x = LOWORD(lParam);
		g_MousePrev.y = HIWORD(lParam);
		SetCapture(hWnd);
		break;

	case WM_LBUTTONUP:
		g_OnHold = false;
		ReleaseCapture();
		break;

	case WM_MOUSEMOVE:
		if (g_OnHold)
		{
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			int dx = x - g_MousePrev.x;
			int dy = y - g_MousePrev.y;
			g_CameraAzim -= dx * 0.0015f;
			g_CameraPlace -= dy * 0.0015f;
			if (g_CameraPlace > XM_PIDIV2 - 0.01f)
				g_CameraPlace = XM_PIDIV2 - 0.01f;
			if (g_CameraPlace < -XM_PIDIV2 + 0.01f)
				g_CameraPlace = -XM_PIDIV2 + 0.01f;
			g_MousePrev.x = x;
			g_MousePrev.y = y;
		}
		break;

	case WM_KEYDOWN:
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 2;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0; // TODO: 0 or 60?
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };
	hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		createDeviceFlags, featureLevelArray, 1,
		D3D11_SDK_VERSION, &sd, &g_pSwapChain,
		&g_pd3dDevice, &featureLevel, &g_pImmediateContext);

	if (FAILED(hr))
		return hr;


	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;


	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT; // change foramt
	descDepth.SampleDesc.Count = 1;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	ID3D11Texture2D* pDepthStencil = nullptr;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);
	if (FAILED(hr))
	{
		descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // old
		hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);
		if (FAILED(hr))
			return hr;
	}
	if (FAILED(hr))
		return hr;


	hr = g_pd3dDevice->CreateDepthStencilView(pDepthStencil, nullptr, &g_pDepthStencilView);
	pDepthStencil->Release();
	if (FAILED(hr))
		return hr;


	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	width = rc.right - rc.left;
	height = rc.bottom - rc.top;

	D3D11_TEXTURE2D_DESC ppDesc = {};
	ppDesc.Width = width;
	ppDesc.Height = height;
	ppDesc.MipLevels = 1;
	ppDesc.ArraySize = 1;
	ppDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ppDesc.SampleDesc.Count = 1;
	ppDesc.Usage = D3D11_USAGE_DEFAULT;
	ppDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	hr = g_pd3dDevice->CreateTexture2D(&ppDesc, nullptr, &g_pPostProcessTex);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(g_pPostProcessTex, nullptr,
		&g_pPostProcessRTV);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateShaderResourceView(g_pPostProcessTex, nullptr,
		&g_pPostProcessSRV);
	if (FAILED(hr))
		return hr;

	////////////////////////////////////////////////////////////////////////////////////////////
	//HRESULT hr = S_OK;
	ID3DBlob* pBlob = nullptr;

	ID3DBlob* pVSBlob = nullptr;
	hr = CompileShadersFX(const_cast<wchar_t*>(L"Cube.fx"), "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"The FX file cannot be compiled. Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pCubeVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}


	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	UINT numElements = ARRAYSIZE(layout);
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pCubeInputLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;


	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShadersFX(const_cast<wchar_t*>(L"Cube.fx"), "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"The FX file cannot be compiled. Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pCubePixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;


	SimpleVertex cubeVertices[] = {
	{ { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	{ {  0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
	{ {  0.5f,  0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	{ {  0.5f,  0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { -0.5f,  0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },

	{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },

	{ { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	{ { -0.5f, -0.5f,  0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
	{ { -0.5f,  0.5f,  0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	{ { -0.5f,  0.5f,  0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { -0.5f,  0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },

	{ { 0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
	{ { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
	{ { 0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
	{ { 0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
	{ { 0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
	{ { 0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },

	{ { -0.5f, 0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	{ {  0.5f, 0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
	{ {  0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { -0.5f, 0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	{ {  0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },

	{ { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	{ {  0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
	{ {  0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	{ {  0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { -0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } }
	};
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 36;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = cubeVertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pCubeVertexBuffer);
	if (FAILED(hr))
		return hr;


	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = 2 * sizeof(XMMATRIX);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCubeModelBuffer);
	if (FAILED(hr))
		return hr;

	D3D11_BUFFER_DESC vpBufferDesc = {};
	vpBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vpBufferDesc.ByteWidth = sizeof(XMMATRIX);
	vpBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	vpBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = g_pd3dDevice->CreateBuffer(&vpBufferDesc, nullptr, &g_pCubeVPBuffer);
	if (FAILED(hr))
		return hr;


	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"cube.dds", nullptr, &g_pCubeTextureRV);
	if (FAILED(hr))
		return hr;
	ID3D11ShaderResourceView* tex1, * tex2;
	CreateDDSTextureFromFile(g_pd3dDevice, L"cppicon.dds", nullptr, &tex1);
	CreateDDSTextureFromFile(g_pd3dDevice, L"cube.dds", nullptr, &tex2);
	g_CubeTextures.push_back(g_pCubeTextureRV);
	g_CubeTextures.push_back(tex1);
	g_CubeTextures.push_back(tex2);


	/// Used as normal map.
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"wall.dds", nullptr, &g_pCubeNormalMapRV);
	if (FAILED(hr))
		return hr;

	D3D11_BUFFER_DESC indexBDesc = {};
	indexBDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBDesc.ByteWidth = sizeof(WORD) * 36;
	indexBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA indexInitData = {};
	indexInitData.pSysMem = g_CubeIndices;
	hr = g_pd3dDevice->CreateBuffer(&indexBDesc, &indexInitData, &g_pCubeIndexBuffer);
	if (FAILED(hr))
		return hr;


	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
	if (FAILED(hr))
		return hr;


	ID3DBlob* pVSSkyboxBlob = nullptr;
	hr = CompileShadersFX(const_cast<wchar_t*>(L"SkyBox.fx"), "SkyboxVS", "vs_4_0", &pVSSkyboxBlob);
	if (FAILED(hr))
		return hr;
	hr = g_pd3dDevice->CreateVertexShader(pVSSkyboxBlob->GetBufferPointer(), pVSSkyboxBlob->GetBufferSize(),
		nullptr, &g_pSkyboxVertexShader);
	if (FAILED(hr))
	{
		pVSSkyboxBlob->Release();
		return hr;
	}
	D3D11_INPUT_ELEMENT_DESC skyboxLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	UINT numSkyboxElements = ARRAYSIZE(skyboxLayout);
	hr = g_pd3dDevice->CreateInputLayout(skyboxLayout, numSkyboxElements, pVSSkyboxBlob->GetBufferPointer(), pVSSkyboxBlob->GetBufferSize(),
		&g_pSkyboxInputLayout);
	pVSSkyboxBlob->Release();
	if (FAILED(hr))
		return hr;



	hr = CompileShadersFX(const_cast<wchar_t*>(L"SkyBox.fx"), "SkyboxPS", "ps_4_0", &pBlob);
	if (FAILED(hr))
		return hr;
	hr = g_pd3dDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(),
		nullptr, &g_pSkyboxPixelShader);
	pBlob->Release();
	if (FAILED(hr))
		return hr;

	SkyboxVertex skyboxVertices[] =
	{
		{ { -1.0f, -1.0f, -1.0f } },
		{ { -1.0f,  1.0f, -1.0f } },
		{ {  1.0f,  1.0f, -1.0f } },
		{ { -1.0f, -1.0f, -1.0f } },
		{ {  1.0f,  1.0f, -1.0f } },
		{ {  1.0f, -1.0f, -1.0f } },

		{ {  1.0f, -1.0f,  1.0f } },
		{ {  1.0f,  1.0f,  1.0f } },
		{ { -1.0f,  1.0f,  1.0f } },
		{ {  1.0f, -1.0f,  1.0f } },
		{ { -1.0f,  1.0f,  1.0f } },
		{ { -1.0f, -1.0f,  1.0f } },

		{ { -1.0f, -1.0f,  1.0f } },
		{ { -1.0f,  1.0f,  1.0f } },
		{ { -1.0f,  1.0f, -1.0f } },
		{ { -1.0f, -1.0f,  1.0f } },
		{ { -1.0f,  1.0f, -1.0f } },
		{ { -1.0f, -1.0f, -1.0f } },

		{ {  1.0f, -1.0f, -1.0f } },
		{ {  1.0f,  1.0f, -1.0f } },
		{ {  1.0f,  1.0f,  1.0f } },
		{ {  1.0f, -1.0f, -1.0f } },
		{ {  1.0f,  1.0f,  1.0f } },
		{ {  1.0f, -1.0f,  1.0f } },

		{ { -1.0f,  1.0f, -1.0f } },
		{ { -1.0f,  1.0f,  1.0f } },
		{ {  1.0f,  1.0f,  1.0f } },
		{ { -1.0f,  1.0f, -1.0f } },
		{ {  1.0f,  1.0f,  1.0f } },
		{ {  1.0f,  1.0f, -1.0f } },

		{ { -1.0f, -1.0f,  1.0f } },
		{ { -1.0f, -1.0f, -1.0f } },
		{ {  1.0f, -1.0f, -1.0f } },
		{ { -1.0f, -1.0f,  1.0f } },
		{ {  1.0f, -1.0f, -1.0f } },
		{ {  1.0f, -1.0f,  1.0f } },
	};

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SkyboxVertex) * 36;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	initData.pSysMem = skyboxVertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pSkyboxVertexBuffer);
	if (FAILED(hr))
		return hr;

	vpBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vpBufferDesc.ByteWidth = sizeof(XMMATRIX);
	vpBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	vpBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = g_pd3dDevice->CreateBuffer(&vpBufferDesc, nullptr, &g_pSkyboxVPBuffer);
	if (FAILED(hr))
		return hr;

	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"skybox.dds", nullptr, &g_pSkyboxTextureRV);
	if (FAILED(hr))
		return hr;
	indexBDesc.ByteWidth = sizeof(WORD) * 36;
	indexInitData.pSysMem = g_SkyboxIndices;
	hr = g_pd3dDevice->CreateBuffer(&indexBDesc, &indexInitData, &g_pSkyboxIndexBuffer);
	if (FAILED(hr))
		return hr;



	pBlob = nullptr;

	hr = CompileShadersFX(const_cast<wchar_t*>(L"TransCube.fx"), "VS", "vs_4_0", &pBlob);
	if (FAILED(hr)) return hr;
	hr = g_pd3dDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &g_pColorCubeVertexShader);
	if (FAILED(hr)) { pBlob->Release(); return hr; }

	D3D11_INPUT_ELEMENT_DESC layoutColorDesc[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	UINT numColorDesc = ARRAYSIZE(layoutColorDesc);
	hr = g_pd3dDevice->CreateInputLayout(layoutColorDesc, numColorDesc, pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &g_pColorCubeInputLayout);
	pBlob->Release();
	if (FAILED(hr)) return hr;

	hr = CompileShadersFX(const_cast<wchar_t*>(L"TransCube.fx"), "PS", "ps_4_0", &pBlob);
	if (FAILED(hr)) return hr;
	hr = g_pd3dDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &g_pColorCubePixelShader);
	pBlob->Release();
	if (FAILED(hr)) return hr;

	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = 2 * sizeof(XMMATRIX);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pColorCubeModelBuffer);
	if (FAILED(hr)) return hr;

	bd.ByteWidth = sizeof(XMFLOAT4);
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pColorCubeColorBuffer);
	ID3D11Buffer* placeholder = g_pColorCubeColorBuffer;
	//std::cout << "Color buffer " << g_pColorCubeColorBuffer << 
	if (FAILED(hr)) return hr;

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;

	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = g_pd3dDevice->CreateBlendState(&blendDesc, &g_pTransparentBlendState);
	if (FAILED(hr))
		return hr;

	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = TRUE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	hr = g_pd3dDevice->CreateDepthStencilState(&depthDesc, &g_pTransparentDepthState);
	if (FAILED(hr))
		return hr;

	D3D11_BUFFER_DESC lightBufferDesc = {};
	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = 2 * sizeof(Lighting);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = g_pd3dDevice->CreateBuffer(&lightBufferDesc, nullptr, &g_pLightBuffer);

	D3D11_BUFFER_DESC cameraBufferDesc = {};
	cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraBufferDesc.ByteWidth = sizeof(XMFLOAT4);
	cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = g_pd3dDevice->CreateBuffer(&cameraBufferDesc, nullptr, &g_pCameraBuffer);

	
	hr = CompileShadersFX(const_cast<wchar_t*>(L"post.fx"), "VS", "vs_4_0",
		&pBlob);
	if (FAILED(hr))
		return hr;


	hr = g_pd3dDevice->CreateVertexShader(pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(), nullptr,
		&g_pPostProcessVS);
	if (FAILED(hr))
		return hr;

	D3D11_INPUT_ELEMENT_DESC layoutFsDesc[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
		 D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 4,
		 D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	UINT numFsDesc = ARRAYSIZE(layoutFsDesc);
	hr = g_pd3dDevice->CreateInputLayout(
		layoutFsDesc, numFsDesc, pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(), &g_pFullScreenLayout);
	pBlob->Release();
	if (FAILED(hr))
		return hr;
	
	
	pBlob = nullptr;
	hr = CompileShadersFX(const_cast<wchar_t*>(L"post.fx"), "PS", "ps_4_0",
		&pBlob);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreatePixelShader(pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(), nullptr,
		&g_pPostProcessPS);
	pBlob->Release();
	if (FAILED(hr))
		return hr;

	pBlob = nullptr;
	indexBDesc = {};
	indexBDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBDesc.ByteWidth = sizeof(FullScreenVertex) * 3;
	indexBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	indexBDesc.CPUAccessFlags = 0;

	initData = {};
	initData.pSysMem = g_FullScreenTriangle;

	hr = g_pd3dDevice->CreateBuffer(&indexBDesc, &initData, &g_pFullScreenVB);
	if (FAILED(hr))
		return hr;

	D3D11_BUFFER_DESC cbDesc = { 0 };
	cbDesc.ByteWidth = sizeof(InstanceData) * MAX_INSTANCES;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = g_pd3dDevice->CreateBuffer(&cbDesc, nullptr, &g_pInstanceCB);
	if (FAILED(hr))
		return hr;

	InitImGui();

	return S_OK;
}


//HRESULT InitGraphics()
//{
//    HRESULT hr = S_OK;
//    ID3DBlob* pVSBlob = nullptr;
//    ID3DBlob* pErrorBlob = nullptr;
//
//    hr = D3DCompileFromFile(L"VertexShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
//        "main", "vs_5_0", 0, 0, &pVSBlob, &pErrorBlob);
//    if (FAILED(hr))
//    {
//        if (pErrorBlob)
//        {
//            MessageBoxA(nullptr, (char*)pErrorBlob->GetBufferPointer(), "Vertex Shader Compilation Failed", MB_OK);
//            pErrorBlob->Release();
//        }
//        return hr;
//    }
//
//    hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
//        nullptr, &g_pVertexShader);
//    if (FAILED(hr))
//    {
//        pVSBlob->Release();
//        return hr;
//    }
//
//    D3D11_INPUT_ELEMENT_DESC layoutDesc[] =
//    {
//        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
//    };
//    UINT numElements = ARRAYSIZE(layoutDesc);
//    hr = g_pd3dDevice->CreateInputLayout(layoutDesc, numElements, pVSBlob->GetBufferPointer(),
//        pVSBlob->GetBufferSize(), &g_pVertexLayout);
//    pVSBlob->Release();
//    if (FAILED(hr))
//        return hr;
//
//    ID3DBlob* pPSBlob = nullptr;
//    hr = D3DCompileFromFile(L"PixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
//        "main", "ps_5_0", 0, 0, &pPSBlob, &pErrorBlob);
//    if (FAILED(hr))
//    {
//        if (pErrorBlob)
//        {
//            MessageBoxA(nullptr, (char*)pErrorBlob->GetBufferPointer(), "Pixel Shader Compilation Failed", MB_OK);
//            pErrorBlob->Release();
//        }
//        return hr;
//    }
//
//    hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
//        nullptr, &g_pPixelShader);
//    pPSBlob->Release();
//    if (FAILED(hr))
//        return hr;
//
//    SimpleVertex vertices[] = {
//        {{0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
//        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
//        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
//    };
//
//    D3D11_BUFFER_DESC bd = {};
//    bd.Usage = D3D11_USAGE_DEFAULT;
//    bd.ByteWidth = sizeof(SimpleVertex) * 3;
//    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//    bd.CPUAccessFlags = 0;
//
//    D3D11_SUBRESOURCE_DATA initData = {};
//    initData.pSysMem = vertices;
//    hr = g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pVertexBuffer);
//    if (FAILED(hr))
//        return hr;
//
//    return S_OK;
//}



void PostDraw()
{
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pCubeVertexBuffer, &stride, &offset);
	g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCubeVPBuffer);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pImmediateContext->Draw(36, 0);
}

void RenderColorCube(ColorCube* cube)
{
	if (cube->hasTexture) {
		g_pImmediateContext->IASetInputLayout(g_pCubeInputLayout);
		g_pImmediateContext->VSSetShader(g_pCubeVertexShader, nullptr, 0);
		g_pImmediateContext->PSSetShader(g_pCubePixelShader, nullptr, 0);
		ID3D11ShaderResourceView* textures[2] = { g_pCubeTextureRV, g_pCubeNormalMapRV };
		g_pImmediateContext->PSSetShaderResources(0, 2, textures);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
		XMMATRIX posT = XMMatrixTranspose(cube->position);
		g_pImmediateContext->UpdateSubresource(g_pCubeModelBuffer, 0, nullptr, &posT, 0, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCubeModelBuffer);
		g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCubeVPBuffer);
		g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pLightBuffer);
		g_pImmediateContext->PSSetConstantBuffers(3, 1, &g_pCameraBuffer);
	}
	else {
		g_pImmediateContext->IASetInputLayout(g_pColorCubeInputLayout);
		g_pImmediateContext->VSSetShader(g_pColorCubeVertexShader, nullptr, 0);
		g_pImmediateContext->PSSetShader(g_pColorCubePixelShader, nullptr, 0);
		XMMATRIX posT = XMMatrixTranspose(cube->position);
		g_pImmediateContext->UpdateSubresource(g_pColorCubeModelBuffer, 0, nullptr, &posT, 0, 0);
		g_pImmediateContext->UpdateSubresource(g_pColorCubeColorBuffer, 0, nullptr, &(cube->color), 0, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pColorCubeModelBuffer);
		g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCubeVPBuffer);
		g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pColorCubeColorBuffer);
		g_pImmediateContext->PSSetConstantBuffers(3, 1, &g_pLightBuffer);
		g_pImmediateContext->PSSetConstantBuffers(4, 1, &g_pCameraBuffer);
	}
	g_pImmediateContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	g_pImmediateContext->OMSetDepthStencilState(nullptr, 0);
	PostDraw();
}

void RenderTransCube(TransCube* cube)
{
	g_pImmediateContext->IASetInputLayout(g_pColorCubeInputLayout);
	g_pImmediateContext->VSSetShader(g_pColorCubeVertexShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pColorCubePixelShader, nullptr, 0);
	XMMATRIX posT = XMMatrixTranspose(cube->position);
	g_pImmediateContext->UpdateSubresource(g_pColorCubeModelBuffer, 0, nullptr, &posT, 0, 0);
	g_pImmediateContext->UpdateSubresource(g_pColorCubeColorBuffer, 0, nullptr, &(cube->color), 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pColorCubeModelBuffer);
	g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCubeVPBuffer);
	g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pColorCubeColorBuffer);
	g_pImmediateContext->PSSetConstantBuffers(3, 1, &g_pLightBuffer);
	g_pImmediateContext->PSSetConstantBuffers(4, 1, &g_pCameraBuffer);

	float blending[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pImmediateContext->OMSetBlendState(g_pTransparentBlendState, blending, 0xffffffff);
	g_pImmediateContext->OMSetDepthStencilState(g_pTransparentDepthState, 0);
	PostDraw();
}

void DrawCubes(std::vector<InstanceData>& visibleInstances,
	Plane* frustumPlanes, int cubeCount, float radius) {
	for (int i = 0; i < cubeCount; i++) {
		// Calculate fixed position in circle (X/Z)
		float angle = XM_2PI * i / cubeCount; // Base angle for circular arrangement
		float x = radius * cosf(angle);
		float z = radius * sinf(angle);

		// Vertical movement using sine wave with phase offset
		float y = radius * 0.5f * sinf(angle); // Adjust amplitude as needed

		XMVECTOR cubePos = XMVectorSet(x, y, z, 1.0f);
		if (g_EnableFrustumCulling &&
			!IsSphereInFrustum(frustumPlanes, cubePos, 1.2f)) {
			continue;
		}

		// Create world matrix with scaling, optional rotation, and translation
		XMMATRIX world = XMMatrixScaling(1.2f, 1.2f, 1.2f) *
			XMMatrixRotationY(angle) * // Remove if no rotation needed
			XMMatrixTranslation(x, y, z);

		InstanceData data;
		data.modelMatrix = XMMatrixTranspose(world);
		data.textureIndex = i % g_CubeTextures.size();
		visibleInstances.push_back(data);
	}
}


void Render()
{
	ID3D11RenderTargetView* mainRTV = g_pRenderTargetView;
	ID3D11DepthStencilView* mainDSV = g_pDepthStencilView;
	//g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
	if (g_EnablePostProcessFilter) {
		g_pImmediateContext->OMSetRenderTargets(1, &g_pPostProcessRTV, mainDSV);
		mainRTV = g_pPostProcessRTV;
	}
	else {
		g_pImmediateContext->OMSetRenderTargets(1, &mainRTV, mainDSV);
	}
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, g_clearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	XMVECTOR camView;
	XMMATRIX view, proj;
	{
		RECT rc;
		GetClientRect(FindWindow(windowClass, windowName), &rc);
		float aspect = static_cast<float>(rc.right - rc.left) / (rc.bottom - rc.top);
		proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 100.0f);
		float radius = 7.0f;
		float camX = radius * sinf(g_CameraAzim) * cosf(g_CameraPlace);
		float camY = radius * sinf(g_CameraPlace);
		float camZ = radius * cosf(g_CameraAzim) * cosf(g_CameraPlace);
		XMVECTOR Eye = XMVectorSet(camX, camY, camZ, 0.0f);
		XMVECTOR F = XMVectorZero();
		XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		view = XMMatrixLookAtLH(Eye, F, Up);
		camView = XMVectorSet(camX, camY, camZ, 0.0f);
		XMStoreFloat3(&g_CameraPos, camView);
		D3D11_MAPPED_SUBRESOURCE mapped;
		if (SUCCEEDED(g_pImmediateContext->Map(g_pCameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
			memcpy(mapped.pData, &g_CameraPos, sizeof(XMFLOAT3));
			g_pImmediateContext->Unmap(g_pCameraBuffer, 0);
		}
	}

	XMMATRIX viewSkybox = view;
	viewSkybox.r[3] = XMVectorSet(0, 0, 0, 1);
	XMMATRIX vpSkybox = XMMatrixTranspose(viewSkybox * proj);
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if (SUCCEEDED(g_pImmediateContext->Map(g_pSkyboxVPBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
	{
		memcpy(mappedResource.pData, &vpSkybox, sizeof(XMMATRIX));
		g_pImmediateContext->Unmap(g_pSkyboxVPBuffer, 0);
	}
	D3D11_MAPPED_SUBRESOURCE mapped;
	if (SUCCEEDED(g_pImmediateContext->Map(g_pLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
		memcpy(mapped.pData, g_Lightings, sizeof(Lighting) * 2);
		g_pImmediateContext->Unmap(g_pLightBuffer, 0);
	}

	//Skybox
	{
		D3D11_DEPTH_STENCIL_DESC dsDesc = {};
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		ID3D11DepthStencilState* pDSStateSkybox = nullptr;
		if (SUCCEEDED(g_pd3dDevice->CreateDepthStencilState(&dsDesc, &pDSStateSkybox))) {
			g_pImmediateContext->OMSetDepthStencilState(pDSStateSkybox, 0);
			pDSStateSkybox->Release();
		}

		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_FRONT;
		rsDesc.FrontCounterClockwise = false;
		ID3D11RasterizerState* pSkyboxRS = nullptr;
		if (SUCCEEDED(g_pd3dDevice->CreateRasterizerState(&rsDesc, &pSkyboxRS)))
		{
			g_pImmediateContext->RSSetState(pSkyboxRS);
			pSkyboxRS->Release();
		}

		UINT stride = sizeof(SkyboxVertex);
		UINT offset = 0;
		g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pSkyboxVertexBuffer, &stride, &offset);
		g_pImmediateContext->IASetIndexBuffer(g_pSkyboxIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		g_pImmediateContext->IASetInputLayout(g_pSkyboxInputLayout);
		g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		g_pImmediateContext->VSSetShader(g_pSkyboxVertexShader, nullptr, 0);
		g_pImmediateContext->PSSetShader(g_pSkyboxPixelShader, nullptr, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pSkyboxVPBuffer);
		g_pImmediateContext->PSSetShaderResources(0, 1, &g_pSkyboxTextureRV);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
		g_pImmediateContext->DrawIndexed(36, 0, 0);
	}
	g_pImmediateContext->RSSetState(nullptr);
	
	//g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);


	{
		g_CubeAngle += 0.01f;
		float radiusCube = 1.0f;
		float cubeX = radiusCube * cosf(g_CubeAngle);
		float cubeY = radiusCube * sinf(g_CubeAngle);
		XMMATRIX model = XMMatrixTranslation(cubeX, cubeY, 0.0f) * XMMatrixRotationY(g_CubeAngle);
		g_TexCube.position = model;

		XMMATRIX vpPlain = view * proj;
		XMMATRIX vpCube = XMMatrixTranspose(view * proj);
		Plane frustumPlanes[6];
		ExtractFrustumPlanes(vpPlain, frustumPlanes);
		for (int i = 0; i < 6; i++) {
			float len = sqrtf(frustumPlanes[i].a * frustumPlanes[i].a +
				frustumPlanes[i].b * frustumPlanes[i].b +
				frustumPlanes[i].c * frustumPlanes[i].c);
			frustumPlanes[i].a /= len;
			frustumPlanes[i].b /= len;
			frustumPlanes[i].c /= len;
			frustumPlanes[i].d /= len;
		}

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if (SUCCEEDED(g_pImmediateContext->Map(g_pCubeVPBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			memcpy(mappedResource.pData, &vpCube, sizeof(XMMATRIX));
			g_pImmediateContext->Unmap(g_pCubeVPBuffer, 0);
		}

		g_Instances.clear();
		g_ColorCubes.clear();
		g_ColorCubes.push_back(&g_TexCube);
		g_ColorCubes.push_back(&g_ColCube);
		g_ColorCubes.push_back(&g_LightCube1);
		g_ColorCubes.push_back(&g_LightCube2);

		g_TransCubes.clear();
		if (XMVectorGetX(XMVector3LengthSq(g_TransCube1.position.r[3] - camView)) >
			XMVectorGetX(XMVector3LengthSq(g_TransCube2.position.r[3] - camView)))
		{
			g_TransCubes.push_back(&g_TransCube1);
			g_TransCubes.push_back(&g_TransCube2);
		}
		else {
			g_TransCubes.push_back(&g_TransCube2);
			g_TransCubes.push_back(&g_TransCube1);
		}

		int cubeCount = 10;
		std::vector<InstanceData> visibleInstances;
		g_totalInstances = 2 * cubeCount;
		float radius = 3.0f;
		DrawCubes(visibleInstances, frustumPlanes, cubeCount, radius);
		radius = 15.0f;
		DrawCubes(visibleInstances, frustumPlanes, cubeCount, radius);

		g_finalInstanceCount = static_cast<int>(visibleInstances.size());

		for (auto& cube : g_Cubes) {
			if (cube.isTextured) {
				XMVECTOR cubePos = cube.modelMatrix.r[3];
				if (g_EnableFrustumCulling &&
					!IsSphereInFrustum(frustumPlanes, cubePos, 1.0f)) {
					continue;
				}

				InstanceData data;
				data.modelMatrix = XMMatrixTranspose(cube.modelMatrix);
				data.textureIndex = cube.textureIndex;
				visibleInstances.push_back(data);
				g_finalInstanceCount++;
			}
			else {
				if (cube.isTransparent)
					g_TransparentObjects.push_back(&cube);
				else
					g_NonTransparentObjects.push_back(&cube);
			}
		}

		for (auto& cube : g_ColorCubes)
		{
			RenderColorCube(cube);
		}

		for (auto& cube : g_TransCubes)
		{
			RenderTransCube(cube);
		}




		/*UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pCubeVertexBuffer, &stride, &offset);
		g_pImmediateContext->IASetInputLayout(g_pCubeInputLayout);
		g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		g_pImmediateContext->VSSetShader(g_pCubeVertexShader, nullptr, 0);
		g_pImmediateContext->PSSetShader(g_pCubePixelShader, nullptr, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCubeModelBuffer);
		g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCubeVPBuffer);
		g_pImmediateContext->PSSetShaderResources(0, 1, &g_pCubeTextureRV);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
		g_pImmediateContext->Draw(36, 0);*/
	}

	g_pSwapChain->Present(1, 0);
}



void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();
	if (g_pCubeVertexBuffer) g_pCubeVertexBuffer->Release();
	if (g_pCubeInputLayout) g_pCubeInputLayout->Release();
	if (g_pCubeVertexShader) g_pCubeVertexShader->Release();
	if (g_pCubePixelShader) g_pCubePixelShader->Release();
	if (g_pCubeModelBuffer) g_pCubeModelBuffer->Release();
	if (g_pCubeVPBuffer) g_pCubeVPBuffer->Release();
	if (g_pCubeTextureRV) g_pCubeTextureRV->Release();
	if (g_pSamplerLinear) g_pSamplerLinear->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
	if (g_pSkyboxVertexBuffer) g_pSkyboxVertexBuffer->Release();
	if (g_pSkyboxInputLayout) g_pSkyboxInputLayout->Release();
	if (g_pSkyboxVertexShader) g_pSkyboxVertexShader->Release();
	if (g_pSkyboxPixelShader) g_pSkyboxPixelShader->Release();
	if (g_pSkyboxVPBuffer) g_pSkyboxVPBuffer->Release();
	if (g_pSkyboxTextureRV) g_pSkyboxTextureRV->Release();
	if (g_pLightBuffer) g_pLightBuffer->Release();
	if (g_pCameraBuffer) g_pCameraBuffer->Release();
	if (g_pCubeNormalMapRV) g_pCubeNormalMapRV->Release();
	if (g_pColorCubeModelBuffer) g_pColorCubeModelBuffer->Release();
	if (g_pColorCubeColorBuffer) g_pColorCubeColorBuffer->Release();
	if (g_pColorCubeInputLayout) g_pColorCubeInputLayout->Release();
	if (g_pColorCubeVertexShader) g_pColorCubeVertexShader->Release();
	if (g_pColorCubePixelShader) g_pColorCubePixelShader->Release();
	if (g_pBlendState) g_pBlendState->Release();
	if (g_pTransparentDepthState) g_pTransparentDepthState->Release();
	if (g_pTransparentDepthStencilState) g_pTransparentDepthStencilState->Release();
	if (g_pTransparentBlendState) g_pTransparentBlendState->Release();
	if (g_pCubeNormalMapRV) g_pCubeNormalMapRV->Release();
	if (g_pCubeIndexBuffer) g_pCubeIndexBuffer->Release();
	if (g_pSkyboxIndexBuffer) g_pSkyboxIndexBuffer->Release();
	if (g_pPostProcessTex) g_pPostProcessTex->Release();
	if (g_pPostProcessRTV) g_pPostProcessRTV->Release();
	if (g_pPostProcessSRV) g_pPostProcessSRV->Release();
	if (g_pPostProcessVS) g_pPostProcessVS->Release();
	if (g_pPostProcessPS) g_pPostProcessPS->Release();
	if (g_pFullScreenVB) g_pFullScreenVB->Release();
	if (g_pFullScreenLayout) g_pFullScreenLayout->Release();
	if (g_pInstanceCB) g_pInstanceCB->Release();

	for (auto& texture : g_CubeTextures) {
		if (texture)
			texture->Release();
	}
	g_CubeTextures.clear();


	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}