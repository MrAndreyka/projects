//--------------------------------------------------------------------------------------
// ���� 4. ������� � ���������� ����. ������� �� ������� �� DX SDK (c) Microsoft Corp.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <xnamath.h>
#include "resource.h"

#define MX_SETWORLD 0x101

//--------------------------------------------------------------------------------------
// ���������
//--------------------------------------------------------------------------------------

// ��������� �������
struct SimpleVertex
{
    XMFLOAT3 Pos;	 // ���������� ����� � ������������
	int indNorm;	 // ������� �������
};

struct VertexData
{
	XMFLOAT3 Normal;	 // ������� �������
	XMFLOAT4 Color;	 // ���������� ����� � ������������
};

// ��������� ������������ ������ (��������� �� ���������� � �������)
struct ConstantBuffer
{
	XMMATRIX mWorld;		// ������� ����
	XMMATRIX mView;			// ������� ����
	XMMATRIX mProjection;	// ������� ��������
	XMFLOAT4 vLightDir[2];	// ����������� �����
	XMFLOAT4 vLightColor[2];// ���� ���������
	XMFLOAT4 vOutputColor;	// �������� ���� (��� ������� PSSolid)
};



//--------------------------------------------------------------------------------------
// ���������� ����������
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = NULL;
HWND                    g_hWnd = NULL;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = NULL;		// ���������� (��� �������� ��������)
ID3D11DeviceContext*    g_pImmediateContext = NULL;	// �������� (���������� ���������)
IDXGISwapChain*         g_pSwapChain = NULL;		// ���� ����� (������ � �������)
ID3D11RenderTargetView* g_pRenderTargetView = NULL;	// ������ ����, ������ �����
ID3D11Texture2D*        g_pDepthStencil = NULL;		// �������� ������ ������
ID3D11DepthStencilView* g_pDepthStencilView = NULL;	// ������ ����, ����� ������

ID3D11VertexShader*     g_pVertexShader = NULL;		// ��������� ������
ID3D11PixelShader*      g_pPixelShader = NULL;		// ���������� ������ ��� ����
ID3D11PixelShader*      g_pPixelShaderSolid = NULL;	// ���������� ������ ��� ���������� �����
ID3D11InputLayout*      g_pVertexLayout = NULL;		// �������� ������� ������
ID3D11Buffer*           g_pVertexBuffer = NULL;		// ����� ������
ID3D11Buffer*           g_pIndexBuffer = NULL;		// ����� �������� ������
ID3D11Buffer*           g_pConstantBuffer = NULL;	// ����������� �����
ID3D11Buffer*			g_pCBNeverChanges = NULL;	// ����������� �����2

XMMATRIX                g_World;					// ������� ����
XMMATRIX                g_View;						// ������� ����
XMMATRIX                g_Projection;				// ������� ��������
FLOAT					t = 0.0f;					// ����������-�����

XMFLOAT4				vLightDirs[2];				// ����������� ����� (������� ����������)
XMFLOAT4				vLightColors[2];			// ���� ����������


//--------------------------------------------------------------------------------------
// ��������������� ���������� �������
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );  // �������� ����
HRESULT InitDevice();	// ������������� ��������� DirectX
HRESULT InitGeometry();	// ������������� ������� ����� � ������ ������
HRESULT InitMatrixes();	// ������������� ������
void UpdateLight();		// ���������� ���������� �����
void UpdateMatrix(UINT nLightIndex); // ���������� ������� ����
void Render();			// ������� ���������
void CleanupDevice();	// �������� ���������� ��������� DirectX
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );	  // ������� ����


//--------------------------------------------------------------------------------------
// ����� ����� � ���������. ������������� ���� �������� � ���� � ���� ���������.
// ��������� ����� ������������ ��� ��������� �����.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

	// �������� ���� ����������
    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

	// �������� �������� DirectX
    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

	// �������� �������� � ������ ������
    if( FAILED( InitGeometry() ) )
    {
        CleanupDevice();
        return 0;
    }

	// ������������� ������
    if( FAILED( InitMatrixes() ) )
    {
        CleanupDevice();
        return 0;
    }

	// ������� ���� ���������
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            Render(); // ������ �����
        }
    }
	// ����������� ������� DirectX
    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// ����������� ������ � �������� ����
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // ����������� ������
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_ICON1 );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"Urok5WindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_ICON1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // �������� ����
    g_hInst = hInstance;
    RECT rc = { 0, 0, 400, 300 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"Urok5WindowClass", L"���� 5. ������� � ���������� ����", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// ���������� ������ ���, ����� ���������� �������� ��������� ���������
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}


