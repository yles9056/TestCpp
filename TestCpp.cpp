// TestCpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <dshow.h>
#include <cassert>

//using namespace std;

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

#pragma comment(lib, "strmiids.lib")
HRESULT enumerateCategory(IID clsidDeviceClass)
{
    HRESULT hr;

    // Create the System Device Enumerator.
    ICreateDevEnum* pSysDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pSysDevEnum);
    if (FAILED(hr))
    {
        std::cout << "CoCreateInstance CLSID_SystemDeviceEnum failed!";
        return hr;
    }

    // Obtain a class enumerator for the video compressor category.
    IEnumMoniker* pEnumCat = NULL;
    hr = pSysDevEnum->CreateClassEnumerator(clsidDeviceClass, &pEnumCat, 0);

    if (hr == S_OK)
    {
        // Enumerate the monikers.
        IMoniker* pMoniker = NULL;
        ULONG cFetched;
        while (pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
        {
            IPropertyBag* pPropBag;
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
            if (SUCCEEDED(hr))
            {
                // To retrieve the filter's friendly name, do the following:
                VARIANT varName;
                VariantInit(&varName);
                hr = pPropBag->Read(L"FriendlyName", &varName, 0);
                if (SUCCEEDED(hr))
                {
                    // Display the name in your UI somehow.
                    std::wstring ws(varName.bstrVal, SysStringLen(varName.bstrVal));
                    std::wcout << ws << "\n";
                }
                VariantClear(&varName);

                // To create an instance of the filter, do the following:
                IBaseFilter* pFilter = NULL;
                hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pFilter);
                // Now add the filter to the graph. 
                //Remember to release pFilter later.
                SafeRelease(&pFilter);
                pPropBag->Release();
            }
            pMoniker->Release();
        }
        pEnumCat->Release();
    }
    pSysDevEnum->Release();

    return 0;
}

// Create a filter by CLSID and add it to the graph.
HRESULT AddFilterByCLSID(
    IGraphBuilder* pGraph,      // Pointer to the Filter Graph Manager.
    REFGUID clsid,              // CLSID of the filter to create.
    IBaseFilter** ppF,          // Receives a pointer to the filter.
    LPCWSTR wszName             // A name for the filter (can be NULL).
)
{
    *ppF = 0;

    IBaseFilter* pFilter = NULL;

    HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFilter));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pGraph->AddFilter(pFilter, wszName);
    if (FAILED(hr))
    {
        goto done;
    }

    *ppF = pFilter;
    (*ppF)->AddRef();

done:
    SafeRelease(&pFilter);
    return hr;
}

HRESULT createGraph(IGraphBuilder** ppG) {
    IGraphBuilder* pGraph = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);
    if (FAILED(hr))
    {
        std::cout << "CoCreateInstance CLSID_FilterGraph failed!";
    }
    else
    {
        *ppG = pGraph;
        (*ppG)->AddRef();
    }

    SafeRelease(&pGraph);
    return hr;
}

HRESULT createMediaControl(IGraphBuilder* pGraph, IMediaControl** ppC) {
    IMediaControl* pControl = NULL;
    HRESULT hr = pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
    if (FAILED(hr))
    {
        std::cout << "QueryInterface IID_IMediaControl failed!";
    }
    else
    {
        *ppC = pControl;
        (*ppC)->AddRef();
    }

    SafeRelease(&pControl);
    return hr;
}

HRESULT createMediaEvent(IGraphBuilder* pGraph, IMediaEvent** ppE) {
    IMediaEvent* pEvent = NULL;
    HRESULT hr = pGraph->QueryInterface(IID_IMediaEvent, (void**)&pEvent);
    if (FAILED(hr))
    {
        std::cout << "QueryInterface IID_IMediaEvent failed!";
    }
    else
    {
        *ppE = pEvent;
        (*ppE)->AddRef();
    }

    SafeRelease(&pEvent);
    return hr;
}

