#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"

namespace App2
{
	// このサンプル レンダリングでは、基本的なレンダリング パイプラインをインスタンス化します。
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void ReleaseDeviceDependentResources();
		void Update();
		void Render();
		bool IsTracking() { return m_tracking; }

	private:
		// デバイス リソースへのキャッシュされたポインター。
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// キューブ ジオメトリの Direct3D リソース。
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;

		// キューブ ジオメトリのシステム リソース。
		ModelViewProjectionConstantBuffer	m_constantBufferData;
		uint32	m_indexCount;

		// レンダリング ループで使用する変数。
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;
	};
}