//--------------------------------------------------------------------------------------
// ��������������� ������� ��� ���������� �������� � D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        if( pErrorBlob ) pErrorBlob->Release();
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// �������� ���������� Direct3D (D3D Device), ��������� ���� (Swap Chain) �
// ��������� ���������� (Immediate Context).
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;	// �������� ������
    UINT height = rc.bottom - rc.top;	// � ������ ����

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    // ��� �� ������� ������ �������������� ������ DirectX
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

	// ������ �� �������� ���������� DirectX. ��� ������ �������� ���������,
	// ������� ��������� �������� ��������� ������ � ����������� ��� � ������ ����.
    DXGI_SWAP_CHAIN_DESC sd;			// ���������, ����������� ���� ����� (Swap Chain)
    ZeroMemory( &sd, sizeof( sd ) );	// ������� ��
	sd.BufferCount = 1;					// � ��� ���� �����
    sd.BufferDesc.Width = width;		// ������ ������
    sd.BufferDesc.Height = height;		// ������ ������
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// ������ ������� � ������
    sd.BufferDesc.RefreshRate.Numerator = 75;			// ������� ���������� ������
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// ���������� ������ - ������ �����
    sd.OutputWindow = g_hWnd;							// ����������� � ������ ����
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;						// �� ������������� �����

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if (SUCCEEDED(hr))  // ���� ���������� ������� �������, �� ������� �� �����
            break;
    }
    if (FAILED(hr)) return hr;

    // ������ ������� ������ �����. �������� ��������, � SDK
    // RenderTargetOutput - ��� �������� �����, � RenderTargetView - ������.
	// ��������� �������� ������� ������
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if (FAILED(hr)) return hr;

	// �� ����������� �������� ������� ����������� ���������
    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
    pBackBuffer->Release();
    if (FAILED(hr)) return hr;

    // ��������� � �������� ������ ������
    // ������� ��������-�������� ������ ������
    D3D11_TEXTURE2D_DESC descDepth;	// ��������� � �����������
	ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;		// ������ �
    descDepth.Height = height;		// ������ ��������
    descDepth.MipLevels = 1;		// ������� ������������
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	// ������ (������ �������)
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;		// ��� - ����� ������
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
	// ��� ������ ����������� ���������-�������� ������� ������ ��������
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &g_pDepthStencil );
    if (FAILED(hr)) return hr;

    // ������ ���� ������� ��� ������ ������ ������
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;	// ��������� � �����������
	ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = descDepth.Format;		// ������ ��� � ��������
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
	// ��� ������ ����������� ���������-�������� � �������� ������� ������ ������ ������
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if (FAILED(hr)) return hr;

    // ���������� ������ ������� ������ � ������ ������ ������ � ��������� ����������
    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );

    // ��������� �������� (������� � ������� ���������). � ���������� ������� �� ����������
	// �������������, ���� �� ��� ����� ����.
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

	return S_OK;
}


