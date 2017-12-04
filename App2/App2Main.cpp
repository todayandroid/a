#include "pch.h"
#include "App2Main.h"
#include "Common\DirectXHelper.h"

using namespace App2;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

// アプリケーションの読み込み時にアプリケーション資産を読み込んで初期化します。
App2Main::App2Main(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{
	// TODO: これをアプリのコンテンツの初期化で置き換えます。
	m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(m_deviceResources));
}

App2Main::~App2Main()
{
}

// アプリケーション状態をフレームごとに 1 回更新します。
void App2Main::Update() 
{
	// TODO: これをアプリのコンテンツの更新関数で置き換えます。
	m_sceneRenderer->Update();
}

// 現在のアプリケーション状態に応じて現在のフレームをレンダリングします。
// フレームがレンダリングされ、表示準備が完了すると、true を返します。
bool App2Main::Render() 
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	// ビューポートをリセットして全画面をターゲットとします。
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// レンダリング ターゲットを画面にリセットします。
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// バック バッファーと深度ステンシル ビューをクリアします。
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// シーン オブジェクトをレンダリングします。
	// TODO: これをアプリのコンテンツのレンダリング関数で置き換えます。
	m_sceneRenderer->Render();

	return true;
}

