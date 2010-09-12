/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "d3d1xstutil.h"
#include "galliumd3d11.h"
#include <dxgi.h>
#include <pipe/p_screen.h>
#include <pipe/p_context.h>

HRESULT  D3D11CreateDevice(
	__in_opt   IDXGIAdapter *pAdapter,
	__in   D3D_DRIVER_TYPE DriverType,
	__in   HMODULE Software,
	__in   unsigned Flags,
	__in_ecount_opt( FeatureLevels )  const D3D_FEATURE_LEVEL *pFeatureLevels,
	__in   unsigned FeatureLevels,
	__in   unsigned SDKVersion,
	__out_opt  ID3D11Device **ppDevice,
	__out_opt  D3D_FEATURE_LEVEL *pFeatureLevel,
	__out_opt  ID3D11DeviceContext **ppImmediateContext
)
{
	HRESULT hr;
	ComPtr<IDXGIAdapter1> adapter_to_release;
	if(!pAdapter)
	{
		ComPtr<IDXGIFactory1> factory;
		hr = CreateDXGIFactory1(IID_IDXGIFactory1, (void**)&factory);
		if(!SUCCEEDED(hr))
			return hr;
		hr = factory->EnumAdapters1(0, &adapter_to_release);
		if(!SUCCEEDED(hr))
			return hr;
		pAdapter = adapter_to_release.p;
	}
	ComPtr<IGalliumAdapter> gallium_adapter;
	hr = pAdapter->QueryInterface(IID_IGalliumAdapter, (void**)&gallium_adapter);
	if(!SUCCEEDED(hr))
		return hr;
	struct pipe_screen* screen;
	// TODO: what should D3D_DRIVER_TYPE_SOFTWARE return? fast or reference?
	if(DriverType == D3D_DRIVER_TYPE_REFERENCE)
		screen = gallium_adapter->GetGalliumReferenceSoftwareScreen();
	else if(DriverType == D3D_DRIVER_TYPE_SOFTWARE || DriverType == D3D_DRIVER_TYPE_WARP)
		screen = gallium_adapter->GetGalliumFastSoftwareScreen();
	else
		screen = gallium_adapter->GetGalliumScreen();
	if(!screen)
		return E_FAIL;
	struct pipe_context* context = screen->context_create(screen, 0);
	if(!context)
		return E_FAIL;
	ComPtr<ID3D11Device> device;
	hr = GalliumD3D11DeviceCreate(screen, context, TRUE, Flags, pAdapter, &device);
	if(!SUCCEEDED(hr))
	{
		context->destroy(context);
		return hr;
	}
	if(ppImmediateContext)
		device->GetImmediateContext(ppImmediateContext);
	if(pFeatureLevel)
		*pFeatureLevel = device->GetFeatureLevel();
	if(ppDevice)
		*ppDevice = device.steal();
	return S_OK;
}

HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
	__in_opt IDXGIAdapter* pAdapter,
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	unsigned Flags,
	__in_ecount_opt( FeatureLevels ) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
	unsigned FeatureLevels,
	unsigned SDKVersion,
	__in_opt CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
	__out_opt IDXGISwapChain** ppSwapChain,
	__out_opt ID3D11Device** ppDevice,
	__out_opt D3D_FEATURE_LEVEL* pFeatureLevel,
	__out_opt ID3D11DeviceContext** ppImmediateContext )
{
	ComPtr<ID3D11Device> dev;
	ComPtr<ID3D11DeviceContext> ctx;
	HRESULT hr;
	hr = D3D11CreateDevice(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, (ID3D11Device**)&dev, pFeatureLevel, (ID3D11DeviceContext**)&ctx);
	if(!SUCCEEDED(hr))
		return hr;
	if(ppSwapChain)
	{
		ComPtr<IDXGIFactory> factory;
		ComPtr<IDXGIDevice> dxgi_device;
		ComPtr<IDXGIAdapter> adapter;
		hr = dev->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
		if(!SUCCEEDED(hr))
			return hr;

		hr = dxgi_device->GetAdapter(&adapter);
		if(!SUCCEEDED(hr))
			return hr;

		adapter->GetParent(IID_IDXGIFactory, (void**)&factory);
		hr = factory->CreateSwapChain(dev.p, (DXGI_SWAP_CHAIN_DESC*)pSwapChainDesc, ppSwapChain);
		if(!SUCCEEDED(hr))
			return hr;
	}
	if(ppDevice)
		*ppDevice = dev.steal();
	if(ppImmediateContext)
		*ppImmediateContext = ctx.steal();
	return hr;
}
