#include "pch.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"

using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Platform;


// DeviceResources に対するコンストラクター。
DX::DeviceResources::DeviceResources() :
	m_screenViewport(),
	m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1),
	m_d3dRenderTargetSize()
{
	CreateDeviceResources();
}

// Direct3D デバイスを構成し、このハンドルとデバイスのコンテキストを保存します。
void DX::DeviceResources::CreateDeviceResources() 
{
	//このフラグは、カラー チャネルの順序が API の既定値とは異なるサーフェスのサポートを追加します。
	// これは、Direct2D との互換性を保持するために必要です。
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
	if (DX::SdkLayersAvailable())
	{
		// プロジェクトがデバッグ ビルドに含まれる場合、このフラグを使用して SDK レイヤーによるデバッグを有効にします。
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif

	// この配列では、このアプリケーションでサポートされる DirectX ハードウェア機能レベルのセットを定義します。
	// 順序が保存されることに注意してください。
	//アプリケーションの最低限必要な機能レベルをその説明で宣言することを忘れないでください。
	//特に記載がない限り、すべてのアプリケーションは 9.1 をサポートすることが想定されます。
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Direct3D 11 API デバイス オブジェクトと、対応するコンテキストを作成します。
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;

	HRESULT hr = D3D11CreateDevice(
		nullptr,					// 既定のアダプターを使用する nullptr を指定します。
		D3D_DRIVER_TYPE_HARDWARE,	// ハードウェア グラフィックス ドライバーを使用してデバイスを作成します。
		0,							// ドライバーが D3D_DRIVER_TYPE_SOFTWARE でない限り、0 を使用してください。
		creationFlags,				// デバッグ フラグと Direct2D 互換性フラグを設定します。
		featureLevels,				// このアプリがサポートできる機能レベルの一覧を表示します。
		ARRAYSIZE(featureLevels),	// 上記リストのサイズ。
		D3D11_SDK_VERSION,			// Windows ストア アプリでは、これには常に D3D11_SDK_VERSION を設定します。
		&device,					// 作成された Direct3D デバイスを返します。
		&m_d3dFeatureLevel,			// 作成されたデバイスの機能レベルを返します。
		&context					// デバイスのイミディエイト コンテキストを返します。
		);

	if (FAILED(hr))
	{
		// 初期化が失敗した場合は、WARP デバイスにフォール バックします。
		// WARP の詳細については、次を参照してください: 
		// http://go.microsoft.com/fwlink/?LinkId=286690
		DX::ThrowIfFailed(
			D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_WARP, // ハードウェア デバイスの代わりに WARP デバイスを作成します。
				0,
				creationFlags,
				featureLevels,
				ARRAYSIZE(featureLevels),
				D3D11_SDK_VERSION,
				&device,
				&m_d3dFeatureLevel,
				&context
				)
			);
	}

	// Direct3D 11.3 API デバイスへのポインターとイミディエイト コンテキストを保存します。
	DX::ThrowIfFailed(
		device.As(&m_d3dDevice)
		);

	DX::ThrowIfFailed(
		context.As(&m_d3dContext)
		);
}

