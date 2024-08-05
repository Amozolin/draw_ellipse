#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <d2d1.h>
#include <WindowsX.h>

#include "basewin.h"

template <class T> void SafeRelease(T **ppT)
{
	if(*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

class DPIScale
{
	static float scale;

	public:
		static void Initialize(HWND hwnd)
		{
			float dpi = static_cast<float>(GetDpiForWindow(hwnd));
			scale = dpi / USER_DEFAULT_SCREEN_DPI;
		}

		template <typename T>
		static D2D1_POINT_2F PixelsToDips(T x, T y)
		{
			return D2D1::Point2F(static_cast<float>(x) / scale, static_cast<float>(y) / scale);
		}
};

float DPIScale::scale = 1.0f;


class MainWindow : public BaseWindow<MainWindow>
{
	ID2D1Factory *pFactory;
	ID2D1HwndRenderTarget *pRenderTarget;
	ID2D1SolidColorBrush *pBrush;
	D2D1_ELLIPSE ellipse;

	D2D1_POINT_2F ptMouse;
	
	void CalculateLayout();
	HRESULT CreateGraphicsResources();
	void DiscardGraphicsResources();
	void OnPaint();
	void Resize();

	void OnLButtonDown(int pixelX, int pixelY, DWORD flags);
	void OnLButtonUp();
	void OnMouseMove(int pixelX, int pixelY, DWORD flags);

	public:
		MainWindow() : pFactory(NULL), pRenderTarget(NULL), pBrush(NULL), 
			ellipse(D2D1::Ellipse(D2D1::Point2F(), 0, 0)),
			ptMouse(D2D1::Point2F()) 
		{ 
		}
		
		PCWSTR ClassName() const { return L"Circle Window Class"; }
		LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};


// Recalculate drawing layout when the size  of the window changes
void MainWindow::CalculateLayout()
{
//	if(pRenderTarget != NULL)
//	{
//		D2D1_SIZE_F size = pRenderTarget->GetSize();
//		const float x = size.width / 2;
//		const float y = size.height / 2;
//		const float radius = min(x, y);
//		ellipse = D2D1::Ellipse(D2D1::Point2F(x,y), radius, radius);
//	}
}

HRESULT MainWindow::CreateGraphicsResources()
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		
		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
		
		hr = pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&pRenderTarget);

		if(SUCCEEDED(hr))
		{
			const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0.0f);
			hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);

			if(SUCCEEDED(hr))
			{
				CalculateLayout();
			}
		}
	}

	// Set a transform that will be applied to each object drawn until
	// the transform is set back to identity
	//pRenderTarget->SetTransform(D2D1::Matrix3x2F::Skew(20,30, D2D1::Point2F(100,100)));

	// Here how we can reset the transform
	//pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

	return hr;
}	

void MainWindow::DiscardGraphicsResources()
{
	SafeRelease(&pRenderTarget);
	SafeRelease(&pBrush);
}

void MainWindow::OnPaint()
{
	HRESULT hr = CreateGraphicsResources();
	if(SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		
		pRenderTarget->BeginDraw();

		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));
		pRenderTarget->FillEllipse(ellipse, pBrush);

		hr = pRenderTarget->EndDraw();
		if(FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			DiscardGraphicsResources();
		}
		EndPaint(m_hwnd, &ps);
	}
}

void MainWindow::Resize()
{
	if(pRenderTarget != NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		
		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		pRenderTarget->Resize(size);
		CalculateLayout();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}

void MainWindow::OnLButtonDown(int pixelX, int pixelY, DWORD flags)
{
	SetCapture(m_hwnd);
	ellipse.point = ptMouse = DPIScale::PixelsToDips(pixelX, pixelY);
	ellipse.radiusX = ellipse.radiusY = 1.0f;
	InvalidateRect(m_hwnd, NULL, FALSE);
}

void MainWindow::OnLButtonUp()
{
	ReleaseCapture();
}

void MainWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
	if(flags & MK_LBUTTON)
	{
		const D2D1_POINT_2F dips = DPIScale::PixelsToDips(pixelX, pixelY);
	
		const float width = (dips.x - ptMouse.x) / 2;
		const float height = (dips.y - ptMouse.y) / 2;
		const float x1 = ptMouse.x + width;
		const float y1 = ptMouse.y + height;
	
		ellipse = D2D1::Ellipse(D2D1::Point2F(x1, y1), width, height);
	
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	MainWindow win;

	if(!win.Create(L"Circle", WS_OVERLAPPEDWINDOW))
	{
		return 0;
	}
	
	ShowWindow(win.Window(), nCmdShow);
	
	// Message Loop
	// Allows to respond to the messages sent by windows
	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
		if(FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
		{
			return -1;
		}
		DPIScale::Initialize(m_hwnd);
		
		return 0;
	case WM_DESTROY:
		DiscardGraphicsResources();
		SafeRelease(&pFactory);
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		OnPaint();
		return 0;
	case WM_SIZE:
		Resize();
		return 0;
	case WM_LBUTTONDOWN:
		OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD) wParam);
		return 0;
	case WM_LBUTTONUP:
		OnLButtonUp();
		return 0; 
	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD) wParam);
		return 0;
	default:
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}

}
