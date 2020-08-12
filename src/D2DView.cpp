
#include "stdafx.h"
#include "D2DView.h"
#include "ContainerApp.h"
#include "Text.h"
#include "Engine.h"
#include <fmt/format.h>
#include <Shlwapi.h>
#include <CommCtrl.h>
#include "scheme.h"
#include <dwrite.h>
#include "Utility.h"
#include <wincodec.h>
#include <wrl.h>
#include <wil/com.h>

#define CALL0(who) Scall0(Stop_level_value(Sstring_to_symbol(who)))
#define CALL1(who, arg) Scall1(Stop_level_value(Sstring_to_symbol(who)), arg)
#define CALL2(who, arg, arg2) Scall2(Stop_level_value(Sstring_to_symbol(who)), arg, arg2)

HWND main_window = nullptr;
HANDLE g_image_rotation_mutex=nullptr;

float prefer_width = 800.0f;
float prefer_height = 600.0f;
// represents the visible surface on the window itself.

ID2D1HwndRenderTarget* pRenderTarget;

// stoke width 
float d2d_stroke_width = 1.3;
ID2D1StrokeStyle* d2d_stroke_style = nullptr;

// colours and brushes used when drawing
ID2D1SolidColorBrush* pColourBrush = nullptr;       // line-color
ID2D1SolidColorBrush* pfillColourBrush = nullptr;   // fill-color
ID2D1BitmapBrush* pPatternBrush = nullptr;          // brush-pattern
ID2D1BitmapBrush* pTileBrush = nullptr;				// tile its U/S

// double buffers
// this is normally pointed at bitmap 
ID2D1RenderTarget* ActiveRenderTarget = nullptr;
// bitmap and bitmap2 are swapped 
// functions normally draw on this
ID2D1Bitmap* bitmap = nullptr;
ID2D1BitmapRenderTarget* BitmapRenderTarget = nullptr;
// this is normally what is being displayed
ID2D1Bitmap* bitmap2 = nullptr;
ID2D1BitmapRenderTarget* BitmapRenderTarget2 = nullptr;




ID2D1Factory* pD2DFactory;

// hiDPI
float g_DPIScaleX = 1.0f;
float g_DPIScaleY = 1.0f;
float graphics_pen_width = static_cast<float>(1.2);

#pragma warning(disable : 4996)

void d2d_DPIScale(ID2D1Factory* pFactory)
{
	FLOAT dpiX, dpiY;
	pFactory->GetDesktopDpi(&dpiX, &dpiY);
	g_DPIScaleX = dpiX / 96.0f;
	g_DPIScaleY = dpiY / 96.0f;
}

void d2d_set_main_window(HWND w) {
	main_window = w;
}

ptr d2d_color(float r, float g, float b, float a) {

	if (BitmapRenderTarget == NULL)
	{
		return Snil;
	}
	SafeRelease(&pColourBrush);
	HRESULT hr = BitmapRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF(r, g, b, a)),
		&pColourBrush
	);
	return Strue;
}


ptr d2d_fill_color(float r, float g, float b, float a) {

	if (BitmapRenderTarget == nullptr)
	{
		return Snil;
	}
	SafeRelease(&pfillColourBrush);
	HRESULT hr = BitmapRenderTarget->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF(r, g, b, a)),
		&pfillColourBrush
	);
	return Strue;
}

void CheckFillBrush()
{
	if (pfillColourBrush == nullptr) {
		d2d_fill_color(0.0, 0.0, 0.0, 1.0);
	}
}

void CheckLineBrush()
{
	if (pColourBrush == nullptr) {
		d2d_color(0.0, 0.0, 0.0, 1.0);
	}
}


void d2d_CreateOffscreenBitmap()
{
	if (pRenderTarget == nullptr)
	{
		return;
	}

	if (BitmapRenderTarget == NULL) {
		pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(prefer_width, prefer_height), &BitmapRenderTarget);
		BitmapRenderTarget->GetBitmap(&bitmap);
	}
	if (BitmapRenderTarget2 == NULL) {
		pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(prefer_width, prefer_height), &BitmapRenderTarget2);
		BitmapRenderTarget2->GetBitmap(&bitmap2);
	}

	ActiveRenderTarget = BitmapRenderTarget;
}