//--------------------------------------------------------------------------------------
// �������� ������ ������, �������� (shaders) � �������� ������� ������ (input layout)
//--------------------------------------------------------------------------------------
HRESULT InitGeometry()
{
	HRESULT hr = S_OK;

	// ���������� ���������� ������� �� �����
    ID3DBlob* pVSBlob = NULL; // ��������������� ������ - ������ ����� � ����������� ������
    hr = CompileShaderFromFile( L"urok5.fx", "VS", "vs_4_0", &pVSBlob );
    if(FAILED( hr ))
    {
        MessageBox( NULL, L"���������� �������������� ���� FX (VS). ����������, ��������� ������ ��������� �� �����, ���������� ���� FX.", L"������", MB_OK );
        return hr;
    }

	// �������� ���������� �������
	hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader );
	if(FAILED( hr ))
	{	
		pVSBlob->Release();
        return hr;
	}

    // ����������� ������� ������
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R16_UINT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE( layout );

    // �������� ������� ������
	hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
	pVSBlob->Release();
    if (FAILED(hr)) {
		MessageBox(NULL, L"������ c������� ������� ������", L"������", MB_OK);
		return hr;
	}

    // ����������� ������� ������
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

	// ���������� ����������� ������� ��� ��������� �������� ���� �� �����
	ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile( L"urok5.fx", "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL, L"���������� �������������� ���� FX (PS). ����������, ��������� ������ ��������� �� �����, ���������� ���� FX.", L"������", MB_OK );
        return hr;
    }

	// �������� ����������� �������
	hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader );
	pPSBlob->Release();
	if (FAILED(hr)) {
		MessageBox(NULL, L"������ c������� ����������� �������", L"������", MB_OK);
		return hr;
	}

	// ���������� ����������� ������� ��� ���������� ����� �� �����
	pPSBlob = NULL;
	hr = CompileShaderFromFile( L"urok5.fx", "PSSolid", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( NULL, L"���������� �������������� ���� FX. ����������, ��������� ������ ��������� �� �����, ���������� ���� FX.", L"������", MB_OK );
        return hr;
    }

	// �������� ����������� �������
	hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShaderSolid );
	pPSBlob->Release();
    if (FAILED(hr)) return hr;

	VertexData cd[6] = {
	{XMFLOAT3(0.0f, 1.0f, 0.0f),  XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
	{XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
	{XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
	{XMFLOAT3(1.0f, 0.0f, 0.0f),  XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)},
	{XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
	{XMFLOAT3(0.0f, 0.0f, 1.0f) , XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)} };

    // �������� ������ ������ (�� 4 ����� �� ������ ������� ����, ����� 24 �������)
    SimpleVertex vertices[] =
    {	/* ���������� X, Y, Z				������� X, Y, Z			 */
        { XMFLOAT3( -1.0f, 1.0f, -1.0f ),	0 },
        { XMFLOAT3( 1.0f, 1.0f, -1.0f ),	0 },
        { XMFLOAT3( 1.0f, 1.0f, 1.0f ),		0 },
        { XMFLOAT3( -1.0f, 1.0f, 1.0f ),	0 },

        { XMFLOAT3( -1.0f, -1.0f, -1.0f ),	1 },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f ),	1 },
        { XMFLOAT3( 1.0f, -1.0f, 1.0f ),	1 },
        { XMFLOAT3( -1.0f, -1.0f, 1.0f ),	1 },

        { XMFLOAT3( -1.0f, -1.0f, 1.0f ),	2 },
        { XMFLOAT3( -1.0f, -1.0f, -1.0f ),	2 },
        { XMFLOAT3( -1.0f, 1.0f, -1.0f ),	2 },
        { XMFLOAT3( -1.0f, 1.0f, 1.0f ),	2 },

        { XMFLOAT3( 1.0f, -1.0f, 1.0f ),	3 },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f ),	3 },
        { XMFLOAT3( 1.0f, 1.0f, -1.0f ),	3},
        { XMFLOAT3( 1.0f, 1.0f, 1.0f ),		3 },

        { XMFLOAT3( -1.0f, -1.0f, -1.0f ),	4 },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f ),	4 },
        { XMFLOAT3( 1.0f, 1.0f, -1.0f ),	4 },
        { XMFLOAT3( -1.0f, 1.0f, -1.0f ),	4 },

        { XMFLOAT3( -1.0f, -1.0f, 1.0f ),	5 },
        { XMFLOAT3( 1.0f, -1.0f, 1.0f ),	5 },
        { XMFLOAT3( 1.0f, 1.0f, 1.0f ),		5 },
        { XMFLOAT3( -1.0f, 1.0f, 1.0f ),	5 },
    };

	D3D11_BUFFER_DESC bd;	// ���������, ����������� ����������� �����
	ZeroMemory( &bd, sizeof(bd) );				// ������� ��
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * 24;	// ������ ������
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;	// ��� ������ - ����� ������
	bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;	// ���������, ���������� ������ ������
	ZeroMemory( &InitData, sizeof(InitData) );	// ������� ��
    InitData.pSysMem = vertices;				// ��������� �� ���� 8 ������
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if (FAILED(hr)) return hr;

    // �������� ������ ��������
	// 1) c������� ������� � �������
    WORD indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };
	// 2) c������� ������� ������
	bd.Usage = D3D11_USAGE_DEFAULT;		// ���������, ����������� ����������� �����
	bd.ByteWidth = sizeof( WORD ) * 36;	// 36 ������ ��� 12 ������������� (6 ������)
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER; // ��� - ����� ��������
	bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;				// ��������� �� ��� ������ ��������
	// ����� ������ g_pd3dDevice ������� ������ ������ ��������
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
    if (FAILED(hr)) return hr;

    // ��������� ������ ������
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );
    // ��������� ������ ��������
    g_pImmediateContext->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );
    // ��������� ������� ��������� ������ � ������
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	// �������� ������������ ������
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;	// ��� - ����������� �����
	bd.CPUAccessFlags = 0;

	bd.ByteWidth = sizeof(ConstantBuffer);		// ������ ������ = ������� ���������
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pConstantBuffer );
	if (FAILED(hr)) {
		MessageBox(NULL, L"������ c������� ������������ ������", L"������", MB_OK);
		return hr;
	}

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(cd);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	InitData.pSysMem = &cd;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pCBNeverChanges);
	if (FAILED(hr)) {
		MessageBox(NULL, L"������ c������� ������ ������", L"������", MB_OK);
		return hr;
	}
	
	g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCBNeverChanges);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// ������������� ������
