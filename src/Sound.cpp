
#include "stdafx.h"
#include "Sound.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>
#include <shlwapi.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1effects.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <dwrite.h>
#include <wincodec.h>
#include <string>
#include <scheme.h>
#include <deque>
#include <exception>
#include <stdexcept>
#include <vector>
#include <wincodec.h>	 
#include <WTypes.h>
#include <Mmsystem.h>
#include <Windows.Foundation.h>
#include <wrl\wrappers\corewrappers.h>
#include <wrl\client.h>

#include <functional>
#include <chrono>
#include <future>


#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid")
#pragma comment (lib, "Windowscodecs.lib")
#pragma comment (lib, "runtimeobject.lib")

#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

#include <scheme.h>
#include "Utility.h"
#define CALL0(who) Scall0(Stop_level_value(Sstring_to_symbol(who)))
#define CALL1(who, arg) Scall1(Stop_level_value(Sstring_to_symbol(who)), arg)
#define CALL2(who, arg, arg2) Scall2(Stop_level_value(Sstring_to_symbol(who)), arg, arg2)



using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

template <class callable, class... arguments>
void later(int after, bool async, callable f, arguments&&... args) {
	std::function<typename std::result_of < callable(arguments...)>::type() > task(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

	if (async) {
		std::thread([after, task]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(after));
			task();
			}).detach();
	}
	else {
		std::this_thread::sleep_for(std::chrono::milliseconds(after));
		task();
	}
}

#define bank_size 512
// set up audio
bool audio_available = false;
ComPtr<IMFAttributes> sourceReaderConfiguration;
IXAudio2* pXAudio2 = NULL;
IXAudio2MasteringVoice* masterVoice;

// sound sample banks
struct sound {
	bool loaded = false;
	int AudioBytes = 0;
	int audioDataLength = 0;
	WAVEFORMATEX* waveFormat = {};
	unsigned int waveLength = 0;
	std::vector<BYTE> audioData = {};
	XAUDIO2_BUFFER audioBuffer = {};
} sounds[bank_size];

void init_audio() {
	HRESULT hr;
	hr = MFStartup(MF_VERSION);
	if (FAILED(hr))  return;
	hr = MFCreateAttributes(sourceReaderConfiguration.GetAddressOf(), 1);
	if (FAILED(hr)) return;
	hr = sourceReaderConfiguration->SetUINT32(MF_LOW_LATENCY, true);
	if (FAILED(hr)) return;
	hr = XAudio2Create(&pXAudio2, 0, 1);
	if (FAILED(hr)) return;
	hr = pXAudio2->CreateMasteringVoice(&masterVoice);
	if (FAILED(hr)) return;
	audio_available = true;
}

int load_sound_data_file(const std::wstring& filename, int n)
{
	if (n > bank_size - 1) return 0;
	DWORD streamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;
	HRESULT hr;
	ComPtr<IMFSourceReader> sourceReader;
	ComPtr<IMFMediaType> nativeMediaType;

	hr = MFCreateSourceReaderFromURL(filename.c_str(), sourceReaderConfiguration.Get(), sourceReader.GetAddressOf());
	if (FAILED(hr)) return 0;
	hr = sourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, false);
	if (FAILED(hr)) return 0;
	hr = sourceReader->SetStreamSelection(streamIndex, true);

	ComPtr<IMFMediaType> partialType = nullptr;
	hr = MFCreateMediaType(partialType.GetAddressOf());
	if (FAILED(hr)) return 0;
	hr = partialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	if (FAILED(hr)) return 0;
	hr = partialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	if (FAILED(hr)) return 0;
	hr = sourceReader->SetCurrentMediaType(streamIndex, NULL, partialType.Get());
	if (FAILED(hr)) return 0;

	// what sound is not compressed??
	ComPtr<IMFMediaType> uncompressedAudioType = nullptr;
	hr = sourceReader->GetCurrentMediaType(streamIndex, uncompressedAudioType.GetAddressOf());
	if (FAILED(hr)) return 0;
	hr = MFCreateWaveFormatExFromMFMediaType(uncompressedAudioType.Get(), &sounds[n].waveFormat, &sounds[n].waveLength);
	if (FAILED(hr)) return 0;
	hr = sourceReader->SetStreamSelection(streamIndex, true);
	if (FAILED(hr)) return 0;
	ComPtr<IMFSample> sample = nullptr;
	ComPtr<IMFMediaBuffer> buffer = nullptr;

	while (true)
	{
		DWORD flags = 0;
		hr = sourceReader->ReadSample(streamIndex, 0, nullptr, &flags, nullptr, sample.GetAddressOf());
		if (FAILED(hr)) return 0;
		if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
			break;
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
			break;

		if (sample == nullptr)
			continue;
		hr = sample->ConvertToContiguousBuffer(buffer.GetAddressOf());
		if (FAILED(hr)) return 0;

		BYTE* AudioData = NULL;
		DWORD AudioDataLength = 0;

		hr = buffer->Lock(&AudioData, nullptr, &AudioDataLength);
		if (FAILED(hr)) return 0;
		for (size_t i = 0; i < AudioDataLength; i++)
			sounds[n].audioData.push_back(AudioData[i]);

		hr = buffer->Unlock();
		AudioData = nullptr;
		sounds[n].loaded = true;
		sounds[n].audioBuffer.AudioBytes = sounds[n].audioData.size();
		sounds[n].audioBuffer.pAudioData = &sounds[n].audioData[0];
		sounds[n].audioBuffer.pContext = nullptr;

	}
	return -1;
}

int load_sound_file(const std::wstring fileName, int n)
{
	HRESULT hr = S_OK;
	auto result = load_sound_data_file(fileName, n);
	return result;
}

ptr load_sound(char* s, int n) {
	if (n > bank_size - 1) return Sfalse;
	auto ws = Utility::widen(s).c_str();
	load_sound_file(ws, n);
	return Strue;
}

ptr play_sound(int n) {

	if (n > bank_size - 1) return Sfalse;
	IXAudio2SourceVoice* sourceVoice;
	HRESULT hr = pXAudio2->CreateSourceVoice(&sourceVoice, sounds[n].waveFormat);
	sourceVoice->SubmitSourceBuffer(&sounds[n].audioBuffer);
	sourceVoice->Start();

	later(10000, true, [=]() {
		float pVolume;
		sourceVoice->GetVolume(&pVolume);

		while (pVolume > 0.0f) {
			pVolume -= 0.001f;
			Sleep(1);
			sourceVoice->SetVolume(pVolume);
		}
		sourceVoice->DestroyVoice(); });
	return Strue;
}


void Sound::Start()
{
	Sforeign_symbol("play_sound", static_cast<ptr>(play_sound));
	Sforeign_symbol("load_sound", static_cast<ptr>(load_sound));
}	