void swap_buffers(int n) {

	if (pRenderTarget == nullptr) {
		return;
	}

	ID2D1Bitmap* temp;
	d2d_CreateOffscreenBitmap();
	if (n == 1) {
		BitmapRenderTarget2->BeginDraw();
		BitmapRenderTarget2->DrawBitmap(bitmap, D2D1::RectF(0.0f, 0.0f, prefer_width, prefer_height));
		BitmapRenderTarget2->EndDraw();
	}
	temp = bitmap;
	bitmap = bitmap2;
	bitmap2 = temp;
	ID2D1BitmapRenderTarget* temptarget;
	temptarget = BitmapRenderTarget;
	BitmapRenderTarget = BitmapRenderTarget2;
	BitmapRenderTarget2 = temptarget;
	// reset on swap
	ActiveRenderTarget = BitmapRenderTarget;

	if (main_window != nullptr) {

		InvalidateRect(main_window, NULL, FALSE);
	}
	Sleep(1);
}


ptr d2d_clear(float r, float g, float b, float a) {

	if (pRenderTarget == nullptr || ActiveRenderTarget ==nullptr) {
		return Sfalse;
	}
	WaitForSingleObject(g_image_rotation_mutex, INFINITE);
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->Clear((D2D1::ColorF(r, g, b, a)));
	ActiveRenderTarget->EndDraw();
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->Clear((D2D1::ColorF(r, g, b, a)));
	ActiveRenderTarget->EndDraw();
	ReleaseMutex(g_image_rotation_mutex);
	return Strue;
}


// cannot save hw rendered bitmap to image :( 
ptr d2d_save(char* filename) {
	return Snil;
}


ptr d2d_show(int n)
{
	swap_buffers(n);
	if (main_window != nullptr)
		PostMessageW(main_window, WM_USER + 501, (WPARAM)0, (LPARAM)0);
	return Strue;
}

ptr d2d_set_stroke_width(float w) {
	d2d_stroke_width = w;
	return Strue;
}

ptr d2d_zfill_ellipse(float x, float y, float w, float h) {
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	ActiveRenderTarget->FillEllipse(ellipse, pfillColourBrush);
	return Strue;
}

ptr d2d_fill_ellipse(float x, float y, float w, float h) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}

	CheckFillBrush();
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->FillEllipse(ellipse, pfillColourBrush);
	ActiveRenderTarget->EndDraw();
	return Strue;
}


ptr d2d_zellipse(float x, float y, float w, float h) {
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);
	ActiveRenderTarget->DrawEllipse(ellipse, pColourBrush, d2d_stroke_width);
	return Strue;
}
ptr d2d_ellipse(float x, float y, float w, float h) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	if (pColourBrush == nullptr) {
		d2d_color(0.0, 0.0, 0.0, 1.0);
	}
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	auto ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), w, h);

	BitmapRenderTarget->BeginDraw();
	BitmapRenderTarget->DrawEllipse(ellipse, pColourBrush, d2d_stroke_width);
	BitmapRenderTarget->EndDraw();

	return Strue;
}

ptr d2d_zline(float x, float y, float x1, float y1) {
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	auto p1 = D2D1::Point2F(x, y);
	auto p2 = D2D1::Point2F(x1, y1);
	ActiveRenderTarget->DrawLine(p1, p2, pColourBrush, stroke_width, stroke_style);
	return Strue;
}



ptr d2d_line(float x, float y, float x1, float y1) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckLineBrush();
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	auto p1 = D2D1::Point2F(x, y);
	auto p2 = D2D1::Point2F(x1, y1);

	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawLine(p1, p2, pColourBrush, stroke_width, stroke_style);
	ActiveRenderTarget->EndDraw();

	return Strue;
}


// runs lambda f inside a draw operation
ptr d2d_draw_func(ptr f) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckLineBrush();
	CheckFillBrush();
	try {
		ActiveRenderTarget->BeginDraw();
		ptr result=Engine::Run(f);
		ActiveRenderTarget->EndDraw();
		return result;
	}
	catch (const CException& e)
	{
		return Sfalse;
	}
	return Strue;
}


 
ptr d2d_zrectangle(float x, float y, float w, float h) {
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	ActiveRenderTarget->DrawRectangle(&rectangle1, pColourBrush, stroke_width);
	return Strue;
}