HRESULT getFilterByName(IID clsidDeviceClass, std::wstring targetName, IBaseFilter** ppF)
{
    HRESULT hr = 0;

    // Create the System Device Enumerator.
    ICreateDevEnum* pSysDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pSysDevEnum);
    if (FAILED(hr))
    {
        std::cout << "CoCreateInstance CLSID_SystemDeviceEnum failed!";
        return hr;
    }

    // Obtain a class enumerator for the video compressor category.
    IEnumMoniker* pEnumCat = NULL;
    hr = pSysDevEnum->CreateClassEnumerator(clsidDeviceClass, &pEnumCat, 0);

    if (hr == S_OK)
    {
        // Enumerate the monikers.
        IMoniker* pMoniker = NULL;
        ULONG cFetched;
        bool foundFilter = false;
        while (!foundFilter && pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
        {
            IPropertyBag* pPropBag = NULL;
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
            if (SUCCEEDED(hr))
            {
                // To retrieve the filter's friendly name, do the following:
                VARIANT varName;
                VariantInit(&varName);
                hr = pPropBag->Read(L"FriendlyName", &varName, 0);
                if (SUCCEEDED(hr))
                {
                    // Display the name in your UI somehow.
                    std::wstring ws(varName.bstrVal, SysStringLen(varName.bstrVal));
                    std::wcout << ws << "\n";

                    std::size_t foundContainString = ws.find(targetName);
                    std::size_t foundContainStringDS = ws.find(L"DirectSound:");
                    if (foundContainString != std::wstring::npos && foundContainStringDS == std::wstring::npos)
                    {
                        std::cout << "^(Found target filter)\n";
                        foundFilter = true;

                        // To create an instance of the filter, do the following:
                        IBaseFilter* pFilter = NULL;
                        hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pFilter);

                        if (SUCCEEDED(hr))
                        {
                            *ppF = pFilter;
                            (*ppF)->AddRef();
                        }
                        SafeRelease(&pFilter);
                    }
                }
                VariantClear(&varName);

                pPropBag->Release();
            }
            pMoniker->Release();
        }
        pEnumCat->Release();
    }
    pSysDevEnum->Release();

    return 0;
}

// Query whether a pin is connected to another pin.
//
// Note: This function does not return a pointer to the connected pin.
HRESULT IsPinConnected(IPin* pPin, BOOL* pResult)
{
    IPin* pTmp = NULL;
    HRESULT hr = pPin->ConnectedTo(&pTmp);
    if (SUCCEEDED(hr))
    {
        *pResult = TRUE;
    }
    else if (hr == VFW_E_NOT_CONNECTED)
    {
        // The pin is not connected. This is not an error for our purposes.
        *pResult = FALSE;
        hr = S_OK;
    }

    SafeRelease(&pTmp);
    return hr;
}

// Query whether a pin has a specified direction (input / output)
HRESULT IsPinDirection(IPin* pPin, PIN_DIRECTION dir, BOOL* pResult)
{
    PIN_DIRECTION pinDir;
    HRESULT hr = pPin->QueryDirection(&pinDir);
    if (SUCCEEDED(hr))
    {
        *pResult = (pinDir == dir);
    }
    return hr;
}

// Match a pin by pin direction and connection state.
HRESULT MatchPin(IPin* pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL* pResult)
{
    assert(pResult != NULL);

    BOOL bMatch = FALSE;
    BOOL bIsConnected = FALSE;

    HRESULT hr = IsPinConnected(pPin, &bIsConnected);
    if (SUCCEEDED(hr))
    {
        if (bIsConnected == bShouldBeConnected)
        {
            hr = IsPinDirection(pPin, direction, &bMatch);
        }
    }

    if (SUCCEEDED(hr))
    {
        *pResult = bMatch;
    }
    return hr;
}

// Return the first unconnected input pin or output pin.
HRESULT FindUnconnectedPin(IBaseFilter* pFilter, PIN_DIRECTION PinDir, IPin** ppPin)
{
    IEnumPins* pEnum = NULL;
    IPin* pPin = NULL;
    BOOL bFound = FALSE;

    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        goto done;
    }

    while (S_OK == pEnum->Next(1, &pPin, NULL))
    {
        hr = MatchPin(pPin, PinDir, FALSE, &bFound);
        if (FAILED(hr))
        {
            goto done;
        }
        if (bFound)
        {
            *ppPin = pPin;
            (*ppPin)->AddRef();
            break;
        }
        SafeRelease(&pPin);
    }

    if (!bFound)
    {
        hr = VFW_E_NOT_FOUND;
    }

