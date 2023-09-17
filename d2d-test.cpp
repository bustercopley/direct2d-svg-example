#define _WIN32_WINNT _WIN32_WINNT_WIN10
#define NTDDI_VERSION NTDDI_WIN10_RS2
#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <windows.h>

#include <cstdio>

#include <comdef.h>
#include <d2d1_1.h>
#include <wincodec.h>

#ifndef NO_SVG
#include <d2d1_3.h>
#include <shlwapi.h>
#endif

#define COM_SMARTPTR_TYPEDEF(a) _COM_SMARTPTR_TYPEDEF(a, __uuidof(a))
#define CHK(hr) check_hresult(hr, __FILE__, __LINE__)

inline void check_hresult(HRESULT hr, const char *file, int line) {
  if (!SUCCEEDED(hr)) {
    std::fprintf(
      stdout, "%s:%d: bad HRESULT 0x%08x\n", file, line, (unsigned)hr);
    std::fflush(stdout);
    throw _com_error(hr);
  }
}

COM_SMARTPTR_TYPEDEF(IWICImagingFactory);
COM_SMARTPTR_TYPEDEF(IWICBitmap);
COM_SMARTPTR_TYPEDEF(IWICStream);
COM_SMARTPTR_TYPEDEF(IWICBitmapEncoder);
COM_SMARTPTR_TYPEDEF(IWICBitmapFrameEncode);
COM_SMARTPTR_TYPEDEF(ID2D1Brush);
COM_SMARTPTR_TYPEDEF(ID2D1RenderTarget);
COM_SMARTPTR_TYPEDEF(ID2D1Factory1);
COM_SMARTPTR_TYPEDEF(ID2D1SolidColorBrush);

#ifndef NO_SVG
COM_SMARTPTR_TYPEDEF(IStream);
COM_SMARTPTR_TYPEDEF(ID2D1DeviceContext5);
COM_SMARTPTR_TYPEDEF(ID2D1SvgDocument);
#endif

void run_test(const wchar_t *output_filename, const wchar_t *svg_filename) {

#ifdef NO_SVG
  (void)svg_filename; // Silence warning about unused parameter.
#endif

  // Create a WIC (Windows Imaging Component) factory.
  IWICImagingFactoryPtr WICImagingFactory;
  CHK(WICImagingFactory.CreateInstance(
    CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER));

  // Create a WIC bitmap.
  IWICBitmapPtr WICBitmap;
  CHK(WICImagingFactory->CreateBitmap(
    320, 256, GUID_WICPixelFormat32bppBGR, WICBitmapCacheOnDemand, &WICBitmap));

  // Create a Direct2D factory.
  ID2D1Factory1Ptr D2D1Factory;
  D2D1_FACTORY_OPTIONS options = {D2D1_DEBUG_LEVEL_INFORMATION};
  CHK(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED,
    __uuidof(ID2D1Factory1), &options, (void **)&D2D1Factory));

  // Create a Direct2D render target for rendering to the WIC bitmap.
  ID2D1RenderTargetPtr D2D1RenderTarget;
  D2D1_RENDER_TARGET_PROPERTIES props =
    D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
      D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 0,
      0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);
  CHK(D2D1Factory->CreateWicBitmapRenderTarget(
    WICBitmap, &props, &D2D1RenderTarget));

  // Define colors.
  D2D1_COLOR_F purple{0.459f, 0.227f, 0.533f, 1.0f};
  D2D1_COLOR_F white{1.0f, 1.0f, 1.0f, 1.0f};

  // Create a brush.
  ID2D1SolidColorBrushPtr D2D1Brush;
  CHK(D2D1RenderTarget->CreateSolidColorBrush(white, &D2D1Brush));

  // Draw a picture.
  D2D1RenderTarget->BeginDraw();
  D2D1RenderTarget->Clear(purple);
  D2D1RenderTarget->DrawLine({10, 10}, {310, 246}, D2D1Brush, 10);

#ifndef NO_SVG
  if (svg_filename) {
    // Get an ID2D1Context5.
    ID2D1DeviceContext5Ptr D2D1Context5;
    CHK(D2D1RenderTarget->QueryInterface(&D2D1Context5));

    // Open SVG file and create stream.
    IStreamPtr Stream;
    CHK(SHCreateStreamOnFileW(
      svg_filename, STGM_READ | STGM_SHARE_DENY_WRITE, &Stream));

    // Load the SVG document from the stream.
    ID2D1SvgDocumentPtr SvgDocument;
    CHK(D2D1Context5->CreateSvgDocument(Stream, {320, 256}, &SvgDocument));

    // Draw the SVG image.
    D2D1Context5->DrawSvgDocument(SvgDocument);
  }
#endif

  CHK(D2D1RenderTarget->EndDraw());

  // Create a WIC stream.
  IWICStreamPtr WICStream;
  CHK(WICImagingFactory->CreateStream(&WICStream));
  CHK(WICStream->InitializeFromFilename(output_filename, GENERIC_WRITE));

  // Create a WIC bitmap encoder.
  IWICBitmapEncoderPtr WICEncoder;
  CHK(WICImagingFactory->CreateEncoder(
    GUID_ContainerFormatPng, nullptr, &WICEncoder));
  CHK(WICEncoder->Initialize(WICStream, WICBitmapEncoderNoCache));

  // Create a ... frame thing.
  IWICBitmapFrameEncodePtr WICBitmapFrameEncode;
  CHK(WICEncoder->CreateNewFrame(&WICBitmapFrameEncode, nullptr));

  // Save the WIC bitmap to PNG.
  CHK(WICBitmapFrameEncode->Initialize(nullptr));
  CHK(WICBitmapFrameEncode->WriteSource(WICBitmap, nullptr));
  CHK(WICBitmapFrameEncode->Commit());
  CHK(WICEncoder->Commit());
}

int wmain(int argc, const wchar_t *argv[]) {
  if (argc != 2 && argc != 3) {
    return 1;
  }

  const wchar_t *output_filename = argv[1];
  const wchar_t *svg_filename = argc < 3 ? nullptr : argv[2];

  try {
    CHK(CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE));
    run_test(output_filename, svg_filename);
  } catch (...) {
    return 1;
  }
  CoUninitialize();

  return 0;
}