ptr d2d_rectangle(float x, float y, float w, float h) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckLineBrush();
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);

	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawRectangle(&rectangle1, pColourBrush, stroke_width);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_rounded_rectangle(float x, float y, float w, float h, float rx, float ry) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckLineBrush();
	auto stroke_width = d2d_stroke_width;
	auto stroke_style = d2d_stroke_style;
	D2D1_ROUNDED_RECT rectangle1 = { D2D1::RectF(x, y, w, h), rx, ry };
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawRoundedRectangle(rectangle1, pfillColourBrush, stroke_width, stroke_style);
	ActiveRenderTarget->EndDraw();
	return Strue;
}


ptr d2d_zfill_rectangle(float x, float y, float w, float h) {
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	ActiveRenderTarget->FillRectangle(&rectangle1, pfillColourBrush);
	return Strue;
}


ptr d2d_fill_rectangle(float x, float y, float w, float h) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckFillBrush();
	D2D1_RECT_F rectangle1 = D2D1::RectF(x, y, w, h);
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->FillRectangle(&rectangle1, pfillColourBrush);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

// reset matrix
ptr d2d_matrix_identity() {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	return Strue;
}

ptr d2d_matrix_rotate(float a, float x, float y) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y)));
	return Strue;
}

ptr d2d_matrix_translate(float x, float y) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Translation(40, 10));
	return Strue;
}

ptr d2d_matrix_rotrans(float a, float x, float y, float x1, float y1) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
	ActiveRenderTarget->SetTransform(rot * trans);
	return Strue;
}

ptr d2d_matrix_transrot(float a, float x, float y, float x1, float y1) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
	ActiveRenderTarget->SetTransform(trans * rot);
	return Strue;
}

ptr d2d_matrix_scale(float x, float y) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Scale(
			D2D1::Size(x, y),
			D2D1::Point2F(prefer_width / 2, prefer_height / 2))
	);
	return Strue;
}

ptr d2d_matrix_rotscale(float a, float x, float y, float x1, float y1) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(x, y),
		D2D1::Point2F(prefer_width / 2, prefer_height / 2));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	ActiveRenderTarget->SetTransform(rot * scale);
	return Strue;
}

ptr d2d_matrix_scalerot(float a, float x, float y, float x1, float y1) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(x, y),
		D2D1::Point2F(prefer_width / 2, prefer_height / 2));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	ActiveRenderTarget->SetTransform(rot * scale);
	return Strue;
}

ptr d2d_matrix_scalerottrans(float a, float x, float y, float x1, float y1, float x2, float y2) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(x, y),
		D2D1::Point2F(prefer_width / 2, prefer_height / 2));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x, y));
	const D2D1::Matrix3x2F trans = D2D1::Matrix3x2F::Translation(x1, y1);
	ActiveRenderTarget->SetTransform(scale * rot * trans);
	return Strue;
}

ptr d2d_matrix_skew(float x, float y) {
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Skew(
			x, y,
			D2D1::Point2F(prefer_width / 2, prefer_height / 2))
	);
	return Strue;
}

// display current display buffer.
ptr d2d_render(float x, float y) {

	swap_buffers(1);
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawBitmap(bitmap2, D2D1::RectF(x, y, prefer_width, prefer_height));
	ActiveRenderTarget->EndDraw();
	return Strue;
}

int d2d_CreateGridPatternBrush(
	ID2D1RenderTarget* pRenderTarget,
	ID2D1BitmapBrush** ppBitmapBrush
)
{
	if (pPatternBrush != nullptr) {
		return 0;
	}
	// Create a compatible render target.
	ID2D1BitmapRenderTarget* pCompatibleRenderTarget = nullptr;
	HRESULT hr = pRenderTarget->CreateCompatibleRenderTarget(
		D2D1::SizeF(10.0f, 10.0f),
		&pCompatibleRenderTarget
	);
	if (SUCCEEDED(hr))
	{
		// Draw a pattern.
		ID2D1SolidColorBrush* pGridBrush = nullptr;
		hr = pCompatibleRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF(0.93f, 0.94f, 0.96f, 1.0f)),
			&pGridBrush
		);

		// create offscreen bitmap
		d2d_CreateOffscreenBitmap();

		if (SUCCEEDED(hr))
		{
			pCompatibleRenderTarget->BeginDraw();
			pCompatibleRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, 10.0f, 1.0f), pGridBrush);
			pCompatibleRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.1f, 1.0f, 10.0f), pGridBrush);
			pCompatibleRenderTarget->EndDraw();

			// Retrieve the bitmap from the render target.
			ID2D1Bitmap* pGridBitmap = nullptr;
			hr = pCompatibleRenderTarget->GetBitmap(&pGridBitmap);
			if (SUCCEEDED(hr))
			{
				// Choose the tiling mode for the bitmap brush.
				D2D1_BITMAP_BRUSH_PROPERTIES brushProperties =
					D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP);

				// Create the bitmap brush.
				hr = pRenderTarget->CreateBitmapBrush(pGridBitmap, brushProperties, ppBitmapBrush);

				SafeRelease(&pGridBitmap);
			}

			SafeRelease(&pGridBrush);

		}

		SafeRelease(&pCompatibleRenderTarget);

	}

	return hr;
}