// これらのリソースは、ウィンドウ サイズが変更されるたびに再作成する必要があります。
void DX::DeviceResources::CreateWindowSizeDependentResources() 
{
	// 前のウィンドウ サイズに固有のコンテキストをクリアします。
	ID3D11RenderTargetView* nullViews[] = {nullptr};
	m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_d3dRenderTargetView = nullptr;
	m_d3dDepthStencilView = nullptr;
	m_d3dContext->Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);

	// スワップ チェーンの幅と高さは、ウィンドウのネイティブ方向の幅と高さに
	// 基づいている必要があります。ウィンドウがネイティブではない場合は、
	// サイズを反転させる必要があります。
	m_d3dRenderTargetSize.Width = 600;
	m_d3dRenderTargetSize.Height = 400;

	if (m_swapChain != nullptr)
	{
		// スワップ チェーンが既に存在する場合は、そのサイズを変更します。
		HRESULT hr = m_swapChain->ResizeBuffers(
			2, // ダブル バッファーされたスワップ チェーンです。
			lround(m_d3dRenderTargetSize.Width),
			lround(m_d3dRenderTargetSize.Height),
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
			);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			//すべての設定が完了しました。このメソッドの実行を続行しないでください。HandleDeviceLost はこのメソッドに再入し、
			// 新しいデバイスを正しく設定します。
			return;
		}
		else
		{
			DX::ThrowIfFailed(hr);
		}
	}
	else
	{
		// それ以外の場合は、既存の Direct3D デバイスと同じアダプターを使用して、新規作成します。
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};

		swapChainDesc.Width = m_d3dRenderTargetSize.Width;		// ウィンドウのサイズと一致させます。
		swapChainDesc.Height = m_d3dRenderTargetSize.Height;
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;				// これは、最も一般的なスワップ チェーンのフォーマットです。
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;								// マルチサンプリングは使いません。
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;									// 遅延を最小限に抑えるにはダブル バッファーを使用します。
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// Windows ストア アプリはすべて、この SwapEffect を使用する必要があります。
		swapChainDesc.Flags = 0;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		// このシーケンスは、上の Direct3D デバイスを作成する際に使用された DXGI ファクトリを取得します。
		ComPtr<IDXGIDevice3> dxgiDevice;
		DX::ThrowIfFailed(
			m_d3dDevice.As(&dxgiDevice)
			);

		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed(
			dxgiDevice->GetAdapter(&dxgiAdapter)
			);

		ComPtr<IDXGIFactory4> dxgiFactory;
		DX::ThrowIfFailed(
			dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
			);

		ComPtr<IDXGISwapChain1> swapChain;
		DX::ThrowIfFailed(
			dxgiFactory->CreateSwapChainForCoreWindow(
				m_d3dDevice.Get(),
				reinterpret_cast<IUnknown*>(m_window.Get()),
				&swapChainDesc,
				nullptr,
				&swapChain
				)
			);
		DX::ThrowIfFailed(
			swapChain.As(&m_swapChain)
			);

		// DXGI が 1 度に複数のフレームをキュー処理していないことを確認します。これにより、遅延が減少し、
		// アプリケーションが各 VSync の後でのみレンダリングすることが保証され、消費電力が最小限に抑えられます。
		DX::ThrowIfFailed(
			dxgiDevice->SetMaximumFrameLatency(1)
			);
	}

	// スワップ チェーンのバック バッファーのレンダリング ターゲット ビューを作成します。
	ComPtr<ID3D11Texture2D1> backBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
		);

	DX::ThrowIfFailed(
		m_d3dDevice->CreateRenderTargetView1(
			backBuffer.Get(),
			nullptr,
			&m_d3dRenderTargetView
			)
		);

	// 必要な場合は 3D レンダリングで使用する深度ステンシル ビューを作成します。
	CD3D11_TEXTURE2D_DESC1 depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT, 
		lround(m_d3dRenderTargetSize.Width),
		lround(m_d3dRenderTargetSize.Height),
		1, // この深度ステンシル ビューには、1 つのテクスチャしかありません。
		1, // 1 つの MIPMAP レベルを使用します。
		D3D11_BIND_DEPTH_STENCIL
		);

	ComPtr<ID3D11Texture2D1> depthStencil;
	DX::ThrowIfFailed(
		m_d3dDevice->CreateTexture2D1(
			&depthStencilDesc,
			nullptr,
			&depthStencil
			)
		);

	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	DX::ThrowIfFailed(
		m_d3dDevice->CreateDepthStencilView(
			depthStencil.Get(),
			&depthStencilViewDesc,
			&m_d3dDepthStencilView
			)
		);
	
	// 3D レンダリング ビューポートをウィンドウ全体をターゲットにするように設定します。
	m_screenViewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		m_d3dRenderTargetSize.Width,
		m_d3dRenderTargetSize.Height
		);

	m_d3dContext->RSSetViewports(1, &m_screenViewport);


	ComPtr<IDXGISurface2> dxgiBackBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))
		);
}

//このメソッドは、CoreWindow オブジェクトが作成 (または再作成) されるときに呼び出されます。
void DX::DeviceResources::SetWindow(CoreWindow^ window)
{
	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	m_window = window;

	CreateWindowSizeDependentResources();
}


// スワップ チェーンの内容を画面に表示します。
void DX::DeviceResources::Present() 
{
	// 最初の引数は、DXGI に VSync までブロックするよう指示し、アプリケーションを次の VSync まで
	// スリープさせます。これにより、画面に表示されることのないフレームをレンダリングして
	// サイクルを無駄にすることがなくなります。
	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	HRESULT hr = m_swapChain->Present1(1, 0, &parameters);

	// レンダリング ターゲットのコンテンツを破棄します。
	//この操作は、既存のコンテンツ全体が上書きされる場合のみ有効です。
	// dirty rect または scroll rect を使用する場合は、この呼び出しを削除する必要があります。
	m_d3dContext->DiscardView1(m_d3dRenderTargetView.Get(), nullptr, 0);

	// 深度ステンシルのコンテンツを破棄します。
	m_d3dContext->DiscardView1(m_d3dDepthStencilView.Get(), nullptr, 0);

	//デバイスが切断またはドライバーの更新によって削除された場合は、
	// すべてのデバイス リソースを再作成する必要があります。
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		return;
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}
}