//--------------------------------------------------------------------------------------
HRESULT InitMatrixes()
{
    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;	// �������� ������
    UINT height = rc.bottom - rc.top;	// � ������ ����

	// ������������� ������� ����
	g_World = XMMatrixIdentity();

    // ������������� ������� ����
	XMVECTOR Eye = XMVectorSet( 0.0f, 4.0f, -10.0f, 0.0f );	// ������ �������
	XMVECTOR At = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );	// ���� �������
	XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );	// ����������� �����
	g_View = XMMatrixLookAtLH( Eye, At, Up );

    // ������������� ������� ��������
	// ���������: 1) ������ ���� ��������� 2) "������������" �������
	// 3) ����� ������� ������� ���������� 4) ����� ������� ������� ����������
	g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV4, width / (FLOAT)height, 0.01f, 100.0f );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// ��������� ����������� �����
//--------------------------------------------------------------------------------------
void UpdateLight()
{
    // ���������� ����������-�������
    if( g_driverType == D3D_DRIVER_TYPE_REFERENCE )
    {
        t += ( float )XM_PI * 0.0125f;
    }
    else
    {
        static DWORD dwTimeStart = 0;
        DWORD dwTimeCur = GetTickCount();
        if( dwTimeStart == 0 )
            dwTimeStart = dwTimeCur;
        t = ( dwTimeCur - dwTimeStart ) / 1000.0f;
    }

	// ������ ��������� ���������� ���������� �����
	vLightDirs[0] = XMFLOAT4( -0.577f, 0.577f, -0.577f, 1.0f );
	vLightDirs[1] = XMFLOAT4(  0.0f,   0.0f,   -1.0f,   1.0f );
	// ������ ���� ���������� �����, � ��� �� �� ����� ��������
	vLightColors[0] = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
	vLightColors[1] = XMFLOAT4( 1.0f, 0.0f, 0.0f, 1.0f );

	// ��� ������ ������������� ������������ ������ �������� �����
	XMMATRIX mRotate = XMMatrixRotationY( -2.0f * t );
	XMVECTOR vLightDir = XMLoadFloat4( &vLightDirs[1] );
	vLightDir = XMVector3Transform( vLightDir, mRotate );
	XMStoreFloat4( &vLightDirs[1], vLightDir );

	// ��� ������ ������������� ������������ ������ �������� �����
	mRotate = XMMatrixRotationY( 0.5f * t );
	vLightDir = XMLoadFloat4( &vLightDirs[0] );
	vLightDir = XMVector3Transform( vLightDir, mRotate );
	XMStoreFloat4( &vLightDirs[0], vLightDir );
}