void d2d_make_default_stroke() {
	HRESULT r = pD2DFactory->CreateStrokeStyle(
		D2D1::StrokeStyleProperties(
			D2D1_CAP_STYLE_FLAT,
			D2D1_CAP_STYLE_FLAT,
			D2D1_CAP_STYLE_ROUND,
			D2D1_LINE_JOIN_MITER,
			1.0f,
			D2D1_DASH_STYLE_SOLID,
			0.0f),
		nullptr, 0,
		&d2d_stroke_style
	);
}


// direct write
IDWriteFactory* pDWriteFactory;
IDWriteTextFormat* TextFont;
ID2D1SolidColorBrush* pBlackBrush;


ptr d2d_text_mode(int n) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	D2D1_TEXT_ANTIALIAS_MODE mode = (D2D1_TEXT_ANTIALIAS_MODE)n;
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->SetTextAntialiasMode(mode);
	ActiveRenderTarget->EndDraw();
	return Strue;
}

ptr d2d_zwrite_text(float x, float y, char* s) {
	const auto text = Utility::widen(s);
	const auto len = text.length();
	D2D1_RECT_F layoutRect = D2D1::RectF(x, y, prefer_width - x, prefer_height - y);
	ActiveRenderTarget->DrawTextW(text.c_str(), len, TextFont, layoutRect, pfillColourBrush);
	return Strue;
}

ptr d2d_write_text(float x, float y, char* s) {

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	CheckFillBrush();

	const auto text = Utility::widen(s);
	const auto len = text.length();

	D2D1_RECT_F layoutRect = D2D1::RectF(x, y, prefer_width - x, prefer_height - y);

	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawTextW(text.c_str(), len, TextFont, layoutRect, pfillColourBrush);
	ActiveRenderTarget->EndDraw();
	return Strue;
}


ptr d2d_set_font(char* s, float size) {

	SafeRelease(&TextFont);
	auto face = Utility::widen(s);
	HRESULT
		hr = pDWriteFactory->CreateTextFormat(
			face.c_str(),
			NULL,
			DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			size,
			L"en-us",
			&TextFont
		);
	if (SUCCEEDED(hr)) {
		return Strue;
	}
	return Snil;
}

const auto bank_size = 1024;
// images bank
ID2D1Bitmap* pSpriteSheet[bank_size];

void d2d_sprite_loader(char* filename, int n)
{
	if (n > bank_size - 1) {
		return;
	}
	HRESULT hr;
	CoInitialize(NULL);
	IWICImagingFactory* wicFactory = NULL;
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory,
		(LPVOID*)&wicFactory);

	if (FAILED(hr)) {
		return;
	}
	//create a decoder
	IWICBitmapDecoder* wicDecoder = NULL;
	std::wstring fname =Utility::widen(filename);
	hr = wicFactory->CreateDecoderFromFilename(
		fname.c_str(),
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&wicDecoder);

	IWICBitmapFrameDecode* wicFrame = NULL;
	hr = wicDecoder->GetFrame(0, &wicFrame);

	// create a converter
	IWICFormatConverter* wicConverter = NULL;
	hr = wicFactory->CreateFormatConverter(&wicConverter);

	// setup the converter
	hr = wicConverter->Initialize(
		wicFrame,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		NULL,
		0.0,
		WICBitmapPaletteTypeCustom
	);
	if (SUCCEEDED(hr))
	{
		hr = pRenderTarget->CreateBitmapFromWicBitmap(
			wicConverter,
			NULL,
			&pSpriteSheet[n]
		);
	}
	SafeRelease(&wicFactory);
	SafeRelease(&wicDecoder);
	SafeRelease(&wicConverter);
	SafeRelease(&wicFrame);
}

