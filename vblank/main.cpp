#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <vector>

#include <dxgi.h>
#include <Dwmapi.h>

#define ITERATIONS 500
#define OUTPUT_FILE 1

bool test(FILE* output, bool useDwmFlush, IDXGIOutput* monitor) {
	LARGE_INTEGER frequency;
	if (!QueryPerformanceFrequency(&frequency)) {
		return false;
	}

	LARGE_INTEGER start;
	if (FAILED(QueryPerformanceCounter(&start))) {
		return false;
	}

	fprintf(output, "QPCFreq,%lld\n", frequency.QuadPart);
	fprintf(output, "QPCStart,%lld\n", start.QuadPart);

	for (int i = 0; i < ITERATIONS; i++) {
		if (useDwmFlush) {
			DwmFlush();
		} else if (FAILED(monitor->WaitForVBlank())) {
			return false;
		}

		LARGE_INTEGER qpcVBlank;
		if (FAILED(QueryPerformanceCounter(&qpcVBlank))) {
			return false;
		}

		DWM_TIMING_INFO timingInfo;
		memset(&timingInfo, 0, sizeof(DWM_TIMING_INFO));
		timingInfo.cbSize = sizeof(DWM_TIMING_INFO);
		DwmGetCompositionTimingInfo(0, &timingInfo);

		fprintf(output, "QPC,%lld,DwmGetTimingInfo,%llu\n",
			qpcVBlank.QuadPart,
			timingInfo.qpcVBlank);
	}

	fclose(output);
	return true;
}

int main(int argc, char** argv) {
	IDXGIFactory* factory = nullptr;
	if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory))) {
		fprintf(stderr, "Failed to create DXGIFactory.\n");
		return 1;
	}

	// Gather all the adapters and outputs
	std::vector<IDXGIOutput*> outputs;
	int index = 0;

	for (int i = 0; true; i++) {
		IDXGIAdapter* adapter = nullptr;
		if (FAILED(factory->EnumAdapters(i, &adapter))) {
			break;
		}

		DXGI_ADAPTER_DESC desc;
		if (FAILED(adapter->GetDesc(&desc))) {
			fprintf(stderr, "Failed to get adapter description.\n");
		}
		
		fprintf(stderr, "Adapter (%ws)\n", desc.Description);

		for (int j = 0; true; j++) {
			IDXGIOutput* output = nullptr;
			if (FAILED(adapter->EnumOutputs(j, &output))) {
				break;
			}

			DXGI_OUTPUT_DESC desc;
			if (FAILED(output->GetDesc(&desc))) {
				fprintf(stderr, "Failed to get monitor description.\n");
			}

			fprintf(stderr, "\tOutput %d (%ws)\n", index, desc.DeviceName);

			index += 1;
			outputs.push_back(output);
		}
	}

	// Get the test to perform
	fprintf(stderr, "\n");

	int syncMode = 0;
	fprintf(stderr, "Select sync mode (DXGIOutput::WaitForVBlank = 0, DwmFlush = 1): ");
	if (scanf_s("%d", &syncMode) != 1) {
		return false;
	}

	int outputIndex = 0;
	if (syncMode != 1) {
		fprintf(stderr, "Select output index: ");
		if (scanf_s("%d", &outputIndex) != 1) {
			return false;
		}
	}

	FILE* file = stdout;
#if defined(OUTPUT_FILE)
	fopen_s(&file, "vblank.csv", "w");
#endif

	bool result = test(
		file,
		syncMode == 1,
		outputs[outputIndex]);

	return result ? 0 : 1;
}
