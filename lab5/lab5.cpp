#include <d3d11_1.h>
#include <directxcolors.h>
#include <windows.h>
#include "DDSTextureLoader.h"
#include <directxmath.h>
#include "framework.h"
#include "resource.h"
#include <d3dcompiler.h>

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
    XMFLOAT2 tex;
};

struct SkyboxVertex
{
    XMFLOAT3 pos;
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
ID3D11ShaderResourceView* g_pCubeTextureRV = nullptr;
ID3D11VertexShader* g_pSkyboxVertexShader = nullptr;
ID3D11PixelShader* g_pSkyboxPixelShader = nullptr;
ID3D11InputLayout* g_pSkyboxInputLayout = nullptr;
ID3D11Buffer* g_pSkyboxVertexBuffer = nullptr;
ID3D11Buffer* g_pSkyboxVPBuffer = nullptr;
ID3D11ShaderResourceView* g_pSkyboxTextureRV = nullptr;
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
float g_CameraAzim = 0.0f;
float g_CameraPlace = 0.0f;



HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
HRESULT InitGraphics();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();


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
            pErrorBlob->Release();
        }
        return hr;
    }
    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
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
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D* pDepthStencil = nullptr;
    hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);


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

    //////////////////////////////////////////////////////////////////////////////

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
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    UINT numElements = ARRAYSIZE(layout);

    hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pCubeInputLayout);
    pVSBlob->Release();
    if (FAILED(hr))
        return hr;

    //g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

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
    { { -0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f } },
    { {  0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f } },
    { {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f } },

    { { -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f } },
    { {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f } },
    { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f } },

    { {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f } },
    { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f } },
    { { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f } },

    { {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f } },
    { { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f } },
    { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f } },

    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f } },
    { { -0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f } },
    { { -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f } },

    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f } },
    { { -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f } },
    { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f } },

    { {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f } },
    { {  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f } },
    { {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f } },

    { {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f } },
    { {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f } },
    { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f } },

    { { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f } },
    { {  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f } },
    { {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f } },

    { { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f } },
    { {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f } },
    { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f } },

    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f } },
    { {  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f } },
    { {  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f } },

    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f } },
    { {  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f } },
    { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f } },
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
    bd.ByteWidth = sizeof(XMMATRIX);
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


    // Textures
    hr = CreateDDSTextureFromFile(g_pd3dDevice, L"cube.dds", nullptr, &g_pCubeTextureRV);
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


    ID3DBlob* pPSSkyboxBlob = nullptr;
    hr = CompileShadersFX(const_cast<wchar_t*>(L"SkyBox.fx"), "SkyboxPS", "ps_4_0", &pPSSkyboxBlob);
    if (FAILED(hr))
        return hr;


    hr = g_pd3dDevice->CreatePixelShader(pPSSkyboxBlob->GetBufferPointer(), pPSSkyboxBlob->GetBufferSize(),
        nullptr, &g_pSkyboxPixelShader);
    pPSSkyboxBlob->Release();
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


void Render()
{
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, g_clearColor);
    g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);


    XMMATRIX view, proj;
    {
        RECT rc;
        GetClientRect(FindWindow(windowClass, windowName), &rc);
        float aspect = static_cast<float>(rc.right - rc.left) / (rc.bottom - rc.top);
        proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.1f, 100.0f);
        float radius = 5.0f;
        float camX = radius * sinf(g_CameraAzim) * cosf(g_CameraPlace);
        float camY = radius * sinf(g_CameraPlace);
        float camZ = radius * cosf(g_CameraAzim) * cosf(g_CameraPlace);
        XMVECTOR Eye = XMVectorSet(camX, camY, camZ, 0.0f);
        XMVECTOR F = XMVectorZero();
        XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        view = XMMatrixLookAtLH(Eye, F, Up);
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


    {
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = true;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        ID3D11DepthStencilState* pDSStateSkybox = nullptr;
        g_pd3dDevice->CreateDepthStencilState(&dsDesc, &pDSStateSkybox);
        g_pImmediateContext->OMSetDepthStencilState(pDSStateSkybox, 0);
        pDSStateSkybox->Release();

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
        g_pImmediateContext->IASetInputLayout(g_pSkyboxInputLayout);
        g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pImmediateContext->VSSetShader(g_pSkyboxVertexShader, nullptr, 0);
        g_pImmediateContext->PSSetShader(g_pSkyboxPixelShader, nullptr, 0);
        g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pSkyboxVPBuffer);
        g_pImmediateContext->PSSetShaderResources(0, 1, &g_pSkyboxTextureRV);
        g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
        g_pImmediateContext->Draw(36, 0);
    }

    g_pImmediateContext->RSSetState(nullptr);
    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);


    {
        g_CubeAngle += 0.01f;
        float radiusCube = 1.0f;
        float cubeX = radiusCube * cosf(g_CubeAngle);
        float cubeY = radiusCube * sinf(g_CubeAngle);
        XMMATRIX model = XMMatrixTranslation(cubeX, cubeY, 0.0f) * XMMatrixRotationY(g_CubeAngle);
        XMMATRIX modelT = XMMatrixTranspose(model);
        g_pImmediateContext->UpdateSubresource(g_pCubeModelBuffer, 0, nullptr, &modelT, 0, 0);

        XMMATRIX vpCube = XMMatrixTranspose(view * proj);
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(g_pImmediateContext->Map(g_pCubeVPBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
        {
            memcpy(mappedResource.pData, &vpCube, sizeof(XMMATRIX));
            g_pImmediateContext->Unmap(g_pCubeVPBuffer, 0);
        }

        UINT stride = sizeof(SimpleVertex);
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
        g_pImmediateContext->Draw(36, 0);
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
}