ptr d2d_load_sprites(char* filename, int n) {
	if (n > bank_size - 1) {
		return Snil;
	}
	d2d_sprite_loader(filename, n);
	return Strue;
}


ptr d2d_zrender_sprite(int n, float dx, float dy) {

	if (n > bank_size - 1) {
		return Snil;
	}
 
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, dx + size.width, dy + size.height);
	const auto opacity = 1.0f;
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Scale(
			D2D1::Size(1.0, 1.0),
			D2D1::Point2F(size.width / 2, size.height / 2)));
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest);
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	return Strue;
}


// from sheet n; at sx, sy to dx, dy, w,h
ptr d2d_render_sprite(int n, float dx, float dy) {

	if (n > bank_size - 1) {
		return Snil;
	}
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, dx + size.width, dy + size.height);
	const auto opacity = 1.0f;
	ActiveRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Scale(
			D2D1::Size(1.0, 1.0),
			D2D1::Point2F(size.width / 2, size.height / 2)));
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest);
	ActiveRenderTarget->EndDraw();
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	return Strue;
}

ptr d2d_zrender_sprite_rotscale(int n, float dx, float dy, float a, float s) {

	if (n > bank_size - 1) {
		return Snil;
	}
 
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, dx + size.width, dy + size.height);
	const auto opacity = 1.0f;

	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(s, s),
		D2D1::Point2F(dx, dy));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(dx + (size.width / 2), dy + (size.height / 2)));
	ActiveRenderTarget->SetTransform(rot * scale);
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest);
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	return Strue;
}


ptr d2d_render_sprite_rotscale(int n, float dx, float dy, float a, float s) {

	if (n > bank_size - 1) {
		return Snil;
	}
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, dx + size.width, dy + size.height);
	const auto opacity = 1.0f;

	const auto scale = D2D1::Matrix3x2F::Scale(
		D2D1::Size(s, s),
		D2D1::Point2F(dx, dy));
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(dx + (size.width / 2), dy + (size.height / 2)));

	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->SetTransform(rot * scale);
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest);
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	ActiveRenderTarget->EndDraw();
	return Strue;
}


// from sheet n; at sx, sy to dx, dy, w,h scale up
ptr d2d_zrender_sprite_sheet(int n, float dx, float dy, float dw, float dh,
	float sx, float sy, float sw, float sh, float scale) {

	if (n > bank_size - 1) {
		return Snil;
	}

 

	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, scale * (dx + dw), scale * (dy + dh));
	const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
	const auto opacity = 1.0f;
 
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
 
	return Strue;
}


// from sheet n; at sx, sy to dx, dy, w,h scale up
ptr d2d_render_sprite_sheet(int n, float dx, float dy, float dw, float dh,
	float sx, float sy, float sw, float sh, float scale) {

	if (n > bank_size - 1) {
		return Snil;
	}

	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}

	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, scale * (dx + dw), scale * (dy + dh));
	const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
	const auto opacity = 1.0f;
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
	ActiveRenderTarget->EndDraw();
	return Strue;
}


// from sheet n; at sx, sy to dx, dy, w,h scale up
ptr d2d_zrender_sprite_sheet_rot_scale(int n, float dx, float dy, float dw, float dh,
	float sx, float sy, float sw, float sh, float scale, float a, float x2, float y2) {

	if (n > bank_size - 1) {
		return Snil;
	}
 
	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, scale * (dx + dw), scale * (dy + dh));
	const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
	const auto opacity = 1.0f;
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x2, y2));
 
	ActiveRenderTarget->SetTransform(rot);
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
 
	return Strue;
}




// from sheet n; at sx, sy to dx, dy, w,h scale up
ptr d2d_render_sprite_sheet_rot_scale(int n, float dx, float dy, float dw, float dh,
	float sx, float sy, float sw, float sh, float scale, float a, float x2, float y2) {

	if (n > bank_size - 1) {
		return Snil;
	}
	if (pRenderTarget == nullptr || ActiveRenderTarget == nullptr) {
		return Sfalse;
	}

	auto sprite_sheet = pSpriteSheet[n];
	if (sprite_sheet == NULL) {
		return Snil;
	}
	const auto size = sprite_sheet->GetPixelSize();
	const auto dest = D2D1::RectF(dx, dy, scale * (dx + dw), scale * (dy + dh));
	const auto source = D2D1::RectF(sx, sy, sx + sw, sy + sh);
	const auto opacity = 1.0f;
	const D2D1::Matrix3x2F rot = D2D1::Matrix3x2F::Rotation(a, D2D1::Point2F(x2, y2));
	ActiveRenderTarget->BeginDraw();
	ActiveRenderTarget->SetTransform(rot);
	ActiveRenderTarget->DrawBitmap(sprite_sheet, dest, 1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, source);
	ActiveRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	ActiveRenderTarget->EndDraw();
	return Strue;
}