//--------------------------------------------------------------------------------------
// ������������� ������� ��� �������� ��������� ����� (0-1) ��� ���� (MX_SETWORLD)
//--------------------------------------------------------------------------------------
void UpdateMatrix(UINT nLightIndex)
{
 	// ��������� �������� �������
	if (nLightIndex == MX_SETWORLD) {
		// ���� ������ ����������� ���: ��� ���� ������ �������� �������
		g_World = XMMatrixRotationAxis( XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f), t );
		nLightIndex = 0;
	}
	else if (nLightIndex < 2) {
		// ���� ������ ��������� �����: ���������� ������� � ����� � ��������� � 5 ���
		g_World = XMMatrixTranslationFromVector( 5.0f * XMLoadFloat4( &vLightDirs[nLightIndex] ) );
		XMMATRIX mLightScale = XMMatrixScaling( 0.2f, 0.2f, 0.2f );
		g_World = mLightScale * g_World;
	}
	else {
		nLightIndex = 0;
	}

    // ���������� ����������� ������������ ������
    ConstantBuffer cb1;	// ��������� ���������
	cb1.mWorld = XMMatrixTranspose( g_World );	// ��������� � ���� �������
	cb1.mView = XMMatrixTranspose( g_View );
	cb1.mProjection = XMMatrixTranspose( g_Projection );
	cb1.vLightDir[0] = vLightDirs[0];			// ��������� ������ � �����
	cb1.vLightDir[1] = vLightDirs[1];
	cb1.vLightColor[0] = vLightColors[0];
	cb1.vLightColor[1] = vLightColors[1];
	cb1.vOutputColor = vLightColors[nLightIndex];
	g_pImmediateContext->UpdateSubresource( g_pConstantBuffer, 0, NULL, &cb1, 0, 0 );	
}


//--------------------------------------------------------------------------------------
// ��������� �����
//--------------------------------------------------------------------------------------
void Render()
{
	// ������� ������ ����� � ����� ����
    float ClearColor[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
    g_pImmediateContext->ClearRenderTargetView( g_pRenderTargetView, ClearColor );
    // ������� ����� ������ �� ������� (������������ �������)
    g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	UpdateLight();	// ��������� ���������

	// ������ ����������� ���
	// 1) ��������� ������� ������������ ����
	UpdateMatrix(MX_SETWORLD);
	// 2) ������������� ������� � ����������� ������
	g_pImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
	g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pConstantBuffer );
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pCBNeverChanges);
	g_pImmediateContext->PSSetShader( g_pPixelShader, NULL, 0 );
	g_pImmediateContext->PSSetConstantBuffers( 0, 1, &g_pConstantBuffer );

	// 3) ������ � ������ ������ 36 ������ 
	g_pImmediateContext->DrawIndexed( 36, 0, 0 );

    // ������ ��� ��������� �����
	// 1) ������������� ���������� ������
	g_pImmediateContext->PSSetShader( g_pPixelShaderSolid, NULL, 0 );
    for( int m = 0; m < 2; m++ )
    {
		// 2) ������������� ������� ���� ��������� �����
		UpdateMatrix( m );
		// 3) ������ � ������ ������ 36 ������ 
		g_pImmediateContext->DrawIndexed( 36, 0, 0 );
    }

    // �������� ������ ����� � �������� (�� �����)
    g_pSwapChain->Present( 0, 0 );
}


//--------------------------------------------------------------------------------------
// ������������ ���� ��������� ��������
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    // ������� �������� �������� ����������
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();
	// ����� ������ �������
    if( g_pConstantBuffer ) g_pConstantBuffer->Release();
	if (g_pCBNeverChanges) g_pCBNeverChanges->Release();
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pIndexBuffer ) g_pIndexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
    if( g_pPixelShaderSolid ) g_pPixelShaderSolid->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}