#pragma once

#include "Common\DeviceResources.h"
#include "Content\Sample3DSceneRenderer.h"

// Direct2D および 3D コンテンツを画面上でレンダリングします。
namespace App2
{
	class App2Main
	{
	public:
		App2Main(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~App2Main();
		void Update();
		bool Render();

	private:
		// デバイス リソースへのキャッシュされたポインター。
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// TODO: これを独自のコンテンツ レンダラーで置き換えます。
		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;
	};
}