void CreateFactory() {

	if (pD2DFactory == NULL) {
		HRESULT hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_MULTI_THREADED, &pD2DFactory);
		d2d_DPIScale(pD2DFactory);
	}
	if (pDWriteFactory == NULL) {
		HRESULT hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&pDWriteFactory)
		);
	}
}

HRESULT Create_D2D_Device_Dep(HWND h)
{
	if (IsWindow(h)) {

		if (pRenderTarget == NULL)
		{

			CreateFactory();

			RECT rc;
			GetClientRect(h, &rc);

			D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

			HRESULT hr = pD2DFactory->CreateHwndRenderTarget(
				D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(h,
					D2D1::SizeU((UINT32)rc.right, (UINT32)rc.bottom)),
				&pRenderTarget);

			if (FAILED(hr)) {
		
				return hr;
			}

			d2d_CreateGridPatternBrush(pRenderTarget, &pPatternBrush);

			hr = pDWriteFactory->CreateTextFormat(
				L"Consolas",
				NULL,
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				32.0f,
				L"en-us",
				&TextFont
			);

			if (FAILED(hr)) {
		
				return hr;
			}

			hr = pRenderTarget->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Black),
				&pBlackBrush
			);

			if (FAILED(hr)) {
	
				return hr;
			}

			return hr;
		}
	}
	else {
	
	}
	return 0;
}


void safe_release() {

	SafeRelease(&pRenderTarget);
	SafeRelease(&pPatternBrush);
	SafeRelease(&pBlackBrush);
	SafeRelease(&pD2DFactory);
	SafeRelease(&TextFont);
	SafeRelease(&BitmapRenderTarget);
	SafeRelease(&BitmapRenderTarget2);
 
}


ptr d2d_image_size(int w, int h)
{
	prefer_width = w;
	prefer_height = h;

	safe_release();

	Create_D2D_Device_Dep(main_window);
	while (pRenderTarget == nullptr) {
		Sleep(10);
	}
	d2d_fill_rectangle(0.0, 0.0, 1.0 * w, 1.0 * h);

	return Strue;
}


ptr d2d_release() {
	safe_release();
	return Strue;
}


void onPaint(HWND hWnd) {

	WaitForSingleObject(g_image_rotation_mutex, INFINITE);

	if (main_window == nullptr) {
		main_window = hWnd;
	}

	if (main_window != hWnd) {
		main_window = hWnd;
		safe_release();
	}

	RECT rc;
	::GetClientRect(hWnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
	HRESULT

		hr = Create_D2D_Device_Dep(hWnd);
	if (SUCCEEDED(hr))
	{

		pRenderTarget->Resize(size);
		pRenderTarget->BeginDraw();
		D2D1_SIZE_F renderTargetSize = pRenderTarget->GetSize();
		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::LightGray));

		pRenderTarget->FillRectangle(
			D2D1::RectF(0.0f, 0.0f, renderTargetSize.width, renderTargetSize.height), pPatternBrush);

		if (bitmap2 != nullptr) {
			pRenderTarget->DrawBitmap(bitmap2, D2D1::RectF(0.0f, 0.0f, prefer_width, prefer_height));
		}
		hr = pRenderTarget->EndDraw();

		if (FAILED(hr)) {

			safe_release();
		}

	}

	if (hr == D2DERR_RECREATE_TARGET)
	{
		safe_release();
	}
	if (FAILED(hr)) {
		safe_release();
	}
	else
	{
		::ValidateRect(hWnd, NULL);
	}

	ReleaseMutex(g_image_rotation_mutex);
}

void step(ptr lpParam) {

	WaitForSingleObject(g_image_rotation_mutex, INFINITE);

	if (Sprocedurep(lpParam)) {
		Scall0(lpParam);
	}

	ReleaseMutex(g_image_rotation_mutex);
}