done:
    SafeRelease(&pPin);
    SafeRelease(&pEnum);
    return hr;
}

int main()
{
    HRESULT hr;

    // Init graph
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        std::cout << "CoInitialize failed!";
        return hr;
    }

    /*std::cout << "[CLSID_AudioInputDeviceCategory]:\n";
    enumerateCategory(CLSID_AudioInputDeviceCategory);
    std::cout << "\n";
    std::cout << "[CLSID_AudioRendererCategory]:\n";
    enumerateCategory(CLSID_AudioRendererCategory);*/

    // Create graph
    IGraphBuilder* pGraph = NULL;
    hr = createGraph(&pGraph);
    if (FAILED(hr))
    {
        std::cout << "createGraph failed!";
        return hr;
    }

    // Create media control
    IMediaControl* pControl = NULL;
    hr = createMediaControl(pGraph, &pControl);
    if (FAILED(hr))
    {
        std::cout << "createMediaControl failed!";
        return hr;
    }

    // Create media event
    IMediaEvent* pEvent = NULL;
    hr = createMediaEvent(pGraph, &pEvent);
    if (FAILED(hr))
    {
        std::cout << "createMediaEvent failed!";
        return hr;
    }

    // Get audio capture source filter by name
    IBaseFilter* pAudioInputFilter = NULL;
    hr = getFilterByName(CLSID_AudioInputDeviceCategory, L"OBS-Audio", &pAudioInputFilter);
    if (FAILED(hr) || pAudioInputFilter == NULL)
    {
        std::cout << "getFilterByName CLSID_AudioInputDeviceCategory failed!";
        return hr;
    }

    // Add audio capture source filter to the graph
    hr = pGraph->AddFilter(pAudioInputFilter, L"AudioInputFilter");
    if (FAILED(hr))
    {
        std::cout << "AddFilter pAudioInputFilter failed!";
        return hr;
    }

    std::cout << "\n";

    // Get audio renderer filter by name
    IBaseFilter* pAudioRendererFilter = NULL;
    hr = getFilterByName(CLSID_AudioRendererCategory, L"AudioMirror Virtual Device", &pAudioRendererFilter);
    if (FAILED(hr) || pAudioRendererFilter == NULL)
    {
        std::cout << "getFilterByName CLSID_AudioRendererCategory failed!";
        return hr;
    }

    // Add audio renderer filter to the graph
    hr = pGraph->AddFilter(pAudioRendererFilter, L"AudioRendererFilter");
    if (FAILED(hr))
    {
        std::cout << "AddFilter pAudioRendererFilter failed!";
        return hr;
    }

    std::cout << "\n";

    // Get output pin of audio capture source filter
    IPin* pOut = NULL;
    hr = FindUnconnectedPin(pAudioInputFilter, PINDIR_OUTPUT, &pOut);
    if (FAILED(hr))
    {
        std::cout << "FindUnconnectedPin pAudioInputFilter PINDIR_OUTPUT failed!";
        return hr;
    }

    // Get input pin of audio renderer filter
    IPin* pIn = NULL;
    hr = FindUnconnectedPin(pAudioRendererFilter, PINDIR_INPUT, &pIn);
    if (FAILED(hr))
    {
        std::cout << "FindUnconnectedPin pAudioRendererFilter PINDIR_INPUT failed!";
        return hr;
    }

    // Connect output and input pins
    hr = pGraph->Connect(pOut, pIn);
    if (FAILED(hr))
    {
        std::cout << "Connect failed!";
        return hr;
    }
    else
    {
        std::cout << "Connection done\n";
    }

    // Run the graph
    std::cout << "Running the graph...\n";
    hr = pControl->Run();
    long evCode = 0;
    pEvent->WaitForCompletion(INFINITE, &evCode);
    std::cout << "Ending the graph...\n";

    // Release everything
    SafeRelease(&pOut);
    SafeRelease(&pIn);
    SafeRelease(&pAudioInputFilter);
    SafeRelease(&pAudioRendererFilter);
    SafeRelease(&pControl);
    SafeRelease(&pEvent);
    SafeRelease(&pGraph);
    CoUninitialize();
}