ptr step_func(ptr lpParam) {

	WaitForSingleObject(g_image_rotation_mutex, INFINITE);
	if (Sprocedurep(lpParam)) {
		Scall0(lpParam);
	}
	ReleaseMutex(g_image_rotation_mutex);
	if(main_window!=nullptr)
		PostMessageW(main_window, WM_USER + 501, (WPARAM)0, (LPARAM)0);
	return Strue;
}




// constructor
CD2DView::CD2DView()
{
}

// destructor
CD2DView::~CD2DView()
{
	safe_release();
}

// Create the resources used by OnRender
HRESULT CD2DView::CreateDeviceResources()
{
    HRESULT hr = S_OK;
    return hr;
}

void CD2DView::DiscardDeviceResources()
{
	safe_release();
}

HRESULT CD2DView::OnRender()
{
    HRESULT hr = S_OK;
    onPaint(GetHwnd());
    return 0;
}


void CD2DView::OnResize(UINT width, UINT height)
{
  
}

void CD2DView::Stop()
{	
	safe_release();
	ReleaseMutex(g_image_rotation_mutex);
}

void CD2DView::Step(ptr n)
{
	step(n);
}

void CD2DView::Swap(int n)
{
	swap_buffers(n);
}


void CD2DView::PreCreate(CREATESTRUCT& cs)
{
    cs.cx = 640;
    cs.cy = 480;
}

void CD2DView::PreRegisterClass(WNDCLASS& wc)
{
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor = ::LoadCursor(NULL, IDI_APPLICATION);
    wc.lpszClassName = _T("Direct2D");
}

int CD2DView::OnCreate(CREATESTRUCT& cs)
{
    UNREFERENCED_PARAMETER(cs);

    // Set the window's icon
    SetIconSmall(IDW_MAIN);
    SetIconLarge(IDW_MAIN);
    return 0;
}

void CD2DView::OnDestroy()
{
	safe_release();
  
}

LRESULT CD2DView::OnPaint(UINT, WPARAM, LPARAM)
{
    if (GetHwnd() != nullptr) {
        OnRender();
        ValidateRect();
    }
    return 0;
}

LRESULT CD2DView::OnSize(UINT, WPARAM, LPARAM lparam)
{
    UINT width = LOWORD(lparam);
    UINT height = HIWORD(lparam);
    OnResize(width, height);

    return 0;
}

LRESULT CD2DView::OnDisplayChange(UINT, WPARAM, LPARAM)
{
    Invalidate();
    return 0;
}


struct kp {
    int when;
    boolean left;
    boolean right;
    boolean up;
    boolean down;
    boolean ctrl;
    boolean space;
    int key_code;
} graphics_keypressed;

ptr cons_sbool(const char* symbol, bool value, ptr l)
{
    ptr a = Snil;
    if (value) {
        a = CALL2("cons", Sstring_to_symbol(symbol), Strue);
    }
    else
    {
        a = CALL2("cons", Sstring_to_symbol(symbol), Sfalse);
    }
    l = CALL2("cons", a, l);
    return l;
}
ptr cons_sfixnum(const char* symbol, const int value, ptr l)
{
    ptr a = Snil;
    a = CALL2("cons", Sstring_to_symbol(symbol), Sfixnum(value));
    l = CALL2("cons", a, l);
    return l;
}

ptr graphics_keys(void) {
	ptr a = Snil;
	a = cons_sbool("left", graphics_keypressed.left, a);
	a = cons_sbool("right", graphics_keypressed.right, a);
	a = cons_sbool("up", graphics_keypressed.up, a);
	a = cons_sbool("down", graphics_keypressed.down, a);
	a = cons_sbool("ctrl", graphics_keypressed.ctrl, a);
	a = cons_sbool("space", graphics_keypressed.space, a);
	a = cons_sfixnum("key", graphics_keypressed.key_code, a);
	a = cons_sfixnum("recent", GetTickCount() - graphics_keypressed.when, a);
	return a;
}

LRESULT CD2DView::WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
{

    switch (msg)
    {
    case WM_KEYDOWN:
        graphics_keypressed.ctrl = false;
        graphics_keypressed.left = false;
        graphics_keypressed.right = false;
        graphics_keypressed.down = false;
        graphics_keypressed.up = false;
        graphics_keypressed.space = false;
        graphics_keypressed.key_code = wparam;
        graphics_keypressed.when = GetTickCount();
        switch (wparam) {

        case VK_CONTROL:
            graphics_keypressed.ctrl = true;
            break;
        case VK_LEFT:
            graphics_keypressed.left = true;
            break;
        case VK_RIGHT:
            graphics_keypressed.right = true;
            break;
        case VK_UP:
            graphics_keypressed.up = true;
            break;
        case VK_DOWN:
            graphics_keypressed.down = true;
            break;
        case VK_SPACE:
            graphics_keypressed.space = true;
            break;

        }
        break;

    case WM_DISPLAYCHANGE: 
		return OnDisplayChange(msg, wparam, lparam);
    case WM_SIZE:           return OnSize(msg, wparam, lparam);
    }


    return WndProcDefault(msg, wparam, lparam);
}



void add_d2d_commands() {

	Sforeign_symbol("d2d_matrix_identity", static_cast<ptr>(d2d_matrix_identity));
	Sforeign_symbol("d2d_matrix_rotate", static_cast<ptr>(d2d_matrix_rotate));
	Sforeign_symbol("d2d_render_sprite", static_cast<ptr>(d2d_render_sprite));
	Sforeign_symbol("d2d_render_sprite_rotscale", static_cast<ptr>(d2d_render_sprite_rotscale));
	Sforeign_symbol("d2d_render_sprite_sheet", static_cast<ptr>(d2d_render_sprite_sheet));
	Sforeign_symbol("d2d_render_sprite_sheet_rot_scale", static_cast<ptr>(d2d_render_sprite_sheet_rot_scale));
	Sforeign_symbol("d2d_load_sprites", static_cast<ptr>(d2d_load_sprites));
	Sforeign_symbol("d2d_write_text", static_cast<ptr>(d2d_write_text));
	Sforeign_symbol("d2d_zwrite_text", static_cast<ptr>(d2d_zwrite_text));
	Sforeign_symbol("d2d_text_mode", static_cast<ptr>(d2d_text_mode));
	Sforeign_symbol("d2d_set_font", static_cast<ptr>(d2d_set_font));
	Sforeign_symbol("d2d_color", static_cast<ptr>(d2d_color));
	Sforeign_symbol("d2d_fill_color", static_cast<ptr>(d2d_fill_color));
	Sforeign_symbol("d2d_rectangle", static_cast<ptr>(d2d_rectangle));
	Sforeign_symbol("d2d_zrectangle", static_cast<ptr>(d2d_zrectangle));
	Sforeign_symbol("d2d_fill_rectangle", static_cast<ptr>(d2d_fill_rectangle));
	Sforeign_symbol("d2d_zfill_rectangle", static_cast<ptr>(d2d_zfill_rectangle));
	Sforeign_symbol("d2d_ellipse", static_cast<ptr>(d2d_ellipse));
	Sforeign_symbol("d2d_zellipse", static_cast<ptr>(d2d_zellipse));
	Sforeign_symbol("d2d_fill_ellipse", static_cast<ptr>(d2d_fill_ellipse));
	Sforeign_symbol("d2d_zfill_ellipse", static_cast<ptr>(d2d_zfill_ellipse));
	Sforeign_symbol("d2d_line", static_cast<ptr>(d2d_line));
	Sforeign_symbol("d2d_zline", static_cast<ptr>(d2d_zline));
	Sforeign_symbol("d2d_render", static_cast<ptr>(d2d_render));
	Sforeign_symbol("d2d_show", static_cast<ptr>(d2d_show));
	Sforeign_symbol("d2d_save", static_cast<ptr>(d2d_save)); 
	Sforeign_symbol("d2d_clear", static_cast<ptr>(d2d_clear));
	Sforeign_symbol("d2d_set_stroke_width", static_cast<ptr>(d2d_set_stroke_width));
	Sforeign_symbol("d2d_image_size", static_cast<ptr>(d2d_image_size));
	Sforeign_symbol("d2d_release", static_cast<ptr>(d2d_release));
	Sforeign_symbol("d2d_draw_func", static_cast<ptr>(d2d_draw_func));
	Sforeign_symbol("step", static_cast<ptr>(step_func));
	Sforeign_symbol("graphics_keys", static_cast<ptr>(graphics_keys));
}

void CD2DView::AddCommands()
{
	add_d2d_commands();
}
